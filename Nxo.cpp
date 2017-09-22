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
