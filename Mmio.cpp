#include "Ctu.h"

Mmio::Mmio(Ctu *_ctu) {
	MMIORegister(0x70000000, 0x1000, new ApbMmio());

	MMIORegister(0x702ec000, 0x2000, new ApbMmio());
	MMIORegister(0x70030000, 0x8000, new ApbMmio());
	MMIORegister(0x702ef700, 0x40, new ApbMmio());
	MMIORegister(0x702f9000, 0x1000, new ApbMmio());
	
	//MMIORegister(0x70030000, 0x8000, new ApbMmio());
	ctu = _ctu;
	//MMIOInitialize();
}

void Mmio::MMIORegister(gptr base, guint size, MmioBase *mmioBase) {
	LOG_DEBUG(Mmio, "Registered MMIO " ADDRFMT, base);
	mmioBase->setSize(size);
	mmioBases[base] = mmioBase;
}

void Mmio::MMIOInitialize() {
	for(auto item : mmioBases) {
		item.second->setOffset(mmioBaseSize);
		mmioBaseSize += item.second->mmioSize;
	}
	if(mmioBaseSize & 0xFFF) 
		mmioBaseSize = (mmioBaseSize & ~0xFFF) + 0x1000;

	LOG_DEBUG(Mmio, "Mapping 0x" ADDRFMT " size 0x%x", mmioBaseAddr, (uint) mmioBaseSize);
	ctu->cpu.map(mmioBaseAddr, mmioBaseSize);
}

gptr Mmio::getVirtualAddressFromAddr(gptr addr) {
	for(auto item : mmioBases) {
		if(addr >= item.first && addr <= (item.first+item.second->mmioSize)) {
			return mmioBaseAddr+item.second->offsetFromMMIO;
		}
	}
	return 0x0;
}

gptr Mmio::getPhysicalAddressFromVirtual(gptr addr) {
	if(addr < mmioBaseAddr) return 0x0;
	gptr offset = addr - mmioBaseAddr;
	for(auto item : mmioBases) {
		if(item.second->offsetFromMMIO >= offset && offset <= item.second->offsetFromMMIO+item.second->mmioSize) {
			return item.first+offset;
		}
	}
	return 0x0;
}


MmioBase *Mmio::getMMIOFromPhysicalAddress(gptr addr) {
	if(addr < mmioBaseAddr) return nullptr;
	gptr offset = addr - mmioBaseAddr;
	for(auto item : mmioBases) {
		if(item.second->offsetFromMMIO >= offset && offset <= item.second->offsetFromMMIO+item.second->mmioSize) {
			return item.second;
		}
	}
	return nullptr;
}
