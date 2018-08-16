#include "Ctu.h"
#include <lz4.h>

Nxo::Nxo(string fn) {
	fp.open(fn, ios::in | ios::binary);
	fp.seekg(0, ios_base::end);
	length = (uint32_t) fp.tellg();
	fp.seekg(0);
}

typedef struct {
    uint32_t out_offset;
    uint32_t out_size;
    uint32_t compressed_size;
    uint32_t attribute;
} kip_section_header_t;

typedef struct {
    uint32_t magic;
    char name[0xC];
    uint64_t title_id;
    uint32_t process_category;
    uint8_t main_thread_priority;
    uint8_t default_core;
    uint8_t _0x1E;
    uint8_t flags;
    kip_section_header_t section_headers[6];
    uint32_t capabilities[0x20];
} kip1_header_t;

typedef struct {
	uint32_t compressed_size;
	uint32_t init_index;
	uint32_t uncompressed_added_size;
} blz_footer;

char *kip1_decompress(ifstream &fp, size_t offset, uint32_t csize, uint32_t usize) {
	fp.seekg(offset);

	unsigned char *compressed = new unsigned char[csize];
	unsigned char *decompressed = new unsigned char[usize];
	fp.read((char*)compressed, csize);

	blz_footer *footer = (blz_footer*)(compressed + csize - 0xC);
	// Ugly hack, *not* sorry. Blame SciresM :3
	if (csize != footer->compressed_size) {
		assert(csize == footer->compressed_size + 1);
		footer->init_index -= 1;
	}

	assert(usize == csize + footer->uncompressed_added_size);

	int index = footer->compressed_size - footer->init_index;
	int outindex = usize;
	while (outindex > 0) {
		index -= 1;
		unsigned char control = compressed[index];
		for (int i = 0; i < 8; i++) {
			if (control & 0x80) {
				if (index < 2) {
					// Compression out of bounds
					assert(false);
				}
				index -= 2;
				unsigned int segmentoffset = compressed[index] | (compressed[index + 1] << 8);
				unsigned int segmentsize = ((segmentoffset >> 12) & 0xF) + 3;
				segmentoffset &= 0x0FFF;
				segmentoffset += 2;
				if (outindex < segmentsize) {
					// Compression out of bounds
					assert(false);
				}
				for (int j = 0; j < segmentsize; j++) {
					if (outindex + segmentoffset >= usize) {
						// Compression out of boundds
						assert(false);
					}
					char data = decompressed[outindex + segmentoffset];
					outindex -= 1;
					decompressed[outindex] = data;
				}
			} else {
				if (outindex < 1) {
					// Compression out of bounds
					assert(false);
				}
				outindex -= 1;
				index -= 1;
				decompressed[outindex] = compressed[index];
			}
			control <<= 1;
			control &= 0xFF;
			if (outindex == 0)
				break;
		}
	}
	delete[] compressed;
	return (char*)decompressed;
}

static uint32_t align(uint32_t offset, uint32_t alignment) {
    uint32_t mask = ~(alignment-1);

    return (offset + (alignment-1)) & mask;
}

guint Kip::load(Ctu &ctu, gptr base, bool relocate) {
	kip1_header_t hdr;
	fp.read((char*)&hdr, sizeof(kip1_header_t));
	if (hdr.magic != FOURCC('K', 'I', 'P', '1'))
		return 0;

	// TODO: probably inaccurate
	// Find max size
	gptr tmaxsize = hdr.section_headers[0].out_offset + align(hdr.section_headers[0].out_size, 0x1000);
	gptr romaxsize = hdr.section_headers[1].out_offset + align(hdr.section_headers[1].out_size, 0x1000);
	gptr rwmaxsize = hdr.section_headers[2].out_offset + align(hdr.section_headers[2].out_size, 0x1000);
	gptr bssmaxsize = hdr.section_headers[3].out_offset + align(hdr.section_headers[3].out_size, 0x1000);

	gptr maxsize = std::max( { tmaxsize, romaxsize, rwmaxsize, bssmaxsize } );
	printf("WTF IS THIS: " LONGFMT "\n", maxsize);
	ctu.cpu.map(base, maxsize);

	size_t toff = 0x100;
	size_t roff = toff + hdr.section_headers[0].compressed_size;
	size_t doff = roff + hdr.section_headers[1].compressed_size;

	char *text = kip1_decompress(fp, toff, hdr.section_headers[0].compressed_size, hdr.section_headers[0].out_size);
	ctu.cpu.writemem(base + hdr.section_headers[0].out_offset, text, hdr.section_headers[0].out_size);
	delete[] text;

	char *ro = kip1_decompress(fp, roff, hdr.section_headers[1].compressed_size, hdr.section_headers[1].out_size);
	ctu.cpu.writemem(base + hdr.section_headers[1].out_offset, ro, hdr.section_headers[1].out_size);
	delete[] ro;

	char *data = kip1_decompress(fp, doff, hdr.section_headers[2].compressed_size, hdr.section_headers[2].out_size);
	ctu.cpu.writemem(base + hdr.section_headers[2].out_offset, data, hdr.section_headers[2].out_size);
	delete[] data;

	return maxsize;
}

typedef struct {
	uint32_t magic, pad0, pad1, flags;
	uint32_t textOff, textLoc, textSize, pad3;
	uint32_t rdataOff, rdataLoc, rdataSize, pad4;
	uint32_t dataOff, dataLoc, dataSize;
	uint32_t bssSize;
} NsoHeader;

char *nso_read_and_decompress(ifstream &fp, uint32_t offset, uint32_t csize, uint32_t usize, bool need_decompression) {
	fp.seekg(offset);
	char *buf = new char[csize];
	fp.read(buf, csize);
	if (!need_decompression)
		return buf;
	char *obuf = new char[usize];
	assert(LZ4_decompress_safe(buf, obuf, csize, usize) == usize);
	delete[] buf;
	return obuf;
}

guint Nso::load(Ctu &ctu, gptr base, bool relocate) {
	NsoHeader hdr;
	fp.read((char *) &hdr, sizeof(NsoHeader));
	if(hdr.magic != FOURCC('N', 'S', 'O', '0'))
		return 0;

	gptr tsize = hdr.dataLoc + hdr.dataSize + hdr.bssSize;
	if(tsize & 0xFFF)
		tsize = (tsize & ~0xFFF) + 0x1000;

	ctu.cpu.map(base, tsize);

	bool is_text_compressed = ((hdr.flags >> 0) & 1) != 0;
	char *text = nso_read_and_decompress(fp, hdr.textOff, hdr.rdataOff - hdr.textOff, hdr.textSize, is_text_compressed);
	ctu.cpu.writemem(base + hdr.textLoc, text, hdr.textSize);
	delete[] text;

	bool is_rdata_compressed = ((hdr.flags >> 1) & 1) != 0;
	char *rdata = nso_read_and_decompress(fp, hdr.rdataOff, hdr.dataOff - hdr.rdataOff, hdr.rdataSize, is_rdata_compressed);
	ctu.cpu.writemem(base + hdr.rdataLoc, rdata, hdr.rdataSize);
	delete[] rdata;

	bool is_data_compressed = ((hdr.flags >> 2) & 1) != 0;
	char *data = nso_read_and_decompress(fp, hdr.dataOff, length - hdr.dataOff, hdr.dataSize, is_data_compressed);
	ctu.cpu.writemem(base + hdr.dataLoc, data, hdr.dataSize);
	delete[] data;

	return tsize;
}

typedef struct {
	uint32_t unk0, moduleHeaderOff, unk2, unk3;
	uint32_t magic, unk5, fileSize, unk7;
	uint32_t textOff, textSize, roOff, roSize;
	uint32_t dataOff, dataSize, bssSize, unk15;
	uint8_t restOfTheHeader[0x80]; // bunch of hashes and stuff
} NroHeader;

typedef struct {
	uint32_t magic, dynamicOff, bssStartOff, bssEndOff;
	uint32_t unwindStartOff, unwindEndOff, moduleObjectOff;
} ModuleHeader;

typedef struct {
  int64_t d_tag;
  union {
    uint64_t d_val;
    gptr d_ptr;
  };
} Elf64_Dyn;

typedef struct {
	uint64_t r_offset;
	uint32_t r_reloc_type;
	uint32_t r_symbol;
	uint64_t r_addend;
} Elf64_Rela;

guint Nro::load(Ctu &ctu, gptr base, bool relocate) {
	NroHeader hdr;
	fp.read((char *) &hdr, sizeof(NroHeader));
	if(hdr.magic != FOURCC('N', 'R', 'O', '0')) {
		return 0;
	}

	gptr tsize = hdr.fileSize + hdr.bssSize;
	ctu.cpu.map(base, tsize);

	char *image = new char[hdr.fileSize];
	fp.seekg(0);
	fp.read(image, hdr.fileSize);
	
	if(relocate) {
		// stupid tiny linker
		ModuleHeader moduleHeader = *(ModuleHeader*) (image + hdr.moduleHeaderOff);
		Elf64_Dyn *dynamic = (Elf64_Dyn*) (image + hdr.moduleHeaderOff + moduleHeader.dynamicOff);

		uint64_t relaOffset = 0;
		uint64_t relaSize = 0;
		uint64_t relaEnt = 0;
		uint64_t relaCount = 0;
		bool foundRela = false;
		
		while(dynamic->d_tag > 0) {
			switch(dynamic->d_tag) {
			case 7: // DT_RELA
				if(foundRela) {
					LOG_ERROR(TLD, "two DT_RELA entries found");
				}
				relaOffset = dynamic->d_val;
				foundRela = true;
				break;
			case 8: // DT_RELASZ
				relaSize = dynamic->d_val;
				break;
			case 9: // DT_RELAENT
				relaEnt = dynamic->d_val;
				break;
			case 16: // DT_SYMBOLIC
				break;
			case 0x6ffffff9: // DT_RELACOUNT
				relaCount = dynamic->d_val;
				break;
			}
			dynamic++;
		}

		if(relaEnt != 0x18) {
			LOG_ERROR(TLD, "relaEnt != 0x18");
		}

		if(sizeof(Elf64_Rela) != 0x18) {
			LOG_ERROR(TLD, "sizeof(Elf64_Rela) != 0x18");
		}

		if(relaSize != relaCount * relaEnt) {
			LOG_ERROR(TLD, "relaSize mismatch");
		}
		
		Elf64_Rela *relaBase = (Elf64_Rela*) (image + relaOffset);
		for(int i = 0; i < relaCount; i++) {
			Elf64_Rela rela = relaBase[i];
			
			switch(rela.r_reloc_type) {
			case 0x403: // R_AARCH64_RELATIVE
				if(rela.r_symbol != 0) {
					LOG_ERROR(TLD, "R_AARCH64_RELATIVE with symbol unsupported");
				}
				*(uint64_t*)(image + rela.r_offset) = base + rela.r_addend;
				break;
			default:
				LOG_ERROR(TLD, "unsupported relocation %x", rela.r_reloc_type);
			}
		}
	}
        
	ctu.cpu.writemem(base, image, hdr.fileSize);
	delete[] image;
        
	return tsize;
}
