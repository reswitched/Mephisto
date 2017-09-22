#pragma once

#include <byteswap.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <forward_list>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <unordered_map>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>
#include <sstream>
using namespace std;

typedef __int128_t int128_t;
typedef __uint128_t uint128_t;
typedef float float32_t;
typedef double float64_t;
typedef uint64_t gptr;
typedef uint64_t guint;
typedef uint8_t guchar;
typedef uint16_t gushort;
typedef uint32_t ghandle;
typedef uint64_t gpid;
const gptr TERMADDR = 1ULL << 61;

#define FOURCC(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

#define ADDRFMT "%016lx"
#define LONGFMT "%lx"

enum LogLevel {
	None = 0, 
	Error = 1, 
	Warn = 2, 
	Debug = 3, 
	Info = 4
};

extern LogLevel g_LogLevel;

#define LOG_ERROR(module, msg, ...) do { \
	if(g_LogLevel >= Error) { \
		fprintf(stderr, "[" #module "] ERROR: " msg "\n", ##__VA_ARGS__); \
		cerr << endl; \
	} \
	exit(1); \
} while(0)
#define LOG_INFO(module, msg, ...) do { \
	if(g_LogLevel >= Info) { \
		printf("[" #module "] INFO: " msg, ##__VA_ARGS__); \
		cout << endl; \
	} \
} while(0)
#define LOG_DEBUG(module, msg, ...) do { \
	if(g_LogLevel >= Debug) { \
		printf("[" #module "] DEBUG: " msg, ##__VA_ARGS__); \
		cout << endl; \
	} \
} while(0)

class Ctu;

#include "optionparser.h"
#include "Lisparser.h"
#include "KObject.h"
#include "ThreadManager.h"
#include "Mmio.h"
#include "Cpu.h"
#include "Sync.h"
#include "Svc.h"
#include "Ipc.h"
#include "Nxo.h"
#include "IpcBridge.h"
#include "GdbStub.h"

template<unsigned long N>
void hexdump(shared_ptr<array<uint8_t, N>> buf, unsigned long count=N) {
	if(g_LogLevel < Debug)
		return;

	for(auto i = 0; i < count; i += 16) {
		printf("%04x | ", i);
		for(auto j = 0; j < 16; ++j) {
			printf("%02x ", buf->data()[i + j]);
			if(j == 7)
				printf(" ");
		}
		printf("| ");
		for(auto j = 0; j < 16; ++j) {
			auto val = buf->data()[i + j];
			if(isprint(val))
				printf("%c", val);
			else
				printf(".");
			if(j == 7)
				printf(" ");
		}
		printf("\n");
	}
	printf("%04x\n", (int) count);
}

class Ctu {
public:
	Ctu();
	void execProgram(gptr ep);

	template<typename T>
	ghandle newHandle(shared_ptr<T> obj) {
		static_assert(std::is_base_of<KObject, T>::value, "T must derive from KObject");
		auto hnd = handleId++;
		handles[hnd] = dynamic_pointer_cast<KObject>(obj);
		return hnd;
	}
	template<typename T>
	shared_ptr<T> getHandle(ghandle handle) {
		static_assert(std::is_base_of<KObject, T>::value, "T must derive from KObject");
		if(handles.find(handle) == handles.end())
			LOG_ERROR(Ctu, "Could not find handle with ID 0x%x !", handle);
		auto obj = handles[handle];
		auto faux = dynamic_pointer_cast<FauxHandle>(obj);
		if(faux != nullptr)
			LOG_ERROR(Ctu, "Accessing faux handle! 0x%x", faux->val);
		auto temp = dynamic_pointer_cast<T>(obj);
		if(temp == nullptr)
			LOG_ERROR(Ctu, "Got null pointer after cast. Before: 0x%p", (void *) obj.get());
		return temp;
	}
	ghandle duplicateHandle(KObject *ptr);
	void deleteHandle(ghandle handle);

	Mmio mmiohandler;
	Cpu cpu;
	Svc svc;
	Ipc ipc;
	ThreadManager tm;
	IpcBridge bridge;
	GdbStub gdbStub;

	guint heapsize;
	gptr loadbase, loadsize;

private:
	ghandle handleId;
	unordered_map<ghandle, shared_ptr<KObject>> handles;
};

#include "IpcStubs.h"
