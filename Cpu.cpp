#include "Ctu.h"

void intrHook(uc_engine *uc, uint32_t intNo, void *user_data) {
	((Cpu *) user_data)->interruptHook(intNo);
}

bool unmpdHook(uc_engine *uc, uc_mem_type type, gptr addr, uint32_t size, guint value, void *user_data) {
	return ((Cpu *) user_data)->unmappedHook(type, addr, size, value);
}

void mmioHook(uc_engine *uc, uc_mem_type type, gptr address, uint32_t size, gptr value, void *user_data) {
	auto cpu = (Cpu *) user_data;
	auto physicalAddress = cpu->mmioHandler->getPhysicalAddressFromVirtual(address);
	auto mmio = cpu->mmioHandler->getMMIOFromPhysicalAddress(address);
	assert(mmio != nullptr);
	switch(type) {
		case UC_MEM_READ:
			LOG_DEBUG(Cpu, "MMIO Read at " ADDRFMT " size %x", physicalAddress, size);
			
			uint64_t ovalue;
			if(mmio->read(physicalAddress, size, ovalue)) {
				switch(size) {
					case 1:
						cpu->guestptr<uint8_t>(address) = (uint8_t) ovalue;
						break;
					case 2:
						cpu->guestptr<uint16_t>(address) = (uint16_t) ovalue;
						break;
					case 4:
						cpu->guestptr<uint32_t>(address) = (uint32_t) ovalue;
						break;
					case 8:
						cpu->guestptr<uint64_t>(address) = ovalue;
						break;
				}
			}
			
			break;
		case UC_MEM_WRITE:
			LOG_DEBUG(Cpu, "MMIO Write at " ADDRFMT " size %x data " LONGFMT, physicalAddress, size, value);
			mmio->write(physicalAddress, size, value);
			break;
	}
}

void codeBpHook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
	auto ctu = (Ctu *) user_data;
	cout << "Hit breakpoint at ... " << hex << address << endl;
	auto thread = ctu->tm.current();
	assert(thread != nullptr);
	ctu->tm.requeue();
	thread->regs.PC = address;
	ctu->cpu.stop();
	ctu->gdbStub._break();
}

void memBpHook(uc_engine *uc, uc_mem_type type, uint64_t address, uint32_t size, uint64_t value, void *user_data) {
	auto ctu = (Ctu *) user_data;
	if(ctu->cpu.hitMemBreakpoint) {
		ctu->cpu.hitMemBreakpoint = false;
		return;
	}
	cout << "Hit memory breakpoint accessing ... " << hex << address << endl;
	auto thread = ctu->tm.current();
	assert(thread != nullptr);
	ctu->tm.requeue();
	ctu->cpu.stop();
	ctu->gdbStub._break(true);
	ctu->cpu.hitMemBreakpoint = true;
}

Cpu::Cpu(Ctu *_ctu) : ctu(_ctu) {
	CHECKED(uc_open(UC_ARCH_ARM64, UC_MODE_ARM, &uc));

	CHECKED(uc_mem_map(uc, TERMADDR, 0x1000, UC_PROT_ALL));
	guestptr<uint32_t>(TERMADDR) = 0xd40000e1; // svc 0x7 (ExitProcess)

	auto fpv = 3 << 20;
	CHECKED(uc_reg_write(uc, UC_ARM64_REG_CPACR_EL1, &fpv));

	uc_hook hookHandle;
	CHECKED(uc_hook_add(uc, &hookHandle, UC_HOOK_INTR, (void *) intrHook, this, 0, -1));
	CHECKED(uc_hook_add(uc, &hookHandle, UC_HOOK_MEM_INVALID, (void *) unmpdHook, this, 0, -1));

	for(auto i = 0; i < 0x80; ++i)
		svcHandlers[i] = nullptr;

	hitMemBreakpoint = false;
}

Cpu::~Cpu() {
	CHECKED(uc_close(uc));
}

guint Cpu::call(gptr _pc, guint x0, guint x1, guint x2, guint x3) {
	reg(0, x0);
	reg(1, x1);
	reg(2, x2);
	reg(3, x3);
	reg(30, TERMADDR);
	CHECKED(uc_emu_start(uc, _pc, TERMADDR + 4, 0, 0));
	return reg(0);
}

void Cpu::setMmio(Mmio *_mmioHandler) {
	mmioHandler = _mmioHandler;
	uc_hook hookHandle;
	CHECKED(uc_hook_add(uc, &hookHandle, UC_HOOK_MEM_READ, (void *)mmioHook, this, mmioHandler->GetBase(), mmioHandler->GetBase()+mmioHandler->GetSize() ));
	CHECKED(uc_hook_add(uc, &hookHandle, UC_HOOK_MEM_WRITE, (void *)mmioHook, this, mmioHandler->GetBase(), mmioHandler->GetBase()+mmioHandler->GetSize() ));
}

void Cpu::exec(size_t insnCount) {
	CHECKED(uc_emu_start(uc, pc(), TERMADDR + 4, 0, insnCount));
}

void Cpu::stop() {
	CHECKED(uc_emu_stop(uc));
}

bool Cpu::map(gptr addr, guint size) {
	CHECKED(uc_mem_map(uc, addr, size, UC_PROT_ALL));
	auto temp = new uint8_t[size];
	if(ctu->initializeMemory) {
		uint8_t val[] = "badmem!!";
		for(size_t sz = 0; sz < size; sz++) {
			temp[sz] = val[sz % sizeof(val)];
		}
	} else {
		memset(temp, 0, size);
	}
	writemem(addr, temp, size);
	delete[] temp;
	return true;
}

bool Cpu::unmap(gptr addr, guint size) {
	CHECKED(uc_mem_unmap(uc, addr, size));
	return true;
}

list<tuple<gptr, gptr, int>> Cpu::regions() {
	list<tuple<gptr, gptr, int>> ret;

	uc_mem_region *regions;
	uint32_t count;

	CHECKED(uc_mem_regions(uc, &regions, &count));
	list<tuple<gptr, gptr>> temp;
	for(auto i = 0; i < count; ++i) {
		auto region = regions[i];
		temp.push_back(make_tuple(region.begin, region.end));
	}
	uc_free(regions);

	temp.sort([](auto a, auto b) { auto [ab, _] = a; auto [bb, __] = b; return ab < bb; });

	gptr last = 0;
	for(auto [begin, end] : temp) {
		if(last != begin)
			ret.push_back(make_tuple(last, begin - 1, -1));
		ret.push_back(make_tuple(begin, end, 0));
		last = end + 1;
	}

	if(last != 0xFFFFFFFFFFFFFFFF)
		ret.push_back(make_tuple(last, 0xFFFFFFFFFFFFFFFF, -1));

	return ret;
}

bool Cpu::readmem(gptr addr, void *dest, guint size) {
	return uc_mem_read(uc, addr, dest, size) == UC_ERR_OK;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
void Cpu::dumpmem(gptr addr, guint size) {
	char *data = (char*)malloc(size);
	memset(data, 0, size);
	readmem(addr, data, size);

	auto hfmt = "%08lx | ";
	if((addr + size) & 0xFFFF000000000000)
		hfmt = "%016lx | ";
	else if((addr + size) & 0xFFFFFFFF00000000)
		hfmt = "%012lx | ";

	for(uint32_t i = 0; i < size; i += 16) {
		printf(hfmt, addr+i);
		string ascii = "";
		for(uint8_t j = 0; j < 16; j++) {
			if((i+j) < size) {
				printf("%02x ", (uint8_t)data[i+j]);
				if(isprint(data[i+j]))
					ascii += data[i+j];
				else
					ascii += ".";
			} else {
				printf("  ");
				ascii += " ";
			}
			if(j==7) {
				printf(" ");
				ascii += " ";
			}
		}
		printf("| %s\n", ascii.c_str());
	}

	free(data);
}
#pragma clang diagnostic pop

guchar Cpu::read8(gptr addr) {
	return *guestptr<guchar>(addr);
}

std::string Cpu::readstring(gptr addr) {
	std::string out;
	uint32_t offset = 0;
	while(guchar c = read8(addr+offset)) {
		if(c == 0)
			break;
		out += c;
		offset++;
	}
	return out;
}

bool Cpu::writemem(gptr addr, void *src, guint size) {
	return uc_mem_write(uc, addr, src, size) == UC_ERR_OK;
}

gptr Cpu::pc() {
	gptr val;
	CHECKED(uc_reg_read(uc, UC_ARM64_REG_PC, &val));
	return val;
}

void Cpu::pc(gptr val) {
	CHECKED(uc_reg_write(uc, UC_ARM64_REG_PC, &val));
}

guint Cpu::reg(uint regn) {
	guint val;
	auto treg = UC_ARM64_REG_SP;
	if(regn <= 28)
		treg = (uc_arm64_reg) (UC_ARM64_REG_X0 + regn);
	else if(regn < 31)
		treg = (uc_arm64_reg) (UC_ARM64_REG_X29 + regn - 29);
	CHECKED(uc_reg_read(uc, treg, &val));
	return val;
}

void Cpu::reg(uint regn, guint val) {
	auto treg = UC_ARM64_REG_SP;
	if(regn <= 28)
		treg = (uc_arm64_reg) (UC_ARM64_REG_X0 + regn);
	else if(regn < 31)
		treg = (uc_arm64_reg) (UC_ARM64_REG_X29 + regn - 29);
	CHECKED(uc_reg_write(uc, treg, &val));
}

void Cpu::loadRegs(ThreadRegisters &regs) {
	int uregs[32];
	void *tregs[32];

	CHECKED(uc_reg_write(uc, UC_ARM64_REG_SP, &regs.SP));
	CHECKED(uc_reg_write(uc, UC_ARM64_REG_PC, &regs.PC));
	CHECKED(uc_reg_write(uc, UC_ARM64_REG_NZCV, &regs.NZCV));

	for(auto i = 0; i < 29; ++i) {
		uregs[i] = UC_ARM64_REG_X0 + i;
		tregs[i] = &regs.gprs[i];
	}
	CHECKED(uc_reg_write_batch(uc, uregs, tregs, 29));
	CHECKED(uc_reg_write(uc, UC_ARM64_REG_X29, &regs.X29));
	CHECKED(uc_reg_write(uc, UC_ARM64_REG_X30, &regs.X30));

	for(auto i = 0; i < 32; ++i) {
		uregs[i] = UC_ARM64_REG_Q0 + i;
		tregs[i] = &regs.fprs[i];
	}
	CHECKED(uc_reg_write_batch(uc, uregs, tregs, 32));
}

void Cpu::storeRegs(ThreadRegisters &regs) {
	int uregs[32];
	void *tregs[32];

	CHECKED(uc_reg_read(uc, UC_ARM64_REG_SP, &regs.SP));
	CHECKED(uc_reg_read(uc, UC_ARM64_REG_PC, &regs.PC));
	CHECKED(uc_reg_read(uc, UC_ARM64_REG_NZCV, &regs.NZCV));

	for(auto i = 0; i < 29; ++i) {
		uregs[i] = UC_ARM64_REG_X0 + i;
		tregs[i] = &regs.gprs[i];
	}
	CHECKED(uc_reg_read_batch(uc, uregs, tregs, 29));
	CHECKED(uc_reg_read(uc, UC_ARM64_REG_X29, &regs.X29));
	CHECKED(uc_reg_read(uc, UC_ARM64_REG_X30, &regs.X30));

	for(auto i = 0; i < 32; ++i) {
		uregs[i] = UC_ARM64_REG_Q0 + i;
		tregs[i] = &regs.fprs[i];
	}
	CHECKED(uc_reg_read_batch(uc, uregs, tregs, 32));
}

gptr Cpu::tlsBase() {
	gptr base;
	CHECKED(uc_reg_read(uc, UC_ARM64_REG_TPIDRRO_EL0, &base));
	return base;
}

void Cpu::tlsBase(gptr base) {
	CHECKED(uc_reg_write(uc, UC_ARM64_REG_TPIDRRO_EL0, &base));
}

void Cpu::interruptHook(uint32_t intNo) {
	uint32_t esr;
	CHECKED(uc_reg_read(uc, UC_ARM64_REG_ESR, &esr));
	auto ec = esr >> 26;
	auto iss = esr & 0xFFFFFF;
	switch(ec) {
		case 0x15: // SVC
			if(iss >= 0x80 || svcHandlers[iss] == nullptr)
				LOG_ERROR(Cpu, "Unhandled SVC 0x%02x", iss);
			svcHandlers[iss](this);
			break;
	}
}

bool Cpu::unmappedHook(uc_mem_type type, gptr addr, int size, guint value) {
	cout << "!!!!!!!!!!!!!!!!!!!!!!!! Unmapped !!!!!!!!!!!!!!!!!!!!!!!" << endl;
	switch(type) {
		case UC_MEM_READ_UNMAPPED:
		case UC_MEM_READ_PROT:
			LOG_INFO(Cpu, "Attempted to read from %s memory at " ADDRFMT " from " ADDRFMT, (type == UC_MEM_READ_UNMAPPED ? "unmapped" : "protected"), addr, pc());
			break;
		case UC_MEM_FETCH_UNMAPPED:
		case UC_MEM_FETCH_PROT:
			LOG_INFO(Cpu, "Attempted to fetch from %s memory at " ADDRFMT " from " ADDRFMT, (type == UC_MEM_READ_UNMAPPED ? "unmapped" : "protected"), addr, pc());
			break;
		case UC_MEM_WRITE_UNMAPPED:
		case UC_MEM_WRITE_PROT:
			LOG_INFO(Cpu, "Attempted to write " LONGFMT " to %s memory at " ADDRFMT " from " ADDRFMT, value, (type == UC_MEM_READ_UNMAPPED ? "unmapped" : "protected"), addr, pc());
			break;
	}
	if(ctu->gdbStub.enabled) {
		auto thread = ctu->tm.current();
		if(thread == nullptr)
			return false;
		ctu->tm.requeue();
		ctu->gdbStub._break(false);
		if(type == UC_MEM_FETCH_PROT || type == UC_MEM_FETCH_UNMAPPED)
			pc(TERMADDR);
		else
			stop();
		return true;
	}
	return false;
}

void Cpu::registerSvcHandler(int num, std::function<void()> handler) {
	registerSvcHandler(num, [=](auto _) { handler(); });
}

void Cpu::registerSvcHandler(int num, std::function<void(Cpu *)> handler) {
	svcHandlers[num] = handler;
}

hook_t Cpu::addCodeBreakpoint(gptr addr) {
	assert(ctu->gdbStub.enabled);

	hook_t hookHandle;
	CHECKED(uc_hook_add(uc, &hookHandle, UC_HOOK_CODE, (void *)codeBpHook, ctu, addr, addr + 2));
	return hookHandle;
}

hook_t Cpu::addMemoryBreakpoint(gptr addr, guint len, BreakpointType type) {
	assert(ctu->gdbStub.enabled);

	hook_t hookHandle;
	CHECKED(uc_hook_add(uc, &hookHandle, ((type == BreakpointType::Read || type == BreakpointType::Access) ? UC_HOOK_MEM_READ : 0) | ((type == BreakpointType::Write || type == BreakpointType::Access) ? UC_HOOK_MEM_WRITE : 0), (void *)memBpHook, ctu, addr, addr + len));
	return hookHandle;
}

void Cpu::removeBreakpoint(hook_t hook) {
	CHECKED(uc_hook_del(uc, hook));
}
