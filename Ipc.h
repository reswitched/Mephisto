#pragma once

#include "Ctu.h"

// Used to clarify IPC stubs
#define IN
#define OUT

#define buildInterface(cls, ...) make_shared<cls>(ctu, ##__VA_ARGS__)

string bufferToString(uint8_t *buf, uint size);

class IPipe : public Waitable {
public:
	virtual ~IPipe() {}
	virtual bool isAsync() = 0;
	virtual uint32_t messageSync(shared_ptr<array<uint8_t, 0x100>> buf, bool& closeHandle) { return 0; }
	virtual void messageAsync(shared_ptr<array<uint8_t, 0x100>> buf, function<void(uint32_t, bool closeHandle)> cb) {}
};

class IPort : public Waitable {
public:
	virtual ~IPort() {}
	virtual bool isAsync() = 0;
	virtual shared_ptr<IPipe> connectSync() { return nullptr; }
	virtual void connectAsync(function<void(shared_ptr<IPipe>)> cb) {}
};

class PipeEnd : public Waitable {
public:
	PipeEnd() : closed(false) {}
	void close() override {
		if(closed)
			return;
		closed = true;
		signal();
	}

	void push(shared_ptr<array<uint8_t, 0x100>> message) {
		acquire();
		messages.push(message);
		release();
	}
	shared_ptr<array<uint8_t, 0x100>> pop() {
		acquire();
		if(closed) {
			release();
			return nullptr;
		}
		auto temp = messages.front();
		messages.pop();
		release();
		return temp;
	}
	bool closed;

private:
	queue<shared_ptr<array<uint8_t, 0x100>>> messages;
};

class NPipe : public IPipe {
public:
	NPipe(Ctu *_ctu, string _name) : ctu(_ctu), name(_name), closed(false) {}
	bool isAsync() override { return true; }

	void messageAsync(shared_ptr<array<uint8_t, 0x100>> buf, function<void(uint32_t, bool closeHandle)> cb) override;
	void close() override {
		if(closed)
			return;
		closed = true;
		server.close();
		client.close();
		signal();
	}

	string name;
	PipeEnd server, client;
	bool closed;

private:
	Ctu *ctu;
};

class NPort : public IPort {
public:
	NPort(Ctu *_ctu, string _name) : ctu(_ctu), name(_name) {}
	~NPort() override {}
	bool isAsync() override { return false; }
	shared_ptr<IPipe> connectSync() override;

	shared_ptr<NPipe> accept();

	string name;

private:
	Ctu *ctu;
	queue<shared_ptr<NPipe>> available;
};

class IncomingIpcMessage {
public:
	IncomingIpcMessage(uint8_t *_ptr, bool isDomainObject);
	template<typename T>
	T getData(uint offset) {
		return *((T *) (ptr + sfciOffset + 8 + offset));
	}
	template<typename T>
	T getDataPointer(uint offset) {
		return (T) (ptr + sfciOffset + 8 + offset);
	}
	gptr getBuffer(int btype, int num, guint& size) {
		if(btype & 0x20) {
			auto buf = getBuffer((btype & ~0x20) | 4, num, size);
			if(size != 0)
				return buf;
			return getBuffer((btype & ~0x20) | 8, num, size);
		}

		size = 0;
		auto ax = (btype & 3) == 1;
		auto flags_ = btype & 0xC0;
		auto flags = flags_ == 0x80 ? 3 : (flags_ == 0x40 ? 1 : 0);
		auto cx = (btype & 0xC) == 8;
		switch((ax << 1) | cx) {
		case 0: { // B
			auto t = (uint32_t *) (ptr + descOffset + xCount * 8 + aCount * 12 + num * 12);
			gptr a = t[0], b = t[1], c = t[2];
			size = (guint) (a | (((c >> 24) & 0xF) << 32));
			if((c & 0x3) != flags)
				LOG_ERROR(Ipc, "B descriptor flags don't match: %u vs expected %u", (uint) (c & 0x3), flags);
			return b | (((((c >> 2) << 4) & 0x70) | ((c >> 28) & 0xF)) << 32);
		}
		case 1: { // C
			assert(num == 0);
			auto t = (uint32_t *) (ptr + rawOffset + wlen * 4);
			gptr a = t[0], b = t[1];
			size = b >> 16;
			return a | ((b & 0xFFFF) << 32);
		}
		case 2: { // A
			auto t = (uint32_t *) (ptr + descOffset + xCount * 8 + num * 12);
			gptr a = t[0], b = t[1], c = t[2];
			size = (guint) (a | (((c >> 24) & 0xF) << 32));
			if((c & 0x3) != flags)
				LOG_ERROR(Ipc, "A descriptor flags don't match: %u vs expected %u", (uint) (c & 0x3), flags);
			return b | (((((c >> 2) << 4) & 0x70) | ((c >> 28) & 0xF)) << 32);
		}
		case 3: { // X
			auto t = (uint32_t *) (ptr + descOffset + num * 8);
			gptr a = t[0], b = t[1];
			size = (guint) (a >> 16);
			return b | ((((a >> 12) & 0xF) | ((a >> 2) & 0x70)) << 32);
		}
		}
		return 0;
	}
	ghandle getMoved(int off) {
		return *(ghandle *) (ptr + moveOffset + off * 4);
	}
	ghandle getCopied(int off) {
		return *(ghandle *) (ptr + copyOffset + off * 4);
	}

	uint cmdId, type;
	uint aCount, bCount, cCount, hasC, xCount, hasPid, moveCount, copyCount;
	gpid pid;

	ghandle domainHandle;
	uint domainCommand;

private:
	uint wlen, rawOffset, sfciOffset, descOffset, copyOffset, moveOffset;
	uint8_t *ptr;
};

class OutgoingIpcMessage {
public:
	OutgoingIpcMessage(uint8_t *_ptr, bool _isDomainObject);
	void initialize(uint _moveCount, uint _copyCount, uint dataBytes);
	void commit();
	template<typename T>
	T getDataPointer(uint offset) {
		return (T) (ptr + sfcoOffset + 8 + offset + (offset < 8 ? 0 : realDataOffset));
	}
	void copy(int offset, ghandle hnd) {
		auto buf = (uint32_t *) ptr;
		buf[3 + offset] = hnd;
	}
	void move(int offset, ghandle hnd) {
		auto buf = (uint32_t *) ptr;
		if(isDomainObject)
			buf[(sfcoOffset >> 2) + 4 + offset] = hnd;
		else
			buf[3 + copyCount + offset] = hnd;
	}

	uint32_t errCode;
	uint moveCount, copyCount;
	bool isDomainObject;

private:
	uint sfcoOffset;
	uint realDataOffset;
	uint8_t *ptr;
};

class IpcService : public IPipe {
public:
	IpcService(Ctu *_ctu) : ctu(_ctu), domainOwner(nullptr), isDomainObject(false), domainHandleIter(0xf001), thisHandle(0xf000) {}
	~IpcService() override {}
	bool isAsync() override { return false; }
	uint32_t messageSync(shared_ptr<array<uint8_t, 0x100>> buf, bool& closeHandle) override;
	virtual uint32_t dispatch(IncomingIpcMessage &req, OutgoingIpcMessage &resp) { return 0xF601; }

protected:
	Ctu *ctu;

	IpcService *domainOwner;
	bool isDomainObject;
	int domainHandleIter, thisHandle;
	unordered_map<uint32_t, shared_ptr<KObject>> domainHandles;

	template<typename T>
	ghandle createHandle(shared_ptr<T> obj) {
		static_assert(std::is_base_of<KObject, T>::value, "T must derive from KObject");
		if(domainOwner)
			return domainOwner->createHandle(obj);
		else if(isDomainObject) {
			auto hnd = domainHandleIter++;

			auto temp = dynamic_pointer_cast<IpcService>(obj);
			if(temp != nullptr)
				temp->domainOwner = this;

			domainHandles[hnd] = dynamic_pointer_cast<KObject>(obj);
			return hnd;
		} else
			return fauxNewHandle(dynamic_pointer_cast<KObject>(obj));
	}

	ghandle fauxNewHandle(shared_ptr<KObject> obj);
};

class IUnknown : public IpcService {
};

namespace nn::sm::detail {
	class IUserInterface;
}

class Ipc {
public:
	Ipc(Ctu *_ctu);
	ghandle ConnectToPort(string name);
	shared_ptr<nn::sm::detail::IUserInterface> sm;
private:
	Ctu *ctu;
};
