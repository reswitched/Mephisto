#include "Ctu.h"

/*$IPC$
partial nn::lm::ILogger {
	LogMessage *current;
}
*/

class LogMessage {
public:
	LogMessage(int _severity, int _verbosity) : severity(_severity), verbosity(_verbosity) {
		message = "";
		filename = function = module = thread = "unknown";
		line = -1;
	}

	void addMessage(char *data) {
		message += data;
	}

	void print() {
		while(message.back() == '\n')
			message = message.substr(0, message.size() - 1);

		if(message.size() == 0)
			return;

		LOG_DEBUG(Lm, "Thread %s, module %s, file %s, function %s, line %i:", thread.c_str(), module.c_str(), filename.c_str(), function.c_str(), line);
		LOG_DEBUG(Lm, "%s", message.c_str());
	}

	int severity, verbosity, line;
	string message, filename, function, module, thread;
};

nn::lm::ILogger::ILogger(Ctu *_ctu) : IpcService(_ctu), current(nullptr) {
}

#pragma pack(push, 1)
struct InLogPacket {
	uint64_t pid;
	gptr threadContext;
	uint16_t flags;
	uint8_t severity, verbosity;
	uint32_t payloadSize;
};
#pragma pack(pop)

void dumpstring(uint8_t *data, guint size) {
	gptr addr = 0x0;

	auto hfmt = "%08lx | ";
	if((addr + size) & 0xFFFF000000000000)
		hfmt = "%016lx | ";
	else if((addr + size) & 0xFFFFFFFF00000000)
		hfmt = "%012lx | ";

	for(uint32_t i = 0; i < size; i += 16) {
		printf(hfmt, addr+i);
		string ascii = "";
		for(uint8_t j = 0; j < 16; j++) {
			if((i+j) < size) {
				printf("%02x ", (uint8_t)data[i+j]);
				if(isprint(data[i+j]))
					ascii += data[i+j];
				else
					ascii += ".";
			} else {
				printf("  ");
				ascii += " ";
			}
			if(j==7) {
				printf(" ");
				ascii += " ";
			}
		}
		printf("| %s\n", ascii.c_str());
	}
}

uint32_t nn::lm::ILogger::Initialize(IN uint8_t *message, guint messageSize) {
	auto packet = (InLogPacket *) message;
	dumpstring(message, messageSize);

	bool isHead = packet->flags & 1;
	bool isTail = packet->flags & 2;

	if(isHead || current == nullptr) {
		if(current != nullptr)
			delete current;
		current = new LogMessage(packet->severity, packet->verbosity);
	}

	auto offset = 24;
	while(offset < messageSize) {
		uint8_t id = *(uint8_t *)(&message[offset++]);
		uint8_t len = *(uint8_t *)(&message[offset++]);

		auto buf = new char[len + 1];
		memset(buf, 0, len + 1);
		memcpy(buf, &message[offset], len);
		if(len == 0) {
			delete[] buf;
			continue;
		}

		switch(id) {
			case 2:
				current->addMessage(buf);
				break;
			case 3:
				current->line = *(uint32_t *)(message + offset);
				break;
			case 4:
				current->filename = buf;
				break;
			case 5:
				current->function = buf;
				break;
			case 6:
				current->module = buf;
				break;
			case 7:
				current->thread = buf;
				break;

			default:
				LOG_DEBUG(Lm, "Invalid ID: %d!!!", id);
				break;
		}
		offset += len;
		delete[] buf;
	}

	if(isTail) {
		current->print();
		delete current;
		current = nullptr;
	}

	return 0;
}
