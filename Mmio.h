#pragma once

#include "Ctu.h"

class MmioBase {
public:
	MmioBase() {}
	virtual ~MmioBase() {
		for(auto entry : storedValues) {
			free(entry.second);
		}
	}
	void Setup();

	virtual bool sread(gptr addr, guint size, void *out) {
		return false;
	}
	virtual bool swrite(gptr addr, guint size, void *value) {
		cout << "no" << endl;
		return false;
	}

	void setSize(guint _size) {
		mmioSize = _size;
	}

	void setOffset(guint _offset) {
		offsetFromMMIO = _offset;
	}
	guint offsetFromMMIO;
	guint mmioSize;

private:

	unordered_map<gptr, void *> storedValues;
};

class Mmio {
public:
	Mmio(Ctu *ctu);
	virtual ~Mmio() {}
	gptr getVirtualAddressFromAddr(gptr addr);
	gptr getPhysicalAddressFromVirtual(gptr addr);
	MmioBase *getMMIOFromPhysicalAddress(gptr addr);
	void MMIOInitialize();

	gptr GetBase() {
		return mmioBaseAddr;
	}

	guint GetSize() {
		return mmioBaseSize;
	}
private:
	Ctu *ctu;
	void MMIORegister(gptr base, guint size, MmioBase *mmioBase);
	gptr mmioBaseAddr = 0x4000000;//1 << 58;
	guint mmioBaseSize = 0;
	unordered_map<gptr, MmioBase *> mmioBases;
};

class ApbMmio : public MmioBase {
public:
	bool sread(gptr addr, guint size, void *out) {
		return false;
	}
	bool swrite(gptr addr, guint size, void *value) {
		//*value = 0xdeadbeef;
		return false;
	}
};
