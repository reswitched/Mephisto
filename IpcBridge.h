#pragma once

#include "Ctu.h"

class IpcData {
public:
	IpcData(gptr _ptr, uint64_t _size) : ptr(_ptr), size(_size) {}

	gptr ptr;
	uint64_t size;
};

class OutgoingBridgeMessage {
public:
	auto pack() {
		auto ret = make_shared<array<uint8_t, 0x100>>();

		auto pos = 0;
		auto buf = (uint32_t *) ret->data();
		buf[0] = (uint32_t) (type | (x.size() << 16) | (a.size() << 20) | (b.size() << 24));
		buf[1] = (pid != (uint64_t) -1 || copiedHandles.size() > 0 || movedHandles.size() > 0) ? 1U << 31 : 0;
		buf[1] |= c.size() > 0 ? 2 << 10 : 0;
		pos = 2;
		if(pid != (uint64_t) -1 || copiedHandles.size() > 0 || movedHandles.size() > 0) {
			buf[pos] = pid != (uint64_t) -1 ? 1 : 0;
			buf[pos] |= copiedHandles.size() << 1;
			buf[pos++] |= movedHandles.size() << 5;
			if(pid != (uint64_t) -1) {
				buf[pos++] = pid & 0xFFFFFFFF;
				buf[pos++] = pid >> 32;
			}
			for(auto hnd : copiedHandles)
				buf[pos++] = hnd;
			for(auto hnd : movedHandles)
				buf[pos++] = hnd;
		}

		for(auto [ed, ec] : x) {
			auto laddr = ed.ptr & 0xFFFFFFFF, haddr = ed.ptr >> 32;
			buf[pos++] = (uint32_t) (
					(ec & 0x3F) | 
					(((haddr & 0x70) >> 4) << 6) | 
					(ec & 0xE00) | 
					((haddr & 0xF) << 12) | 
					(ed.size << 16)
				);
			buf[pos++] = (uint32_t) laddr;
		}

		vector<tuple<IpcData, int>> ab;
		ab.insert(ab.end(), a.begin(), a.end());
		ab.insert(ab.end(), b.begin(), b.end());
		for(auto [ad, af] : ab) {
			auto laddr = ad.ptr & 0xFFFFFFFF, haddr = ad.ptr >> 32;
			auto lsize = ad.size & 0xFFFFFFFF, hsize = ad.size >> 32;
			buf[pos++] = (uint32_t) lsize;
			buf[pos++] = (uint32_t) laddr;
			buf[pos++] = (uint32_t) (
					af | 
					(((haddr & 0x70) >> 4) << 2) | 
					((hsize & 0xF) << 24) | 
					((haddr & 0xF) << 28)
				);
		}

		auto epos = pos;
		while(pos & 3)
			buf[pos++] = 0;
		buf[pos++] = FOURCC('S', 'F', 'C', 'I');
		buf[pos++] = 0;
		for(auto elem : data) {
			buf[pos++] = (uint32_t) elem;
			buf[pos++] = (uint32_t) (elem >> 32);
		}

		assert(c.size() == 0); // XXX: Add c descriptor packing eventually. One day.

		buf[1] |= pos - epos + 2;

		return ret;
	}

	uint64_t type, pid;
	vector<uint64_t> data;
	vector<ghandle> copiedHandles, movedHandles;
	vector<tuple<IpcData, int>> a, b, x;
	vector<IpcData> c;
};

class IncomingBridgeMessage {
public:
	IncomingBridgeMessage(shared_ptr<array<uint8_t, 0x100>> _buf) {
		auto buf = (uint32_t *) _buf->data();
		type = buf[0] & 0xFFFF;
		auto nx = (buf[0] >> 16) & 0xF, na = (buf[0] >> 20) & 0xF, nb = (buf[0] >> 24) & 0xF;
		auto hasHD = buf[1] >> 31;
		auto hasC = (buf[1] >> 10) & 3;

		if(nx || na || nb || hasC)
			LOG_DEBUG(IpcBridge, "Warning! Message coming back from IPC bridge has descriptors. This is unhandled and may lose data.");

		auto pos = 2;

		if(hasHD) {
			auto hasPid = buf[pos] & 1, nc = (buf[pos] >> 1) & 0xF, nm = (buf[pos++] >> 5) & 0xF;
			if(hasPid) {
				pid = buf[pos] | (((uint64_t) buf[pos+1]) << 32);
				pos += 2;
			}
			for(auto i = 0; i < nc; ++i)
				copiedHandles.push_back((ghandle) buf[pos++]);
			for(auto i = 0; i < nm; ++i)
				movedHandles.push_back((ghandle) buf[pos++]);
		}

		pos += nx * 2 + na * 3 + nb * 3;

		auto epos = pos + (buf[1] & 0x3FF) - 2;
		while(pos & 3)
			pos++;
		assert(buf[pos++] == FOURCC('S', 'F', 'C', 'O'));
		pos++;

		for( ; pos < epos; pos += 2)
			data.push_back(buf[pos] | (((uint64_t) buf[pos+1]) << 32));
	}

	uint64_t type, pid;
	vector<uint64_t> data;
	vector<ghandle> copiedHandles, movedHandles;
};

class IpcBridge {
public:
	IpcBridge(Ctu *_ctu);

	void start();

private:
	void run();

	uint64_t readint();
	uint64_t readint(bool &closed);
	template<typename T>
	auto readarray(T &&cb) {
		vector<decltype(cb())> vec;
		auto count = readint();
		vec.reserve(count);
		for(auto i = 0; i < count; ++i)
			vec.push_back(cb());
		return vec;
	}
	string readstring();
	IpcData readdata();
	void writeint(uint64_t val);
	void writedesc(vector<tuple<IpcData, int>> descs);
	void writedesc(vector<IpcData> descs);

	void sendResponse(ghandle hnd, uint32_t res, bool closed, shared_ptr<array<uint8_t, 0x100>> buf, const OutgoingBridgeMessage &orig);

	Ctu *ctu;
	int serv;
	int client;

	unordered_map<ghandle, shared_ptr<KObject>> openHandles;
	gptr buffers[24];
	int bufferOff;
	bool waitingForAsync;
};
