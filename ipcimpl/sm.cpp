#include "Ctu.h"

/*$IPC$
partial SmService {
	unordered_map<string, shared_ptr<NPort>> ports;
}
*/

static bool smInitialized = false;

uint32_t SmService::Initialize() {
	smInitialized = true;
	return 0;
}

#define SERVICE(str, iface) do { if(name == (str)) { svc = buildInterface(iface); return 0; } } while(0)

uint32_t SmService::GetService(IN ServiceName _name, OUT shared_ptr<IPipe>& svc) {
	if(!smInitialized) {
		// FIXME: libtransistor need to impl ::Initialize
		// return 0x415;
	}

	string name((char *) _name, strnlen((char *) _name, 8));
	
	if(ports.find(name) != ports.end()) {
		LOG_DEBUG(Sm, "Connecting to native IPC service!");
		svc = ports[name]->connectSync();
		return 0;
	}
	SERVICE_MAPPING();

	LOG_DEBUG(Sm, "Unknown service name %s", name.c_str());
	return 0xC15;
}

uint32_t SmService::RegisterService(IN ServiceName _name, OUT shared_ptr<NPort>& port) {
	string name((char *) _name, strnlen((char *) _name, 8));
	LOG_DEBUG(Sm, "Registering service %s", name.c_str());
	port = buildInterface(NPort, name);
	ports[name] = port;
	return 0;
}
