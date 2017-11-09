#include "Ctu.h"

class ApbMmio : public MmioBase {
public:
	bool read(gptr addr, guint size, uint64_t &value) {
		switch(addr) {
			case 0x70000804: // TEGRA_APB_MISC_BASE | APB_MISC_GP_HIDREV
				value = 0x2100;
				return true;
		}
		return false;
	}
	bool write(gptr addr, guint size, uint64_t value) {
		return false;
	}
};

class FuseMmio : public MmioBase {
public:
	bool read(gptr addr, guint size, uint64_t &value) {
		switch(addr) {
		}
		return false;
	}
	bool write(gptr addr, guint size, uint64_t value) {
		return false;
	}
};

class GpuMmio : public MmioBase {
public:
	bool read(gptr addr, guint size, uint64_t &value) {
		switch(addr) {
			case 0x57000000:
				// boot 0: NVGPU_GPU_ARCH_GM200 | NVGPU_GPU_IMPL_GM20B | revision A1
				value = ((0x120 | 0xB) << 20) | 0xA1;
				return true;
		}
		return false;
	}
	bool write(gptr addr, guint size, uint64_t value) {
		return false;
	}
};

Mmio::Mmio(Ctu *_ctu) {
	MMIORegister(0x50000000, 0x24000, new MmioBase());
	MMIORegister(0x54200000, 0x400000, new MmioBase());
	MMIORegister(0x57000000, 0x1000000, new GpuMmio());
	MMIORegister(0x58000000, 0x1000000, new GpuMmio());
	MMIORegister(0x70000000, 0x1000, new ApbMmio());
	MMIORegister(0x7000f800, 0x400, new FuseMmio());
	MMIORegister(0x702ec000, 0x2000, new ApbMmio());
	MMIORegister(0x70030000, 0x8000, new ApbMmio());
	MMIORegister(0x700e3000, 0x100, new ApbMmio());
	MMIORegister(0x702ef700, 0x40, new ApbMmio());
	MMIORegister(0x702f9000, 0x1000, new ApbMmio());
	
	//MMIORegister(0x70030000, 0x8000, new ApbMmio());
	ctu = _ctu;
}

void Mmio::MMIORegister(gptr base, guint size, MmioBase *mmioBase) {
	LOG_DEBUG(Mmio, "Registered MMIO " ADDRFMT, base);
	mmioBase->setSize(size);

	mmioBases[base] = mmioBase;
}

void Mmio::MMIOInitialize() {
	mmioBaseSize = 0;
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
		if(addr >= item.first && addr < item.first + item.second->mmioSize) {
			return mmioBaseAddr + item.second->offsetFromMMIO;
		}
	}
	return 0x0;
}

gptr Mmio::getPhysicalAddressFromVirtual(gptr addr) {
	if(addr < mmioBaseAddr) return 0x0;
	gptr offset = addr - mmioBaseAddr;
	for(auto item : mmioBases) {
		if(item.second->offsetFromMMIO <= offset && offset < item.second->offsetFromMMIO + item.second->mmioSize) {
			return item.first + offset - item.second->offsetFromMMIO;
		}
	}
	return 0x0;
}


MmioBase *Mmio::getMMIOFromPhysicalAddress(gptr addr) {
	if(addr < mmioBaseAddr) return nullptr;
	gptr offset = addr - mmioBaseAddr;
	for(auto item : mmioBases) {
		if(item.second->offsetFromMMIO <= offset && offset < item.second->offsetFromMMIO + item.second->mmioSize) {
			return item.second;
		}
	}
	return nullptr;
}
