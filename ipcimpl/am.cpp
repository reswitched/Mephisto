#include "Ctu.h"

uint32_t nn::am::service::ICommonStateGetter::GetEventHandle(OUT shared_ptr<KObject>& _0) {
	shared_ptr<Waitable> waitobj = make_shared<Waitable>();
        waitobj->signal(0);
        _0 = waitobj;
	return 0;
}

uint32_t nn::am::service::ICommonStateGetter::ReceiveMessage(OUT nn::am::AppletMessage& _0) {
	_0 = 0xF;
	return 0;
}

uint32_t nn::am::service::ICommonStateGetter::GetCurrentFocusState(OUT uint8_t& _0) {
	_0 = 1;
	return 0;
}

uint32_t nn::am::service::IWindowController::GetAppletResourceUserId(OUT nn::applet::AppletResourceUserId& _0) {
	_0 = 1;
	return 0;
}
