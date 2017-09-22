#include "Ctu.h"

#include "IpcStubs.h"

#define BRIDGE_PORT 31337

IpcBridge::IpcBridge(Ctu *_ctu) : ctu(_ctu) {
	struct sockaddr_in addr;
	serv = socket(AF_INET, SOCK_STREAM, 0);
	auto enable = 1;
	setsockopt(serv, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(BRIDGE_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	memset(addr.sin_zero, '\0', sizeof addr.sin_zero);
	bind(serv, (struct sockaddr *) &addr, sizeof addr);
	listen(serv, 10);
	client = -1;
	waitingForAsync = false;
}

void IpcBridge::start() {
	LOG_INFO(IpcBridge, "Starting");

	for(auto i = 0; i < 24; ++i) {
		auto addr = (i + 1) * (1 << 20) + (1 << 28);
		ctu->cpu.map(addr, 1024 * 1024);
		buffers[i] = addr;
	}

	auto thread = ctu->tm.createNative([this] { run(); });
	thread->resume();
}

bool isReadable(int fd) {
	struct timeval tv;
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	tv.tv_sec = tv.tv_usec = 0;
	return select(fd + 1, &rfds, nullptr, nullptr, &tv) != -1 && FD_ISSET(fd, &rfds);
}

uint64_t IpcBridge::readint() {
	uint64_t val = 0;
	recv(client, &val, 8, 0);
	return val;
}

uint64_t IpcBridge::readint(bool &closed) {
	uint64_t val = 0;
	if(recv(client, &val, 8, 0) == 0)
		closed = true;
	return val;
}

string IpcBridge::readstring() {
	auto size = readint();
	auto buf = new char[size];
	recv(client, buf, size, 0);
	string ret(buf, size);
	delete[] buf;
	return ret;
}

IpcData IpcBridge::readdata() {
	auto size = readint();
	auto buf = new uint8_t[size];
	recv(client, buf, size, 0);
	auto bufptr = buffers[bufferOff++];
	ctu->cpu.writemem(bufptr, buf, size);
	delete[] buf;
	return IpcData(bufptr, size);
}

void IpcBridge::writeint(uint64_t val) {
	send(client, &val, 8, 0);
}

void IpcBridge::writedesc(vector<tuple<IpcData, int>> descs) {
	writeint(descs.size());
	for(auto [data, flag] : descs) {
		writeint(data.size);
		auto buf = new uint8_t[data.size];
		ctu->cpu.readmem(data.ptr, buf, data.size);
		send(client, buf, data.size, 0);
		writeint(flag);
	}
}

void IpcBridge::writedesc(vector<IpcData> descs) {
	writeint(descs.size());
	if(descs.size() > 0)
		LOG_ERROR(IpcBridge, "C descriptors unsupported");
}

void IpcBridge::sendResponse(ghandle hnd, uint32_t res, bool close, shared_ptr<array<uint8_t, 0x100>> buf, const OutgoingBridgeMessage &orig) {
	if(close) {
		openHandles.erase(hnd);
		ctu->deleteHandle(hnd);
		writeint(0xf601);
		return;
	}
	if(res != 0) {
		writeint(res);
		return;
	}

	hexdump(buf);

	IncomingBridgeMessage msg(buf);

	writeint(0);
	writeint(msg.data.size()); for(auto v : msg.data) writeint(v);
	writeint(msg.copiedHandles.size()); for(auto v : msg.copiedHandles) writeint(v);
	writeint(msg.movedHandles.size());
	for(auto v : msg.movedHandles) {
		openHandles[v] = ctu->getHandle<KObject>(v);
		writeint(v);
	}

	writedesc(orig.a);
	writedesc(orig.b);
	writedesc(orig.c);
	writedesc(orig.x);

	writeint(msg.type);
}

void IpcBridge::run() {
	if(waitingForAsync)
		return;

	if(client == -1) {
		if(!isReadable(serv))
			return;
		struct sockaddr_storage client_addr;
		socklen_t addr_size = sizeof client_addr;
		client = accept(serv, (struct sockaddr *) &client_addr, &addr_size);
		LOG_INFO(IpcBridge, "IPC bridge got connection");
	} else {
		if(!isReadable(client))
			return;
		auto closed = false;
		auto cmd = readint(closed);
		if(closed) {
			LOG_INFO(IpcBridge, "Client disconnected");
			client = -1;
			for(auto [hnd, _] : openHandles)
				ctu->deleteHandle(hnd);
			openHandles.clear();
			return;
		}
		switch(cmd) {
		case 0: { // Open service
			auto name = readstring();
			LOG_DEBUG(IpcBridge, "Attempting to open service %s", name.c_str());
			auto sm = ctu->ipc.sm;
			if(sm->ports.find(name) == sm->ports.end()) {
				LOG_DEBUG(IpcBridge, "Unknown service!");
				writeint(0);
				return;
			}
			auto obj = sm->ports[name]->connectSync();
			auto hnd = ctu->newHandle(obj);
			openHandles[hnd] = obj;
			writeint(hnd);
			break;
		}
		case 1: { // Close handle
			auto hnd = (ghandle) readint();
			openHandles.erase(hnd);
			ctu->deleteHandle(hnd);
			break;
		}
		case 2: { // Message
			OutgoingBridgeMessage msg;
			bufferOff = 0;
			msg.type = readint();
			msg.data = readarray([this] { return readint(); });
			msg.pid = readint();
			msg.copiedHandles = readarray([this] { return (ghandle) readint(); });
			msg.movedHandles = readarray([this] { return (ghandle) readint(); });
			msg.a = readarray([this] { return tuple{readdata(), (int) readint()}; });
			msg.b = readarray([this] { return tuple{readdata(), (int) readint()}; });
			msg.c = readarray([this] { return readdata(); });
			msg.x = readarray([this] { return tuple{readdata(), (int) readint()}; });
			auto hnd = (ghandle) readint();

			auto packed = msg.pack();

			if(openHandles.find(hnd) == openHandles.end()) {
				writeint(0xe401); // Bad handle
				break;
			}

			auto obj = dynamic_pointer_cast<IPipe>(openHandles[hnd]);
			if(obj->isAsync()) {
				waitingForAsync = true;
				obj->messageAsync(packed, [=](auto res, auto close) {
					sendResponse(hnd, res, close, packed, msg);
					waitingForAsync = false;
				});
			} else {
				auto close = false;
				auto ret = obj->messageSync(packed, close);
				sendResponse(hnd, ret, close, packed, msg);
			}
			break;
		}
		default:
			close(client);
			client = -1;
			break;
		}
	}
}
