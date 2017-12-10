// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

// Originally written by Sven Peter <sven@fail0verflow.com> for anergistic.
// Integrated into Mephisto/CTUv2 by Cody Brocious

#include "Ctu.h"

const char GDB_STUB_START = '$';
const char GDB_STUB_END = '#';
const char GDB_STUB_ACK = '+';
const char GDB_STUB_NACK = '-';

#ifndef SIGTRAP
const uint32_t SIGTRAP = 5;
#endif

#ifndef SIGTERM
const uint32_t SIGTERM = 15;
#endif

#ifndef MSG_WAITALL
const uint32_t MSG_WAITALL = 8;
#endif

// For sample XML files see the GDB source /gdb/features
// GDB also wants the l character at the start
// This XML defines what the registers are for this specific ARM device
static const char* target_xml =
	R"(<?xml version="1.0"?>
<!DOCTYPE target SYSTEM "gdb-target.dtd">
<target version="1.0">
	<feature name="org.gnu.gdb.aarch64.core">
		<reg name="x0" bitsize="64"/>
		<reg name="x1" bitsize="64"/>
		<reg name="x2" bitsize="64"/>
		<reg name="x3" bitsize="64"/>
		<reg name="x4" bitsize="64"/>
		<reg name="x5" bitsize="64"/>
		<reg name="x6" bitsize="64"/>
		<reg name="x7" bitsize="64"/>
		<reg name="x8" bitsize="64"/>
		<reg name="x9" bitsize="64"/>
		<reg name="x10" bitsize="64"/>
		<reg name="x11" bitsize="64"/>
		<reg name="x12" bitsize="64"/>
		<reg name="x13" bitsize="64"/>
		<reg name="x14" bitsize="64"/>
		<reg name="x15" bitsize="64"/>
		<reg name="x16" bitsize="64"/>
		<reg name="x17" bitsize="64"/>
		<reg name="x18" bitsize="64"/>
		<reg name="x19" bitsize="64"/>
		<reg name="x20" bitsize="64"/>
		<reg name="x21" bitsize="64"/>
		<reg name="x22" bitsize="64"/>
		<reg name="x23" bitsize="64"/>
		<reg name="x24" bitsize="64"/>
		<reg name="x25" bitsize="64"/>
		<reg name="x26" bitsize="64"/>
		<reg name="x27" bitsize="64"/>
		<reg name="x28" bitsize="64"/>
		<reg name="x29" bitsize="64"/>
		<reg name="x30" bitsize="64"/>
		<reg name="sp" bitsize="64" type="data_ptr"/>
		<reg name="pc" bitsize="64" type="code_ptr"/>
		<reg name="cpsr" bitsize="32"/>
	</feature>
</target>)";

uint8_t hexCharToValue(uint8_t hex) {
	if(hex >= '0' && hex <= '9')
		return hex - '0';
	else if(hex >= 'a' && hex <= 'f')
		return hex - 'a' + 0xA;
	else if(hex >= 'A' && hex <= 'F')
		return hex - 'A' + 0xA;

	LOG_ERROR(GdbStub, "Invalid nibble: %c (%02x)", hex, hex);
}

uint8_t nibbleToHex(uint8_t n) {
	n &= 0xF;
	if(n < 0xA)
		return '0' + n;
	else
		return 'a' + n - 0xA;
}

uint64_t hexToInt(const uint8_t* src, size_t len) {
	uint64_t output = 0;
	while(len-- > 0) {
		output = (output << 4) | hexCharToValue(src[0]);
		src++;
	}
	return output;
}

void memToGdbHex(uint8_t* dest, const uint8_t* src, size_t len) {
	while(len-- > 0) {
		auto tmp = *src++;
		*dest++ = nibbleToHex(tmp >> 4);
		*dest++ = nibbleToHex(tmp);
	}
}

void gdbHexToMem(uint8_t* dest, const uint8_t* src, size_t len) {
	while(len-- > 0) {
		*dest++ = (uint8_t) ((hexCharToValue(src[0]) << 4) | hexCharToValue(src[1]));
		src += 2;
	}
}

void intToGdbHex(uint8_t* dest, uint64_t v) {
	for(auto i = 0; i < 16; i += 2) {
		dest[i + 1] = nibbleToHex((uint8_t) (v >> (4 * i)));
		dest[i] = nibbleToHex((uint8_t) (v >> (4 * (i + 1))));
	}
}

uint64_t gdbHexToInt(const uint8_t* src) {
	uint64_t output = 0;

	for(int i = 0; i < 16; i += 2) {
		output = (output << 4) | hexCharToValue(src[15 - i - 1]);
		output = (output << 4) | hexCharToValue(src[15 - i]);
	}

	return output;
}

uint8_t calculateChecksum(const uint8_t* buffer, size_t length) {
	return static_cast<uint8_t>(accumulate(buffer, buffer + length, 0, plus<uint8_t>()));
}

GdbStub::GdbStub(Ctu *_ctu) : ctu(_ctu) {
	memoryBreak = false;
	haltLoop = stepLoop = false;
	enabled = false;
	latestSignal = 0;
}

void GdbStub::enable(uint16_t port) {
	LOG_INFO(GdbStub, "Starting GDB server on port %d...", port);

	sockaddr_in saddr_server = {};
	saddr_server.sin_family = AF_INET;
	saddr_server.sin_port = htons(port);
	saddr_server.sin_addr.s_addr = INADDR_ANY;

	auto tmpsock = socket(PF_INET, SOCK_STREAM, 0);
	if(tmpsock == -1)
		LOG_ERROR(GdbStub, "Failed to create gdb socket");

	auto reuse_enabled = 1;
	if(setsockopt(tmpsock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_enabled, sizeof(reuse_enabled)) < 0)
		LOG_ERROR(GdbStub, "Failed to set gdb socket option");

	auto server_addr = reinterpret_cast<const sockaddr*>(&saddr_server);
	socklen_t server_addrlen = sizeof(saddr_server);
	if(bind(tmpsock, server_addr, server_addrlen) < 0)
		LOG_ERROR(GdbStub, "Failed to bind gdb socket");

	if(listen(tmpsock, 1) < 0)
		LOG_ERROR(GdbStub, "Failed to listen to gdb socket");

	LOG_INFO(GdbStub, "Waiting for gdb to connect...");
	sockaddr_in saddr_client;
	sockaddr* client_addr = reinterpret_cast<sockaddr*>(&saddr_client);
	socklen_t client_addrlen = sizeof(saddr_client);
	client = accept(tmpsock, client_addr, &client_addrlen);
	if(client < 0)
		LOG_ERROR(GdbStub, "Failed to accept gdb client");
	else
		LOG_INFO(GdbStub, "Client connected.");

	enabled = true;
	haltLoop = true;
	remoteBreak = false;
}

uint8_t GdbStub::readByte() {
	uint8_t c;
	auto size = recv(client, reinterpret_cast<char*>(&c), 1, MSG_WAITALL);
	if(size != 1)
		LOG_ERROR(GdbStub, "recv failed : %ld", size);

	return c;
}

guint GdbStub::reg(int x) {
	auto thread = ctu->tm.current();
	if(thread == nullptr)
		thread = ctu->tm.last();
	if(thread == nullptr)
		return 0;
	switch(x) {
	case 31:
		return thread->regs.SP;
	case 32:
		return thread->regs.PC;
	default:
		assert(x < 31);
		return thread->regs.gprs[x];
	}
}

void GdbStub::reg(int x, guint v) {
	auto thread = ctu->tm.current();
	if(thread == nullptr)
		thread = ctu->tm.last();
	if(thread == nullptr)
		return;
	switch(x) {
	case 31:
		thread->regs.SP = v;
		break;
	case 32:
		thread->regs.PC = v;
		break;
	default:
		assert(x < 31);
		thread->regs.gprs[x] = v;
	}
}

auto& GdbStub::getBreakpointList(BreakpointType type) {
	switch(type) {
	case BreakpointType::Execute:
		return breakpointsExecute;
	case BreakpointType::Write:
		return breakpointsWrite;
	case BreakpointType::Read:
	case BreakpointType::Access:
	case BreakpointType::None: // Should never happen
		return breakpointsRead;
	}
}

void GdbStub::removeBreakpoint(BreakpointType type, gptr addr) {
	auto& p = getBreakpointList(type);

	auto bp = p.find(addr);
	if(bp != p.end()) {
		LOG_DEBUG(GdbStub, "gdb: removed a breakpoint: " ADDRFMT " bytes at " ADDRFMT " of type %d",
				  bp->second.len, bp->second.addr, type);
		ctu->cpu.removeBreakpoint(bp->second.hook);
		p.erase(addr);
	}
}

auto GdbStub::getNextBreakpointFromAddress(gptr addr, BreakpointType type) {
	auto& p = getBreakpointList(type);
	auto next_breakpoint = p.lower_bound(addr);
	BreakpointAddress breakpoint;

	if(next_breakpoint != p.end()) {
		breakpoint.address = next_breakpoint->first;
		breakpoint.type = type;
	} else {
		breakpoint.address = 0;
		breakpoint.type = BreakpointType::None;
	}

	return breakpoint;
}

bool GdbStub::checkBreakpoint(gptr addr, BreakpointType type) {
	auto& p = getBreakpointList(type);

	auto bp = p.find(addr);
	if(bp != p.end()) {
		guint len = bp->second.len;

		if(bp->second.active && (addr >= bp->second.addr && addr < bp->second.addr + len)) {
			LOG_DEBUG(GdbStub,
					  "Found breakpoint type %d @ " ADDRFMT ", range: " ADDRFMT " - " ADDRFMT " (%d bytes)", type,
					  addr, bp->second.addr, bp->second.addr + len, (uint32_t) len);
			return true;
		}
	}

	return false;
}

void GdbStub::sendPacket(const char packet) {
	if(send(client, &packet, 1, 0) != 1)
		LOG_ERROR(GdbStub, "send failed");
}

void GdbStub::sendReply(const char* reply) {
	LOG_DEBUG(GdbStub, "Reply: %s", reply);
	memset(commandBuffer, 0, sizeof(commandBuffer));

	commandLength = static_cast<uint32_t>(strlen(reply));
	if(commandLength + 4 > sizeof(commandBuffer)) {
		LOG_DEBUG(GdbStub, "commandBuffer overflow in sendReply");
		return;
	}

	memcpy(commandBuffer + 1, reply, commandLength);

	auto checksum = calculateChecksum(commandBuffer, commandLength + 1);
	commandBuffer[0] = GDB_STUB_START;
	commandBuffer[commandLength + 1] = GDB_STUB_END;
	commandBuffer[commandLength + 2] = nibbleToHex(checksum >> 4);
	commandBuffer[commandLength + 3] = nibbleToHex(checksum);

	auto ptr = commandBuffer;
	auto left = commandLength + 4;
	while(left > 0) {
		auto sent_size = send(client, reinterpret_cast<char*>(ptr), left, 0);
		if(sent_size < 0)
			LOG_ERROR(GdbStub, "gdb: send failed");

		left -= sent_size;
		ptr += sent_size;
	}
}

void GdbStub::handleQuery() {
	LOG_DEBUG(GdbStub, "gdb: query '%s'", commandBuffer + 1);

	auto query = reinterpret_cast<const char*>(commandBuffer + 1);

	if(strcmp(query, "TStatus") == 0)
		sendReply("T0");
	else if(strncmp(query, "Supported", strlen("Supported")) == 0)
		sendReply("PacketSize=1600");
	else if(strncmp(query, "Xfer:features:read:target.xml:",
					   strlen("Xfer:features:read:target.xml:")) == 0)
		sendReply(target_xml);
	else if (strncmp(query, "fThreadInfo", strlen("fThreadInfo")) == 0) {
		auto list = ctu->tm.thread_list();
		char tmp[17] = {0};
		string val = "m";
		for (auto it = list.begin(); it != list.end(); it++) {
			if (!(*it)->started)
				continue;
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%x", (*it)->id);
			val += (char*)tmp;
			val += ",";
		}
		val.pop_back();
		sendReply(val.c_str());
	} else if (strncmp(query, "sThreadInfo", strlen("sThreadInfo")) == 0)
		sendReply("l");
	else
		sendReply("");
}

void GdbStub::handleSetThread() {
	// TODO: allow actually changing threads now :|
	if(memcmp(commandBuffer, "Hg", 2) == 0 || memcmp(commandBuffer, "Hc", 2) == 0) {
		// Get thread id
		if (commandBuffer[2] != '-') {
			int threadid = (int)hexToInt(commandBuffer + 2, strlen((char*)commandBuffer + 2));
			ctu->tm.setCurrent(threadid);
		}
		sendReply("OK");
	} else
		sendReply("E01");
}

auto stringFromFormat(const char* format, ...) {
	char *buf;
	va_list args;
	va_start(args, format);
	if(vasprintf(&buf, format, args) < 0)
		LOG_ERROR(GdbStub, "Unable to allocate memory for string");
	va_end(args);

	string ret = buf;
	free(buf);
	return ret;
}

void GdbStub::sendSignal(uint32_t signal) {
	latestSignal = signal;

	uint8_t sp[16];
	uint8_t pc[16];
	intToGdbHex(sp, reg(31));
	intToGdbHex(pc, reg(32));

	string buffer = stringFromFormat("T%02x%02x:%.16s;%02x:%.16s;", latestSignal, 32, pc, 31, sp);

	auto curthread = ctu->tm.current();
	if(curthread == nullptr)
		curthread = ctu->tm.last();
	if (curthread != nullptr)
		buffer += stringFromFormat("thread:%x;", curthread->id);

	LOG_DEBUG(GdbStub, "Response: %s", buffer.c_str());
	sendReply(buffer.c_str());
}

void GdbStub::readCommand() {
	commandLength = 0;
	memset(commandBuffer, 0, sizeof(commandBuffer));

	uint8_t c = readByte();
	if(c == '+') {
		// ignore ack
		return;
	} else if(c == 0x03) {
		LOG_INFO(GdbStub, "gdb: found break command");
		haltLoop = true;
		remoteBreak = true;
		return;
	} else if(c != GDB_STUB_START) {
		LOG_DEBUG(GdbStub, "gdb: read invalid byte %02x", c);
		return;
	}

	while((c = readByte()) != GDB_STUB_END) {
		if(commandLength >= sizeof(commandBuffer)) {
			LOG_ERROR(GdbStub, "gdb: commandBuffer overflow");
			sendPacket(GDB_STUB_NACK);
			return;
		}
		commandBuffer[commandLength++] = c;
	}

	auto checksum_received = hexCharToValue(readByte()) << 4;
	checksum_received |= hexCharToValue(readByte());

	auto checksum_calculated = calculateChecksum(commandBuffer, commandLength);

	if(checksum_received != checksum_calculated)
		LOG_ERROR(GdbStub,
				  "gdb: invalid checksum: calculated %02x and read %02x for $%s# (length: %d)",
				  checksum_calculated, checksum_received, commandBuffer, commandLength);

	sendPacket(GDB_STUB_ACK);
}

bool GdbStub::isDataAvailable() {
	fd_set fd_socket;

	FD_ZERO(&fd_socket);
	FD_SET(client, &fd_socket);

	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 0;

	if(select(client + 1, &fd_socket, nullptr, nullptr, &t) < 0) {
		LOG_ERROR(GdbStub, "select failed");
		return false;
	}

	return FD_ISSET(client, &fd_socket) != 0;
}

void GdbStub::readRegister() {
	uint8_t reply[64];
	memset(reply, 0, sizeof(reply));

	uint32_t id = hexCharToValue(commandBuffer[1]);
	if(commandBuffer[2] != '\0') {
		id <<= 4;
		id |= hexCharToValue(commandBuffer[2]);
	}

	if(id <= 32)
		intToGdbHex(reply, reg(id));
	else if(id == 33)
		memset(reply, '0', 8);
	else
		return sendReply("E01");

	sendReply(reinterpret_cast<char*>(reply));
}

void GdbStub::readRegisters() {
	uint8_t buffer[GDB_BUFFER_SIZE - 4 + 1];
	memset(buffer, 0, sizeof(buffer));

	uint8_t* bufptr = buffer;
	for(int i = 0; i <= 32; i++) {
		intToGdbHex(bufptr + i * 16, reg(i));
	}

	bufptr += (33 * 16);

	memset(bufptr, '0', 8);
	bufptr[8] = '\0';

	sendReply(reinterpret_cast<char*>(buffer));
}

void GdbStub::writeRegister() {
	const uint8_t* buffer_ptr = commandBuffer + 3;

	uint32_t id = hexCharToValue(commandBuffer[1]);
	if(commandBuffer[2] != '=') {
		++buffer_ptr;
		id <<= 4;
		id |= hexCharToValue(commandBuffer[2]);
	}

	auto val = gdbHexToInt(buffer_ptr);

	if(id <= 32)
		reg(id, val);
	else if(id == 33) {
	}
	else
		return sendReply("E01");

	sendReply("OK");
}

void GdbStub::writeRegisters() {
	const uint8_t* buffer_ptr = commandBuffer + 1;

	if(commandBuffer[0] != 'G')
		return sendReply("E01");
	for(auto i = 0; i < 33; ++i)
		if(i <= 32)
			reg(i, gdbHexToInt(buffer_ptr + i * 16));

	sendReply("OK");
}

void GdbStub::readMemory() {
	uint8_t reply[GDB_BUFFER_SIZE - 4];

	auto start_offset = commandBuffer + 1;
	auto addr_pos = find(start_offset, commandBuffer + commandLength, ',');
	auto addr = hexToInt(start_offset, static_cast<uint32_t>(addr_pos - start_offset));

	start_offset = addr_pos + 1;
	auto len = hexToInt(start_offset, static_cast<uint32_t>((commandBuffer + commandLength) - start_offset));

	LOG_DEBUG(GdbStub, "gdb: addr: " ADDRFMT " len: " ADDRFMT, addr, len);

	if(len * 2 > sizeof(reply)) {
		sendReply("E01");
	}

	auto data = new uint8_t[len];
	if(ctu->cpu.readmem(addr, data, len)) {
		memToGdbHex(reply, data, len);
		reply[len * 2] = '\0';
		sendReply(reinterpret_cast<char*>(reply));
	} else
		sendReply("E00");

	delete[] data;
}

void GdbStub::writeMemory() {
	auto start_offset = commandBuffer + 1;
	auto addr_pos = find(start_offset, commandBuffer + commandLength, ',');
	gptr addr = hexToInt(start_offset, static_cast<uint32_t>(addr_pos - start_offset));

	start_offset = addr_pos + 1;
	auto len_pos = find(start_offset, commandBuffer + commandLength, ':');
	auto len = hexToInt(start_offset, static_cast<uint32_t>(len_pos - start_offset));

	auto dst = new uint8_t[len];
	gdbHexToMem(dst, len_pos + 1, len);

	if(ctu->cpu.writemem(addr, dst, len))
		sendReply("OK");
	else
		sendReply("E00");

	delete[] dst;
}

void GdbStub::_break(bool is_memoryBreak) {
	if(!haltLoop) {
		haltLoop = true;
		sendSignal(SIGTRAP);
	}

	memoryBreak = is_memoryBreak;
}

void GdbStub::step() {
	stepLoop = true;
	haltLoop = true;
}

void GdbStub::_continue() {
	memoryBreak = false;
	stepLoop = false;
	haltLoop = false;
}

bool GdbStub::commitBreakpoint(BreakpointType type, gptr addr, uint32_t len) {
	auto& p = getBreakpointList(type);

	Breakpoint breakpoint;
	breakpoint.active = true;
	breakpoint.addr = addr;
	breakpoint.len = len;

	if(type == BreakpointType::Execute)
		breakpoint.hook = ctu->cpu.addCodeBreakpoint(addr);
	else
		breakpoint.hook = ctu->cpu.addMemoryBreakpoint(addr, len, type);

	p.insert({addr, breakpoint});

	LOG_DEBUG(GdbStub, "gdb: added %d breakpoint: " ADDRFMT " bytes at " ADDRFMT, type, breakpoint.len,
			  breakpoint.addr);

	return true;
}

void GdbStub::addBreakpoint() {
	BreakpointType type;

	uint8_t type_id = hexCharToValue(commandBuffer[1]);
	switch (type_id) {
	case 0:
	case 1:
		type = BreakpointType::Execute;
		break;
	case 2:
		type = BreakpointType::Write;
		break;
	case 3:
		type = BreakpointType::Read;
		break;
	case 4:
		type = BreakpointType::Access;
		break;
	default:
		return sendReply("E01");
	}

	auto start_offset = commandBuffer + 3;
	auto addr_pos = find(start_offset, commandBuffer + commandLength, ',');
	gptr addr = hexToInt(start_offset, static_cast<uint32_t>(addr_pos - start_offset));

	start_offset = addr_pos + 1;
	auto len = (uint32_t) hexToInt(start_offset, static_cast<uint32_t>((commandBuffer + commandLength) - start_offset));

	if(type == BreakpointType::Access) {
		type = BreakpointType::Read;

		if(!commitBreakpoint(type, addr, len)) {
			return sendReply("E02");
		}

		type = BreakpointType::Write;
	}

	if(!commitBreakpoint(type, addr, len)) {
		return sendReply("E02");
	}

	sendReply("OK");
}

void GdbStub::removeBreakpoint() {
	BreakpointType type;

	uint8_t type_id = hexCharToValue(commandBuffer[1]);
	switch (type_id) {
	case 0:
	case 1:
		type = BreakpointType::Execute;
		break;
	case 2:
		type = BreakpointType::Write;
		break;
	case 3:
		type = BreakpointType::Read;
		break;
	case 4:
		type = BreakpointType::Access;
		break;
	default:
		return sendReply("E01");
	}

	auto start_offset = commandBuffer + 3;
	auto addr_pos = find(start_offset, commandBuffer + commandLength, ',');
	gptr addr = hexToInt(start_offset, static_cast<uint32_t>(addr_pos - start_offset));

	if(type == BreakpointType::Access) {
		type = BreakpointType::Read;
		removeBreakpoint(type, addr);

		type = BreakpointType::Write;
	}

	removeBreakpoint(type, addr);
	sendReply("OK");
}

void GdbStub::isThreadAlive() {
	int threadid = (int)hexToInt(commandBuffer + 1, strlen((char*)commandBuffer + 1));
	auto threads = ctu->tm.thread_list();
	for (auto it = threads.begin(); it != threads.end(); it++) {
		if ((*it)->id == threadid) {
			sendReply("OK");
			return;
		}
	}
	sendReply("E01");
}

void GdbStub::handlePacket() {
	if(!isDataAvailable())
		return;

	readCommand();
	if(commandLength == 0)
		return;

	LOG_DEBUG(GdbStub, "Packet: %s", commandBuffer);

	switch(commandBuffer[0]) {
	case 'q':
		handleQuery();
		break;
	case 'H':
		handleSetThread();
		break;
	case '?':
		sendSignal(latestSignal);
		break;
	case 'k':
		LOG_ERROR(GdbStub, "killed by gdb");
	case 'g':
		readRegisters();
		break;
	case 'G':
		writeRegisters();
		break;
	case 'p':
		readRegister();
		break;
	case 'P':
		writeRegister();
		break;
	case 'm':
		readMemory();
		break;
	case 'M':
		writeMemory();
		break;
	case 's':
		step();
		return;
	case 'C':
	case 'c':
		_continue();
		return;
	case 'z':
		removeBreakpoint();
		break;
	case 'T':
		isThreadAlive();
		break;
	case 'Z':
		addBreakpoint();
		break;
	default:
		sendReply("");
		break;
	}
}
