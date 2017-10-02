#include "Ctu.h"
#include <lz4.h>

Nxo::Nxo(string fn) {
	fp.open(fn, ios::in | ios::binary);
	fp.seekg(0, ios_base::end);
	length = (uint32_t) fp.tellg();
	fp.seekg(0);
}

typedef struct {
	uint32_t magic, pad0, pad1, pad2;
	uint32_t textOff, textLoc, textSize, pad3;
	uint32_t rdataOff, rdataLoc, rdataSize, pad4;
	uint32_t dataOff, dataLoc, dataSize;
	uint32_t bssSize;
} NsoHeader;

char *decompress(ifstream &fp, uint32_t offset, uint32_t csize, uint32_t usize) {
	fp.seekg(offset);
	char *buf = new char[csize];
	char *obuf = new char[usize];
	fp.read(buf, csize);
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

	char *text = decompress(fp, hdr.textOff, hdr.rdataOff - hdr.textOff, hdr.textSize);
	ctu.cpu.writemem(base + hdr.textLoc, text, hdr.textSize);
	delete[] text;

	char *rdata = decompress(fp, hdr.rdataOff, hdr.dataOff - hdr.rdataOff, hdr.rdataSize);
	ctu.cpu.writemem(base + hdr.rdataLoc, rdata, hdr.rdataSize);
	delete[] rdata;

	char *data = decompress(fp, hdr.dataOff, length - hdr.dataOff, hdr.dataSize);
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
			LOG_DEBUG(TLD, "found d_tag %lx", dynamic->d_tag);
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
			LOG_DEBUG(TLD, "r_offset: %lx", rela.r_offset);
			LOG_DEBUG(TLD, "r_reloc_type: %x", rela.r_reloc_type);
			LOG_DEBUG(TLD, "r_symbol: %x", rela.r_symbol);
			LOG_DEBUG(TLD, "r_addend: %lx", rela.r_addend);

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
