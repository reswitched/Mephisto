#include "Ctu.h"

#include<time.h>

#define IX0 (ctu->cpu.reg(0))
#define IX1 (ctu->cpu.reg(1))
#define IX2 (ctu->cpu.reg(2))
#define IX3 (ctu->cpu.reg(3))
#define IX4 (ctu->cpu.reg(4))
#define IX5 (ctu->cpu.reg(5))

#define registerSvc(num, func, ...) do { \
	ctu->cpu.registerSvcHandler((num), [&] { \
		(func)(__VA_ARGS__); \
	}); \
} while(0)

#define registerSvc_ret_X0_X1_X2(num, func, ...) do { \
	ctu->cpu.registerSvcHandler((num), [&] { \
		ctu->tm.switched = false; \
		auto [_0, _1, _2] = (func)(__VA_ARGS__); \
		if(ctu->tm.switched) { \
			ctu->tm.switched = false; \
			return; \
		} \
		ctu->cpu.reg(0, _0); \
		ctu->cpu.reg(1, _1); \
		ctu->cpu.reg(2, _2); \
	}); \
} while(0)

#define registerSvc_ret_X0(num, func, ...) do { \
	ctu->cpu.registerSvcHandler((num), [&] { \
		ctu->tm.switched = false; \
		auto v = (func)(__VA_ARGS__); \
		if(ctu->tm.switched) { \
			ctu->tm.switched = false; \
			return; \
		} \
		ctu->cpu.reg(0, v); \
	}); \
} while(0)

#define registerSvc_ret_X1(num, func, ...) do { \
	ctu->cpu.registerSvcHandler((num), [&] { \
		ctu->tm.switched = false; \
		auto v = (func)(__VA_ARGS__); \
		if(ctu->tm.switched) { \
			ctu->tm.switched = false; \
			return; \
		} \
		ctu->cpu.reg(1, v); \
	}); \
} while(0)

#define registerSvc_ret_X0_X1(num, func, ...) do { \
	ctu->cpu.registerSvcHandler((num), [&] { \
		ctu->tm.switched = false; \
		auto [_0, _1] = (func)(__VA_ARGS__); \
		if(ctu->tm.switched) { \
			ctu->tm.switched = false; \
			return; \
		} \
		ctu->cpu.reg(0, _0); \
		ctu->cpu.reg(1, _1); \
	}); \
} while(0)

#define UNIMPLEMENTED(svc) do { LOG_ERROR(Svc[svc], "!Unimplemented!"); } while(0)

Svc::Svc(Ctu *_ctu) : ctu(_ctu) {
	registerSvc_ret_X0_X1(    0x01, SetHeapSize, IX1);
	registerSvc_ret_X0(       0x03, SetMemoryAttribute, IX0, IX1, IX2, IX3);
	registerSvc_ret_X0(       0x04, MirrorStack, IX0, IX1, IX2);
	registerSvc_ret_X0(       0x05, UnmapMemory, IX0, IX1, IX2);
	registerSvc_ret_X0_X1(    0x06, QueryMemory, IX0, IX1, IX2);
	registerSvc(              0x07, ExitProcess, IX0);
	registerSvc_ret_X0_X1(    0x08, CreateThread, IX1, IX2, IX3, IX4, IX5);
	registerSvc_ret_X0(       0x09, StartThread, (ghandle) IX0);
	registerSvc(              0x0A, ExitThread);
	registerSvc_ret_X0(       0x0B, SleepThread, IX0);
	registerSvc_ret_X0_X1(    0x0C, GetThreadPriority, (ghandle) IX0);
	registerSvc_ret_X0(       0x0D, SetThreadPriority, (ghandle) IX0, IX1);
	registerSvc_ret_X0_X1_X2( 0x0E, GetThreadCoreMask, IX0);
	registerSvc_ret_X0(       0x0F, SetThreadCoreMask, IX0);
	registerSvc_ret_X0(       0x10, GetCurrentProcessorNumber, IX0);
	registerSvc_ret_X0(       0x11, SignalEvent, (ghandle) IX0);
	registerSvc_ret_X0(       0x12, ClearEvent, (ghandle) IX0);
	registerSvc_ret_X0(       0x13, MapMemoryBlock, (ghandle) IX0, IX1, IX2, IX3);
	registerSvc_ret_X0_X1(    0x15, CreateTransferMemory, IX0, IX1, IX2);
	registerSvc_ret_X0(       0x16, CloseHandle, (ghandle) IX0);
	registerSvc_ret_X0(       0x17, ResetSignal, (ghandle) IX0);
	registerSvc_ret_X0_X1(    0x18, WaitSynchronization, IX1, IX2, IX3);
	registerSvc_ret_X0(       0x19, CancelSynchronization, (ghandle) IX0);
	registerSvc_ret_X0(       0x1A, LockMutex, (ghandle) IX0, IX1, (ghandle) IX2);
	registerSvc(              0x1B, UnlockMutex, IX0);
	registerSvc_ret_X0(       0x1C, WaitProcessWideKeyAtomic, IX0, IX1, (ghandle) IX2, IX3);
	registerSvc_ret_X0(       0x1D, SignalProcessWideKey, IX0, IX1);
	registerSvc_ret_X0(       0x1E, GetSystemTick);
	registerSvc_ret_X0_X1(    0x1F, ConnectToPort, IX1);
	registerSvc_ret_X0(       0x21, SendSyncRequest, (ghandle) IX0);
	registerSvc_ret_X0(       0x22, SendSyncRequestEx, IX0, IX1, (ghandle) IX2);
	registerSvc_ret_X0_X1(    0x24, GetProcessID, (ghandle) IX1);
	registerSvc_ret_X0_X1(    0x25, GetThreadId);
	registerSvc_ret_X0(       0x26, Break, IX0, IX1, IX2);
	registerSvc_ret_X0(       0x27, OutputDebugString, IX0, IX1);
	registerSvc_ret_X0_X1(    0x29, GetInfo, IX1, (ghandle) IX2, IX3);
	registerSvc_ret_X0(       0x2C, MapPhysicalMemory, IX0, IX1);
	registerSvc_ret_X0(       0x2D, UnmapPhysicalMemory, IX0, IX1);
	registerSvc_ret_X0_X1_X2( 0x40, CreateSession, (ghandle) IX0, (ghandle) IX1, IX2);
	registerSvc_ret_X0_X1(    0x41, AcceptSession, (ghandle) IX1);
	registerSvc_ret_X0_X1(    0x43, ReplyAndReceive, IX1, IX2, (ghandle) IX3, IX4);
	registerSvc_ret_X0_X1_X2( 0x45, CreateEvent, (ghandle) IX0, (ghandle) IX1, IX2);
	registerSvc_ret_X0_X1(    0x4E, ReadWriteRegister, IX1, IX2, IX3);
	registerSvc_ret_X0_X1(    0x50, CreateMemoryBlock, IX1, IX2);
	registerSvc_ret_X0(       0x51, MapTransferMemory, (ghandle) IX0, IX1, IX2, IX3);
	registerSvc_ret_X0(       0x52, UnmapTransferMemory, (ghandle) IX0, IX1, IX2);
	registerSvc_ret_X0_X1(    0x53, CreateInterruptEvent, IX1);
	registerSvc_ret_X0_X1(    0x55, QueryIoMapping, IX1, IX2);
	registerSvc_ret_X0_X1(    0x56, CreateDeviceAddressSpace, IX1, IX2);
	registerSvc_ret_X0(       0x57, AttachDeviceAddressSpace, IX0, (ghandle) IX1);
	registerSvc_ret_X0(       0x59, MapDeviceAddressSpaceByForce, (ghandle) IX0, (ghandle) IX1, IX2, IX3, IX4, IX5);
	registerSvc_ret_X0(       0x5c, UnmapDeviceAddressSpace, IX0, (ghandle) IX1, IX2, IX3, IX4);
	registerSvc_ret_X0(       0x74, MapProcessMemory, IX0, (ghandle) IX1, IX2, IX3);
	registerSvc_ret_X0(       0x75, UnmapProcessMemory, IX0, (ghandle) IX1, IX2, IX3);
	registerSvc_ret_X0(       0x77, MapProcessCodeMemory, (ghandle) IX0, IX1, IX2, IX3);
	registerSvc_ret_X0(       0x78, UnmapProcessCodeMemory, (ghandle) IX0, IX1, IX2, IX3);
}

tuple<guint, guint> Svc::SetHeapSize(guint size) {
	LOG_DEBUG(Svc[0x01], "SetHeapSize 0x" LONGFMT, size);
	if (ctu->heapsize < size) {
		ctu->cpu.map(0xaa0000000 + ctu->heapsize, size - ctu->heapsize);
	} else if (ctu->heapsize > size) {
		ctu->cpu.unmap(0xaa0000000 + size, ctu->heapsize - size);
	}
	ctu->heapsize = size;
	return make_tuple(0, 0xaa0000000);
}

guint Svc::SetMemoryAttribute(gptr addr, guint size, guint state0, guint state1) {
	LOG_DEBUG(Svc[0x03], "SetMemoryAttribute 0x" ADDRFMT " 0x" LONGFMT " -> 0x" LONGFMT " 0x" LONGFMT, addr, size, state0, state1);
	return 0;
}

guint Svc::MirrorStack(gptr dest, gptr src, guint size) {
	LOG_DEBUG(Svc[0x04], "MirrorStack 0x" ADDRFMT " 0x" ADDRFMT " - 0x" LONGFMT, dest, src, size);
	ctu->cpu.map(dest, size);
	auto temp = new uint8_t[size];
	ctu->cpu.readmem(src, temp, size);
	ctu->cpu.writemem(dest, temp, size);
	delete[] temp;
	return 0;
}

guint Svc::UnmapMemory(gptr dest, gptr src, guint size) {
	LOG_DEBUG(Svc[0x05], "UnmapMemory 0x" ADDRFMT " 0x" ADDRFMT " - 0x" LONGFMT, dest, src, size);
	ctu->cpu.unmap(dest, size);
	return 0;
}

typedef struct {
	gptr begin;
	gptr size;
	uint32_t memory_type;
	uint32_t memory_attribute;
	uint32_t permission;
	uint32_t device_ref_count;
	uint32_t ipc_ref_count;
	uint32_t padding;
} MemInfo;

tuple<guint, guint> Svc::QueryMemory(gptr meminfo, gptr pageinfo, gptr addr) {
	LOG_DEBUG(Svc[0x06], "QueryMemory 0x" ADDRFMT, addr);
	for(auto [begin, end, perm] : ctu->cpu.regions()) {
		if(begin <= addr && addr <= end) {
			LOG_DEBUG(Svc[0x06], "Found region at 0x" ADDRFMT "-0x" ADDRFMT, begin, end);
			MemInfo minfo;
			minfo.begin = begin;
			minfo.size = end - begin + 1;
			minfo.memory_type = perm == -1 ? 0 : 3; // FREE or CODE
			minfo.memory_attribute = 0;
			if(addr >= 0xaa0000000 && addr < 0xaa0000000 + ctu->heapsize) {
				minfo.memory_type = 5; // HEAP
			}
			minfo.permission = 0;

			if(perm != -1) {
				auto offset = *ctu->cpu.guestptr<uint32_t>(begin + 4);
				if(begin + offset + 4 < end && *ctu->cpu.guestptr<uint32_t>(begin + offset) == FOURCC('M', 'O', 'D', '0'))
					minfo.permission = 5;
				else
					minfo.permission = 3;
			}
			ctu->cpu.guestptr<MemInfo>(meminfo) = minfo;
			break;
		}
	}
	return make_tuple(0, 0);
}

// the nintendo svc probably doesn't take an exitCode,
// but this makes it easier to return values from
// libtransistor tests.
void Svc::ExitProcess(guint exitCode) {
	LOG_DEBUG(Svc[0x07], "ExitProcess 0x" LONGFMT, exitCode);
	exit((int) exitCode);
}

tuple<guint, guint> Svc::CreateThread(guint pc, guint x0, guint sp, guint prio, guint proc) {
	LOG_DEBUG(Svc[0x08], "CreateThread pc=0x" LONGFMT " X0=0x" LONGFMT " SP=0x" LONGFMT, pc, x0, sp);
	auto thread = ctu->tm.create(pc, sp);
	thread->regs.X0 = x0;
	return make_tuple(0, thread->handle);
}

guint Svc::StartThread(ghandle handle) {
	LOG_DEBUG(Svc[0x09], "StartThread 0x%x", handle);
	auto thread = ctu->getHandle<Thread>(handle);
	thread->resume();
	return 0;
}

void Svc::ExitThread() {
	LOG_DEBUG(Svc[0x0A], "ExitThread");
	ctu->tm.current()->terminate();
}

guint Svc::SleepThread(guint ns) {
	LOG_DEBUG(Svc[0x0B], "SleepThread 0x" LONGFMT " ns", ns);

	auto thread = ctu->tm.current();
	// Yield, at least.
	thread->suspend([=] {
		thread->resume([=] {
				thread->regs.X0 = 0;
		});
	});
	return 0;
}

tuple<guint, guint> Svc::GetThreadPriority(ghandle handle) {
	LOG_DEBUG(Svc[0x0C], "GetThreadPriority 0x%x", handle);
	return make_tuple(0, 0);
}

guint Svc::SetThreadPriority(ghandle handle, guint priority) {
	LOG_DEBUG(Svc[0x0D], "SetThreadPriority 0x%x -> 0x" LONGFMT, handle, priority);
	return 0;
}

tuple<guint, guint, guint> Svc::GetThreadCoreMask(guint tmp) {
	LOG_DEBUG(Svc[0x0E], "GetThreadCoreMask");
	return make_tuple(0, 0xFF, 0xFF);
}

guint Svc::SetThreadCoreMask(guint tmp) {
	LOG_DEBUG(Svc[0x0F], "GetThreadCoreMask");
	return 0;
}

guint Svc::GetCurrentProcessorNumber(guint tmp) {
	LOG_DEBUG(Svc[0x10], "GetCurrentProcessorNumber");
	return 0;
}

guint Svc::SignalEvent(ghandle handle) {
	LOG_DEBUG(Svc[0x11], "SignalEvent 0x%x", handle);
	UNIMPLEMENTED(0x11);
	return 0;
}

guint Svc::ClearEvent(ghandle handle) {
	LOG_DEBUG(Svc[0x12], "ClearEvent 0x%x", handle);
	return 0;
}

guint Svc::MapMemoryBlock(ghandle handle, gptr addr, guint size, guint perm) {
	LOG_DEBUG(Svc[0x13], "MapMemoryBlock 0x%x 0x" LONGFMT " 0x" LONGFMT " 0x" LONGFMT, handle, addr, size, perm);
	auto obj = ctu->getHandle<MemoryBlock>(handle);
	assert(obj->size == size);
	assert(obj->addr == 0);
	ctu->cpu.map(addr, size);
	obj->addr = addr;
	return 0;
}

tuple<guint, guint> Svc::CreateTransferMemory(gptr addr, guint size, guint perm) {
	LOG_DEBUG(Svc[0x15], "CreateTransferMemory 0x" LONGFMT " 0x" LONGFMT " 0x" LONGFMT, addr, size, perm);
	auto tm = make_shared<MemoryBlock>(size, perm);
	tm->addr = addr;
	return make_tuple(0, ctu->newHandle(tm));
}

guint Svc::CloseHandle(ghandle handle) {
	LOG_DEBUG(Svc[0x16], "CloseHandle 0x%x", handle);
	ctu->deleteHandle(handle);
	return 0;
}

guint Svc::ResetSignal(ghandle handle) {
	LOG_DEBUG(Svc[0x17], "ResetSignal 0x%x", handle);
	UNIMPLEMENTED(0x17);
	return 0;
}

tuple<guint, guint> Svc::WaitSynchronization(gptr handles, guint numHandles, guint timeout) {
	LOG_DEBUG(Svc[0x18], "WaitSynchronization %u", (uint) numHandles);

	auto thread = ctu->tm.current();
	thread->suspend([=] {
		auto triggered = make_shared<bool>(false);
		auto cb = [=](int i, bool canceled) {
			if(*triggered)
				return -1;
			*triggered = true;
			thread->resume([=] {
				if(canceled) {
					thread->regs.X0 = 0xec01;
					thread->regs.X1 = 0;
				} else {
					thread->regs.X0 = 0;
					thread->regs.X1 = i;
				}
			});
			return 1;
		};
		auto hdls = ctu->cpu.guestptr<ghandle>(handles);
		for(auto i = 0; i < numHandles; ++i) {
			auto hnd = ctu->getHandle<Waitable>(hdls[i]);
			auto port = dynamic_pointer_cast<NPort>(hnd);
			if(port != nullptr)
				LOG_DEBUG(Svc[0x18], "Waiting on port %s", port->name.c_str());
			hnd->wait([=](auto canceled) { return cb(i, canceled); });
		}
	});

	return make_tuple(0, 0);
}

guint Svc::CancelSynchronization(ghandle handle) {
	LOG_DEBUG(Svc[0x19], "CancelSynchronization 0x%x", handle);
	UNIMPLEMENTED(0x19);
	return 0;
}

shared_ptr<Mutex> Svc::ensureMutex(gptr mutexAddr) {
	if(mutexes.find(mutexAddr) == mutexes.end()) {
		LOG_DEBUG(Svc, "Making new mutex for 0x" LONGFMT, mutexAddr);
		mutexes[mutexAddr] = make_shared<Mutex>(ctu->cpu.guestptr<uint32_t>(mutexAddr));
	}
	return mutexes[mutexAddr];
}

guint Svc::LockMutex(ghandle curthread, gptr mutexAddr, ghandle reqthread) {
	LOG_DEBUG(Svc[0x1A], "LockMutex 0x%x 0x" LONGFMT " 0x%x", curthread, mutexAddr, reqthread);

	auto mutex = ensureMutex(mutexAddr);
	auto owner = mutex->owner();

	auto thread = ctu->getHandle<Thread>(reqthread);

	if(owner != 0 && owner != reqthread) {
		LOG_DEBUG(Svc[0x1A], "Could not get mutex lock. Waiting.");
		mutex->hasWaiters(true);
		thread->suspend([=] {
			mutex->wait([=] {
				if(mutex->owner() == 0) {
					mutex->owner(reqthread);
					thread->resume([=] {
						thread->regs.X0 = 0;
					});
					return 1;
				} else
					return 0;
			});
		});
	} else {
		mutex->owner(reqthread);
		if(!thread->active) {
			thread->regs.X0 = 0;
			thread->resume();
		}
	}
	return 0;
}

void Svc::UnlockMutex(gptr mutexAddr) {
	LOG_DEBUG(Svc[0x1B], "UnlockMutex 0x" ADDRFMT, mutexAddr);

	auto mutex = ensureMutex(mutexAddr);
	auto owner = mutex->owner();
	assert(owner == 0 || owner == ctu->tm.current()->handle);

	mutex->guestRelease();
}

shared_ptr<Semaphore> Svc::ensureSemaphore(gptr semaAddr) {
	if(semaphores.find(semaAddr) == semaphores.end()) {
		LOG_DEBUG(Svc, "Making new semaphore for 0x" LONGFMT, semaAddr);
		semaphores[semaAddr] = make_shared<Semaphore>(ctu->cpu.guestptr<uint32_t>(semaAddr));
	}
	return semaphores[semaAddr];
}

guint Svc::WaitProcessWideKeyAtomic(gptr mutexAddr, gptr semaAddr, ghandle threadHandle, guint timeout) {
	LOG_DEBUG(Svc[0x1C], "WaitProcessWideKeyAtomic 0x" LONGFMT " 0x" LONGFMT " 0x%x 0x" LONGFMT, mutexAddr, semaAddr, threadHandle, timeout);

	auto mutex = ensureMutex(mutexAddr);
	auto semaphore = ensureSemaphore(semaAddr);

	assert(mutex->owner() == threadHandle);

	if(semaphore->value() > 0) {
		semaphore->decrement();
		return 0;
	}

	auto thread = ctu->getHandle<Thread>(threadHandle);
	thread->suspend([=] {
		semaphore->wait([=] {
			semaphore->decrement();
			thread->resume([=] {
				thread->regs.X0 = LockMutex(0, mutexAddr, threadHandle);
			});
			return 1;
		});
	});

	mutex->guestRelease();
	return 0;
}

guint Svc::SignalProcessWideKey(gptr semaAddr, guint target) {
	LOG_DEBUG(Svc[0x1D], "SignalProcessWideKey 0x" LONGFMT " 0x" LONGFMT, semaAddr, target);
	auto semaphore = ensureSemaphore(semaAddr);
	semaphore->increment();
	if(target == 1)
		semaphore->signal(true);
	else if(target == 0xffffffff)
		semaphore->signal();
	return 0;
}

guint Svc::GetSystemTick() {
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return (((uint64_t) time.tv_sec) * 19200000) + ((((uint64_t) time.tv_nsec) * 192) / 10000);
}

tuple<guint, ghandle> Svc::ConnectToPort(guint name) {
	LOG_DEBUG(Svc[0x1F], "ConnectToPort");
	return make_tuple(0, ctu->ipc.ConnectToPort(ctu->cpu.readstring(name)));
}

guint Svc::SendSyncRequest(ghandle handle) {
	LOG_DEBUG(Svc[0x21], "SendSyncRequest 0x%x", handle);
	auto thread = ctu->tm.current();
	auto hnd = ctu->getHandle<IPipe>(handle);
	auto buf = make_shared<array<uint8_t, 0x100>>();
	ctu->cpu.readmem(thread->tlsBase, buf->data(), 0x100);
	if(hnd->isAsync()) {
		thread->suspend([=] {
			hnd->messageAsync(buf, [=](auto res, auto close) {
				ctu->cpu.writemem(thread->tlsBase, buf->data(), 0x100);
				if(close)
					ctu->deleteHandle(handle);
				thread->resume([=] {
					thread->regs.X0 = res;
				});
			});
		});
		return 0;
	} else {
		auto close = false;
		auto ret = hnd->messageSync(buf, close);
		if(close)
			ctu->deleteHandle(handle);
		ctu->cpu.writemem(thread->tlsBase, buf->data(), 0x100);
		return ret;
	}
}

guint Svc::SendSyncRequestEx(gptr buf, guint size, ghandle handle) {
	LOG_ERROR(Svc[0x22], "SendSyncRequestEx not implemented");
	UNIMPLEMENTED(0x22);
	return 0xf601;
}

tuple<guint, guint> Svc::GetProcessID(ghandle handle) {
	LOG_DEBUG(Svc[0x24], "GetProcessID 0x%x", handle);
	return make_tuple(0, 0);
}

tuple<guint, guint> Svc::GetThreadId() {
	LOG_DEBUG(Svc[0x25], "GetThreadId");
	return make_tuple(0, ctu->tm.current()->id);
}

guint Svc::Break(guint X0, guint X1, guint info) {
	LOG_DEBUG(Svc[0x26], "Break");
	exit(1);
}

guint Svc::OutputDebugString(guint ptr, guint size) {
	//LOG_DEBUG(Svc[0x27], "OutputDebugString addr=" ADDRFMT " size=" LONGFMT, ptr, size);
	if(size > 0x2000) {
		LOG_DEBUG(Svc[0x27], "Debug string is bigger than 0x2000 bytes. Something probably went wrong.");
	}
	char *debugStr = (char*) malloc(size + 1);
	memset(debugStr, 0, size+1);
	if(ctu->cpu.readmem(ptr, debugStr, size)) {
		LOG_DEBUG(Svc[0x27], "Debug String: %s", debugStr);
	} else {
		LOG_DEBUG(Svc[0x27], "Could not access memory at " ADDRFMT, ptr);
	}
	free(debugStr);
	return 0;
}

#define matchone(a, v) do { if(id1 == (a)) return make_tuple(0, (v)); } while(0)
#define matchpair(a, b, v) do { if(id1 == (a) && id2 == (b)) return make_tuple(0, (v)); } while(0)

tuple<guint, guint> Svc::GetInfo(guint id1, ghandle handle, guint id2) {
	LOG_DEBUG(Svc[0x29], "GetInfo handle=0x%x id1=0x" LONGFMT " id2=" LONGFMT, handle, id1, id2);
	matchpair(0, 0, 0xF);
	matchpair(1, 0, 0xFFFFFFFF00000000);
	matchpair(2, 0, 0xbb0000000); // map region
	matchpair(3, 0, 0x1000000000); // size
	matchpair(4, 0, 0xaa0000000); // heap region
	matchpair(5, 0, ctu->heapsize); // size
	matchpair(6, 0, 0x400000);
	matchpair(7, 0, 0x10000);
	matchpair(12, 0, 0x8000000);
	matchpair(13, 0, 0x7ff8000000);
	matchpair(14, 0, 0xbb0000000); // new map region
	matchpair(15, 0, 0x1000000000); // size
	matchpair(18, 0, 0x0100000000000036); // Title ID
	matchone(11, 0);

	return make_tuple(0xf001, 0);
}

guint Svc::MapPhysicalMemory(gptr addr, guint size) {
	LOG_DEBUG(Svc[0x2C], "MapPhysicalMemory(0x" LONGFMT ", 0x" LONGFMT ")", addr, size);
	ctu->cpu.map(addr, size);
	return 0;
}

guint Svc::UnmapPhysicalMemory(gptr addr, guint size) {
	ctu->cpu.unmap(addr, size);
	return 0;
}

tuple<guint, guint, guint> Svc::CreateSession(ghandle clientOut, ghandle serverOut, guint unk) {
	LOG_DEBUG(Svc[0x40], "CreateSession");
	auto pipe = make_shared<NPipe>(ctu, "Session");
	return make_tuple(0, ctu->newHandle(pipe), ctu->newHandle(pipe));
}

tuple<guint, guint> Svc::AcceptSession(ghandle hnd) {
	LOG_DEBUG(Svc[0x41], "AcceptSession 0x%x", hnd);
	auto pipe = ctu->getHandle<NPort>(hnd)->accept();
	return make_tuple(0, ctu->newHandle(pipe));
}

tuple<guint, guint> Svc::ReplyAndReceive(gptr handles, guint numHandles, ghandle replySession, guint timeout) {
	LOG_DEBUG(Svc[0x43], "ReplyAndReceive");

	auto thread = ctu->tm.current();
	if(replySession != 0) {
		LOG_DEBUG(Svc[0x43], "Sending message back from native service");
		auto hnd = ctu->getHandle<NPipe>(replySession);
		auto buf = make_shared<array<uint8_t, 0x100>>();
		ctu->cpu.readmem(thread->tlsBase, buf->data(), 0x100);
		hnd->client.push(buf);
		hnd->client.signal(true);
	}

	if(numHandles == 0)
		return make_tuple(0xf601, 0);

	assert(numHandles == 1);
	auto handle = *ctu->cpu.guestptr<ghandle>(handles);
	auto hnd = ctu->getHandle<NPipe>(handle);

	auto buf = hnd->server.pop();
	if(buf == nullptr) {
		ctu->deleteHandle(handle);
		return make_tuple(0xf601, 0);
	}
	LOG_DEBUG(Svc[0x43], "Got message for native service");
	hexdump(buf);
	ctu->cpu.writemem(thread->tlsBase, buf->data(), 0x100);

	return make_tuple(0, 0);
}

tuple<guint, guint, guint> Svc::CreateEvent(ghandle clientOut, ghandle serverOut, guint unk) {
	LOG_DEBUG(Svc[0x45], "CreateEvent");
	return make_tuple(0, ctu->newHandle(make_shared<Waitable>()), ctu->newHandle(make_shared<Waitable>()));
}

tuple<guint, guint> Svc::ReadWriteRegister(guint reg, guint rwm, guint val) {
	LOG_DEBUG(Svc[0x4E], "ReadWriteRegister reg=" ADDRFMT " rwm=" LONGFMT " val=" LONGFMT, reg, rwm, val);
	return make_tuple(0, 0);
}

tuple<guint, guint> Svc::CreateMemoryBlock(guint size, guint perm) {
	LOG_DEBUG(Svc[0x50], "CreateMemoryBlock");
	return make_tuple(0, ctu->newHandle(make_shared<MemoryBlock>(size, perm)));
}

guint Svc::MapTransferMemory(ghandle handle, gptr addr, guint size, guint perm) {
	LOG_DEBUG(Svc[0x51], "MapTransferMemory");
	ctu->cpu.map(addr, size);
	return 0;
}

guint Svc::UnmapTransferMemory(ghandle handle, gptr addr, guint size) {
	LOG_DEBUG(Svc[0x52], "UnmapTransferMemory");
	ctu->cpu.unmap(addr, size);
	return 0;
}

tuple<guint, guint> Svc::CreateInterruptEvent(guint irq) {
	LOG_DEBUG(Svc[0x53], "CreateInterruptEvent irq=" LONGFMT, irq);
	return make_tuple(0, ctu->newHandle(make_shared<InstantWaitable>()));
}

tuple<guint, guint> Svc::QueryIoMapping(gptr physaddr, guint size) {
	LOG_DEBUG(Svc[0x55], "QueryIoMapping " ADDRFMT " size " LONGFMT, physaddr, size);
	gptr addr = ctu->mmiohandler.getVirtualAddressFromAddr(physaddr);
	LOG_DEBUG(Svc[0x55], ADDRFMT, addr);
	if(addr == 0x0) { // force exit for now
		cout << "!Unknown physical address!" << endl;
		exit(1);
	}
	return make_tuple(0x0, addr);
}

class DeviceMemory : public KObject {
public:
	DeviceMemory(gptr _start, gptr _end) : start(_start), end(_end) {
	}

	gptr start, end;
	list<guint> devices;
};

tuple<guint, guint> Svc::CreateDeviceAddressSpace(gptr start, gptr end) {
	LOG_DEBUG(Svc[0x56], "CreateDeviceAddressSpace start=" ADDRFMT " end=" ADDRFMT, start, end);
	auto obj = make_shared<DeviceMemory>(start, end);
	return make_tuple(0, ctu->newHandle(obj));
}

guint Svc::AttachDeviceAddressSpace(guint dev, ghandle handle) {
	LOG_DEBUG(Svc[0x57], "AttachDeviceAddressSpace dev=" LONGFMT " handle=%x", dev, handle);
	auto obj = ctu->getHandle<DeviceMemory>(handle);
	obj->devices.push_back(dev);
	return 0;
}

guint Svc::MapDeviceAddressSpaceByForce(ghandle handle, ghandle phandle, gptr vaddr, guint size, gptr saddr, guint perm) {
	LOG_DEBUG(Svc[0x59], "MapDeviceAddressSpaceByForce handle=%x phandle=%x vaddr=" ADDRFMT " size=" LONGFMT " saddr=" ADDRFMT " perm=" LONGFMT, handle, phandle, vaddr, size, saddr, perm);
	return 0;
}

guint Svc::UnmapDeviceAddressSpace(guint unk0, ghandle phandle, gptr maddr, guint size, gptr paddr) {
	LOG_DEBUG(Svc[0x5c], "UnmapDeviceAddressSpace");
	UNIMPLEMENTED(0x5c);
	return 0;
}

guint Svc::MapProcessMemory(gptr dstaddr, ghandle handle, gptr srcaddr, guint size) {
	LOG_DEBUG(Svc[0x74], "MapProcessMemory");
	ctu->cpu.map(dstaddr, size);
	char *mem = (char *) malloc(size);
	memset(mem, 0, size);
	ctu->cpu.readmem(srcaddr, mem, size);
	ctu->cpu.writemem(dstaddr, mem, size);
	free(mem);
	return 0;
}

guint Svc::UnmapProcessMemory(gptr dstaddr, ghandle handle, gptr srcaddr, guint size) {
	LOG_DEBUG(Svc[0x75], "UnmapProcessMemory");
	ctu->cpu.unmap(dstaddr, size);
	return 0;
}

guint Svc::MapProcessCodeMemory(ghandle handle, gptr dstaddr, gptr srcaddr, guint size) {
	LOG_DEBUG(Svc[0x77], "MapProcessCodeMemory");
	ctu->cpu.map(dstaddr, size);
	return 0;
}

guint Svc::UnmapProcessCodeMemory(ghandle handle, gptr dstaddr, gptr srcaddr, guint size) {
	LOG_DEBUG(Svc[0x78], "UnmapProcessCodeMemory");
	ctu->cpu.unmap(dstaddr, size);
	return 0;
}
