#include "Ctu.h"

uint32_t nn::psc::sf::IPmModule::Unknown0(IN uint32_t _0, IN uint8_t * _1, guint _1_size, OUT shared_ptr<KObject>& _2) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::psc::sf::IPmModule::Unknown0");
	_2 = make_shared<Waitable>();
	return 0;
}
