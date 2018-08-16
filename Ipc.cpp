#define DEFINE_STUBS
#include "Ctu.h"

string bufferToString(uint8_t *buf, uint size) {
	std::stringstream ss;
	auto allValid = true;
	auto hitNull = false;
	for(auto i = 0; i < size; ++i)
		if(hitNull && buf[i] != 0) {
			allValid = false;
			break;
		} else if(buf[i] == 0)
			hitNull = true;
		else if(buf[i] < 0x20 || buf[i] > 0x7E) {
			allValid = false;
			break;
		}
	if(allValid) {
		ss << '"';
		for(auto i = 0; i < size && buf[i] != 0; ++i)
			ss << (char) buf[i];
		ss << '"';
	} else {
		ss << "[";
		for(auto i = 0; i < size; ++i) {
			if(i != 0)
				ss << ", ";
			ss << "0x" << hex << setw(2) << setfill('0') << (int) buf[i];
		}
		ss << "]";
	}
	return ss.str();
}

void NPipe::messageAsync(shared_ptr<array<uint8_t, 0x100>> buf, function<void(uint32_t, bool closeHandle)> cb) {
	acquire();
	client.wait([=] {
		auto obuf = client.pop();
		if (obuf != nullptr) {
			memcpy(buf->data(), obuf->data(), 0x100);
			cb(0, false); // XXX: HANDLE RETCODES
		} else {
			cb(0, true);
		}
		return 1;
	});
	server.push(buf);
	signal(true);
	release();
}

shared_ptr<IPipe> NPort::connectSync() {
	acquire();
	auto pipe = make_shared<NPipe>(ctu, name);
	available.push(pipe);
	signal(true);
	release();
	return pipe;
}

shared_ptr<NPipe> NPort::accept() {
	acquire();
	assert(available.size() > 0);
	auto val = available.front();
	available.pop();
	release();
	return val;
}

IncomingIpcMessage::IncomingIpcMessage(uint8_t *_ptr, bool isDomainObject) : ptr(_ptr) {
	auto buf = (uint32_t *) ptr;
	type = buf[0] & 0xFFFF;
	xCount = (buf[0] >> 16) & 0xF;
	aCount = (buf[0] >> 20) & 0xF;
	bCount = (buf[0] >> 24) & 0xF;
	wlen = buf[1] & 0x3FF;
	hasC = ((buf[1] >> 10) & 0x3) != 0;
	domainHandle = 0;
	domainCommand = 0;
	auto hasHD = (buf[1] >> 31) == 1;
	auto pos = 2;

	if(hasHD) {
		auto hd = buf[pos++];
		hasPid = hd & 1;
		copyCount = (hd >> 1) & 0xF;
		moveCount = hd >> 5;
		if(hasPid) {
			pid = *((uint64_t *) &buf[pos]);
			pos += 2;
		}
		copyOffset = pos * 4;
		pos += copyCount;
		moveOffset = pos * 4;
		pos += moveCount;
	}

	descOffset = pos * 4;

	pos += xCount * 2;
	pos += aCount * 3;
	pos += bCount * 3;
	rawOffset = pos * 4;
	if(pos & 3)
		pos += 4 - (pos & 3);
	if(isDomainObject && type == 4) {
		domainHandle = buf[pos + 1];
		domainCommand = buf[pos] & 0xFF;
		pos += 4;
	}

	assert(type == 2 || (isDomainObject && domainCommand == 2) || buf[pos] == FOURCC('S', 'F', 'C', 'I'));
	sfciOffset = pos * 4;

	cmdId = getData<uint32_t>(0);
}

OutgoingIpcMessage::OutgoingIpcMessage(uint8_t *_ptr, bool _isDomainObject) : ptr(_ptr), isDomainObject(_isDomainObject) {
}

void OutgoingIpcMessage::initialize(uint _moveCount, uint _copyCount, uint dataBytes) {
	moveCount = _moveCount;
	copyCount = _copyCount;

	auto buf = (uint32_t *) ptr;
	buf[0] = 0;
	if(moveCount != 0 || copyCount != 0) {
		buf[1] = ((moveCount != 0 && !isDomainObject) || copyCount != 0) ? (1U << 31) : 0;
		buf[2] = (copyCount << 1) | ((isDomainObject ? 0 : moveCount) << 5);
	}

	auto pos = 2 + (((moveCount != 0 && !isDomainObject) || copyCount != 0) ? (1 + moveCount + copyCount) : 0);
	auto start = pos;
	if(pos & 3)
		pos += 4 - (pos & 3);
	if(isDomainObject) {
		buf[pos] = moveCount;
		pos += 4;
	}
	realDataOffset = isDomainObject ? moveCount << 2 : 0;
	auto dataWords = (realDataOffset >> 2) + (dataBytes & 3) ? (dataBytes >> 2) + 1 : (dataBytes >> 2);

	buf[1] |= 4 + (isDomainObject ? 4 : 0) + 4 + dataWords;
 
	sfcoOffset = pos * 4;
	buf[pos] = FOURCC('S', 'F', 'C', 'O');
}

void OutgoingIpcMessage::commit() {
	auto buf = (uint32_t *) ptr;
	buf[(sfcoOffset >> 2) + 2] = errCode;
}

uint32_t IpcService::messageSync(shared_ptr<array<uint8_t, 0x100>> buf, bool& closeHandle) {
	uint8_t obuf[0x100];
	memset(obuf, 0, 0x100);
	//hexdump(buf, 0x50);
	IncomingIpcMessage msg(buf->data(), isDomainObject);
	OutgoingIpcMessage resp(obuf, isDomainObject);
	auto ret = 0xf601;
	IpcService *target = this;
	if(isDomainObject && msg.domainHandle != thisHandle && msg.type == 4) {
		if(domainHandles.find(msg.domainHandle) != domainHandles.end())
			target = dynamic_pointer_cast<IpcService>(domainHandles[msg.domainHandle]).get();
		else
			LOG_ERROR(Ipc, "Unknown domain handle! 0x%x", msg.domainHandle);
	}
	if(!isDomainObject || msg.domainCommand == 1 || msg.type == 2 || msg.type == 5)
		switch(msg.type) {
		case 2: // Close
			closeHandle = true;
			resp.initialize(0, 0, 0);
			resp.errCode = 0;
			ret = 0x25a0b;
			break;
		case 4: // Normal
		case 6: // Normal with Context
			ret = target->dispatch(msg, resp);
			break;
		case 5: // Control
		case 7: // Control with Context
			switch(msg.cmdId) {
			case 0: // ConvertSessionToDomain
				LOG_DEBUG(Ipc, "ConvertSessionToDomain");
				resp.initialize(0, 0, 4);
				isDomainObject = true;
				*resp.getDataPointer<uint32_t *>(8) = thisHandle;
				resp.errCode = 0;
				ret = 0;
				break;
			case 2: // DuplicateSession
				LOG_DEBUG(Ipc, "DuplicateSession");
				resp.isDomainObject = false;
				resp.initialize(1, 0, 0);
				resp.move(0, ctu->duplicateHandle(dynamic_cast<KObject *>(this)));
				resp.errCode = 0;
				ret = 0;
				break;
			case 3: // QueryPointerBufferSize
				LOG_DEBUG(Ipc, "QueryPointerBufferSize");
				resp.initialize(0, 0, 4);
				*resp.getDataPointer<uint32_t *>(8) = 0x500;
				resp.errCode = 0;
				ret = 0;
				break;
			case 4: // DuplicateSession
				LOG_DEBUG(Ipc, "DuplicateSessionEx");
				resp.isDomainObject = false;
				resp.initialize(1, 0, 0);
				resp.move(0, ctu->duplicateHandle(dynamic_cast<KObject *>(this)));
				resp.errCode = 0;
				ret = 0;
				break;
			default:
				LOG_ERROR(Ipc, "Unknown cmdId to control %u", msg.cmdId);
			}
			break;
		}
	else
		switch(msg.domainCommand) {
		case 2:
			domainHandles.erase(msg.domainHandle);
			resp.initialize(0, 0, 0);
			resp.errCode = 0;
			ret = 0;
			break;
		default:
			LOG_ERROR(Ipc, "Unknown cmdId to domain %u", msg.domainCommand);
		}

	if(ret == 0) {
		resp.commit();
		memcpy(buf->data(), obuf, 0x100);
		//hexdump(buf, 0x50);
	}
	return ret;
}

ghandle IpcService::fauxNewHandle(shared_ptr<KObject> obj) {
	return ctu->newHandle(obj);
}

Ipc::Ipc(Ctu *_ctu) : ctu(_ctu) {
	sm = make_shared<nn::sm::detail::IUserInterface>(ctu);
}

ghandle Ipc::ConnectToPort(string name) {
	if(name != "sm:")
		LOG_ERROR(Ipc, "Attempt to connect to unknown port: \"%s\"", name.c_str());
	return ctu->newHandle(sm);
}
