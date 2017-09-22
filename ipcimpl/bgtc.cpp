#include "Ctu.h"

uint32_t nn::bgtc::ITaskService::Unknown14(OUT shared_ptr<KObject>& _0) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::bgtc::ITaskService::Unknown14");
	_0 = make_shared<Waitable>();
	return 0;
}
