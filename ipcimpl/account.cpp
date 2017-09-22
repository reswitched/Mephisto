#include "Ctu.h"

uint32_t nn::account::detail::INotifier::GetSystemEvent(OUT shared_ptr<KObject>& _0) {
	LOG_INFO(Account, "Stub implementation for nn::account::detail::INotifier::GetSystemEvent");
	_0 = make_shared<Waitable>();
	return 0;
}
