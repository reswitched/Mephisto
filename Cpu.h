#pragma once
#include "Ctu.h"
#include <unicorn/unicorn.h>

typedef uc_hook hook_t;

#define CHECKED(expr) do { if(auto _cerr = (expr)) { printf("Call " #expr " failed with error: %u (%s)\n", _cerr, uc_strerror(_cerr)); exit(1); } } while(0)

template<typename T> class Guest;

class Cpu {
public:
	Cpu(Ctu *_ctu);
	~Cpu();

	uint64_t call(gptr addr, uint64_t x0=0, uint64_t x1=0, uint64_t x2=0, uint64_t x3=0);
	void exec(size_t insnCount=0);
	void stop();

	bool map(gptr addr, guint size);
	bool unmap(gptr addr, guint size);
	list<tuple<gptr, guint, int>> regions();
	bool readmem(gptr addr, void *dest, guint size);
	guchar read8(gptr addr);
	void dumpmem(gptr addr, guint size);
	std::string readstring(gptr addr);
	bool writemem(gptr addr, void *src, guint size);

	gptr pc();
	void pc(gptr val);
	guint reg(uint reg);
	void reg(uint reg, guint val);

	void loadRegs(ThreadRegisters &regs);
	void storeRegs(ThreadRegisters &regs);

	gptr tlsBase();
	void tlsBase(gptr base);

	template<typename T> Guest<T> guestptr(gptr addr) {
		Guest<T> ret(this, addr);
		return ret;
	}

	void registerSvcHandler(int num, std::function<void()> handler);
	void registerSvcHandler(int num, std::function<void(Cpu *)> handler);

	hook_t addCodeBreakpoint(gptr addr);
	hook_t addMemoryBreakpoint(gptr addr, guint len, BreakpointType type);
	void removeBreakpoint(hook_t hook);

	void interruptHook(uint32_t intNo);
	bool unmappedHook(uc_mem_type type, gptr addr, int size, guint value);

	void setMmio(Mmio *_mmioHandler);// { mmioHandler = _mmioHandler; }
	Mmio *mmioHandler;

	bool hitMemBreakpoint;

private:
	Ctu *ctu;
	uc_engine *uc;
	std::function<void(Cpu *)> svcHandlers[0x80];
};

template<typename T>
class Guest {
public:
	Guest(Cpu *cpu, gptr addr) : cpu(cpu), addr(addr) {
	}

	const T& operator*() {
		cpu->readmem(addr, &store, sizeof(T));
		return store;
	}

	T* operator->() {
		cpu->readmem(addr, &store, sizeof(T));
		return &store;
	}

	Guest<T> &operator=(const T &v) {
		cpu->writemem(addr, (void *) &v, sizeof(T));
		return *this;
	}

	const T &operator[](int i) {
		return *Guest<T>(cpu, addr + i * sizeof(T));
	}

	const T &operator[](unsigned int i) {
		return *Guest<T>(cpu, addr + i * sizeof(T));
	}

	Guest<T> operator+(const int &i) {
		return Guest<T>(cpu, addr + i * sizeof(T));
	}

	void writeback() {
		cpu->writemem(addr, &store, sizeof(T));
	}

private:
	Cpu *cpu;
	gptr addr;
	T store;
};
