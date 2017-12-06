#include "Ctu.h"

uint32_t nn::hid::IAppletResource::GetSharedMemoryHandle(OUT shared_ptr<KObject>& _0) {
	_0 = make_shared<MemoryBlock>(0x40000, 0x1);
	return 0;
}
