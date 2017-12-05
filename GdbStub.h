// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

// Originally written by Sven Peter <sven@fail0verflow.com> for anergistic.
// Integrated into Mephisto/CTUv2 by Cody Brocious

#pragma once

#include "Ctu.h"

#define GDB_BUFFER_SIZE 10000

struct BreakpointAddress {
	gptr address;
	BreakpointType type;
};

struct Breakpoint {
	bool active;
	gptr addr;
	guint len;
	hook_t hook;
};

class GdbStub {
public:
	GdbStub(Ctu *_ctu);
	void enable(uint16_t port);
	void _break(bool is_memory_break = false);
	bool isMemoryBreak();
	void handlePacket();
	auto getNextBreakpointFromAddress(gptr addr, BreakpointType type);
	bool checkBreakpoint(gptr addr, BreakpointType type);
	void sendSignal(uint32_t signal);

	bool memoryBreak, haltLoop, stepLoop, remoteBreak, enabled;

private:
	auto& getBreakpointList(BreakpointType type);
	void removeBreakpoint(BreakpointType type, gptr addr);
	uint8_t readByte();
	void sendPacket(const char packet);
	void sendReply(const char* reply);
	void handleQuery();
	void handleSetThread();
	void readCommand();
	bool isDataAvailable();
	void readRegister();
	void readRegisters();
	void writeRegister();
	void writeRegisters();
	void readMemory();
	void writeMemory();
	void isThreadAlive();
	void step();
	void _continue();
	bool commitBreakpoint(BreakpointType type, gptr addr, uint32_t len);
	void addBreakpoint();
	void removeBreakpoint();

	guint reg(int x);
	void reg(int x, guint v);

	Ctu *ctu;

	map<gptr, Breakpoint> breakpointsExecute;
	map<gptr, Breakpoint> breakpointsRead;
	map<gptr, Breakpoint> breakpointsWrite;

	int client;

	uint8_t commandBuffer[GDB_BUFFER_SIZE];
	uint32_t commandLength;

	uint32_t latestSignal;
};
