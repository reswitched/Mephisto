#include "Ctu.h"

uint32_t nn::fatalsrv::IService::TransitionToFatalError(IN uint64_t errorCode, IN uint64_t _1, IN uint8_t * errorBuf, guint errorBuf_size, IN gpid _3) {
	LOG_DEBUG(Fatal, "!!! FATAL ERROR !!! ERROR CODE: " ADDRFMT, errorCode);
	uint64_t stackSize = *(uint64_t *)(&errorBuf[0x240]);
	LOG_DEBUG(Fatal, "Stack trace");
	LOG_DEBUG(Fatal, "----------------------------");
	for(uint64_t i = 0; i < stackSize; i++) {
		uint64_t addr = *(uint64_t *)(&errorBuf[0x130 + (i*8)]);
		LOG_DEBUG(Fatal, "\t[%x] " ADDRFMT, (uint) i, addr);
	}
	LOG_DEBUG(Fatal, "----------------------------");
	return 0;
}
