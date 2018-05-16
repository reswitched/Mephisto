#pragma once

#include "Ctu.h"

class MemoryBlock : public KObject {
public:
	MemoryBlock(guint _size, guint _perms) : size(_size), perms(_perms), addr(0) {}

	guint size, perms;
	gptr addr;
};

class Svc {
public:
	Svc(Ctu *ctu);

private:
	Ctu *ctu;
	unordered_map<gptr, shared_ptr<Semaphore>> semaphores;
	unordered_map<gptr, shared_ptr<Mutex>> mutexes;

	shared_ptr<Mutex> ensureMutex(gptr mutexAddr);
	shared_ptr<Semaphore> ensureSemaphore(gptr semaAddr);

	tuple<guint, guint> SetHeapSize(guint size); // 0x01
	guint SetMemoryAttribute(gptr addr, guint size, guint state0, guint state1); // 0x03
	guint MirrorStack(gptr dest, gptr src, guint size); // 0x04
	guint UnmapMemory(gptr dest, gptr src, guint size); // 0x05
	tuple<guint, guint> QueryMemory(gptr meminfo, gptr pageinfo, gptr addr); // 0x06
	void ExitProcess(guint exitCode); // 0x07
	tuple<guint, guint> CreateThread(guint pc, guint x0, guint sp, guint prio, guint proc); // 0x08
	guint StartThread(ghandle handle); // 0x09
	void ExitThread(); // 0x0A
	guint SleepThread(guint ns); // 0x0B
	tuple<guint, guint> GetThreadPriority(ghandle handle); // 0x0C
	guint SetThreadPriority(ghandle handle, guint priority); // 0x0D
	tuple<guint, guint, guint> GetThreadCoreMask(guint); // 0x0E
	guint SetThreadCoreMask(guint); // 0x0F
	guint GetCurrentProcessorNumber(guint); // 0x10
	guint SignalEvent(ghandle handle); // 0x11
	guint ClearEvent(ghandle handle); // 0x12
	guint MapMemoryBlock(ghandle handle, gptr addr, guint size, guint perm); // 0x13
	tuple<guint, guint> CreateTransferMemory(gptr addr, guint size, guint perm); // 0x15
	guint CloseHandle(ghandle handle); // 0x16
	guint ResetSignal(ghandle handle); // 0x17
	tuple<guint, guint> WaitSynchronization(gptr handles, guint numHandles, guint timeout); // 0x18
	guint CancelSynchronization(ghandle handle); // 0x19
	guint LockMutex(ghandle curthread, gptr mutexAddr, ghandle reqthread); // 0x1A
	void UnlockMutex(gptr mutexAddr); // 0x1B
	guint WaitProcessWideKeyAtomic(gptr mutexAddr, gptr semaAddr, ghandle threadHandle, guint timeout); // 0x1C
	guint SignalProcessWideKey(gptr semaAddr, guint target); // 0x1D
	guint GetSystemTick(); // 0x1E
	tuple<guint, ghandle> ConnectToPort(guint name); // 0x1F
	guint SendSyncRequest(ghandle handle); // 0x21
	guint SendSyncRequestEx(gptr buf, guint size, ghandle handle); // 0x22
	tuple<guint, guint> GetProcessID(ghandle handle); // 0x24
	tuple<guint, guint> GetThreadId(); // 0x25
	guint Break(guint X0, guint X1, guint info); // 0x26
	guint OutputDebugString(guint ptr, guint size); // 0x27
	tuple<guint, guint> GetInfo(guint id1, ghandle handle, guint id2); // 0x29
	guint MapPhysicalMemory(gptr addr, guint size); // 0x2C
	guint UnmapPhysicalMemory(gptr addr, guint size); // 0x2D
	tuple<guint, guint, guint> CreateSession(ghandle clientOut, ghandle serverOut, guint unk); // 0x40
	tuple<guint, guint> AcceptSession(ghandle port); // 0x41
	tuple<guint, guint> ReplyAndReceive(gptr handles, guint numHandles, ghandle replySession, guint timeout); // 0x43
	tuple<guint, guint, guint> CreateEvent(ghandle clientOut, ghandle serverOut, guint unk); // 0x45
	tuple<guint, guint> ReadWriteRegister(guint reg, guint rwm, guint val); // 0x4E
	tuple<guint, guint> CreateMemoryBlock(guint size, guint perm); // 0x50
	guint MapTransferMemory(ghandle handle, gptr addr, guint size, guint perm); // 0x51
	guint UnmapTransferMemory(ghandle handle, gptr addr, guint size); // 0x52
	tuple<guint, guint> CreateInterruptEvent(guint irq); // 0x53
	tuple<guint, guint> QueryIoMapping(gptr physaddr, guint size); // 0x55
	tuple<guint, guint> CreateDeviceAddressSpace(gptr start, gptr end); // 0x56
	guint AttachDeviceAddressSpace(guint dev, ghandle handle); // 0x57
	guint MapDeviceAddressSpaceByForce(ghandle handle, ghandle phandle, gptr vaddr, guint size, gptr saddr, guint perm); // 0x59
	guint UnmapDeviceAddressSpace(guint unk0, ghandle phandle, gptr maddr, guint size, gptr paddr); // 0x5c
	guint MapProcessMemory(gptr dstaddr, ghandle handle, gptr srcaddr, guint size); // 0x74
	guint UnmapProcessMemory(gptr dstaddr, ghandle handle, gptr srcaddr, guint size); // 0x75
	guint MapProcessCodeMemory(ghandle handle, gptr dstaddr, gptr srcaddr, guint size); // 0x77
	guint UnmapProcessCodeMemory(ghandle handle, gptr dstaddr, gptr srcaddr, guint size); // 0x78
};
