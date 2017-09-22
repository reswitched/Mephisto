#include "Ctu.h"

Waitable::Waitable() : presignaled(false), canceled(false) {
}

void Waitable::acquire() {
	lock.lock();
}

void Waitable::release() {
	lock.unlock();
}

void Waitable::wait(function<int()> cb) {
	wait([cb](auto _) { return cb(); });
}

void Waitable::wait(function<int(bool)> cb) {
	acquire();
	if(!presignaled || (presignaled && cb(canceled) == 0))
		waiters.push_back(cb);
	presignaled = false;
	canceled = false;
	release();
}

void Waitable::signal(bool one) {
	acquire();
	if(waiters.size() == 0 && presignalable())
		presignaled = true;
	else {
		auto realhit = false;
		for(auto iter = waiters.begin(); iter != waiters.end();) {
			auto res = (*iter)(canceled);
			if(res != 0) {
				iter = waiters.erase(iter);
			} else {
				iter++;
			}
			if(res != -1) {
				realhit = true;
				if(one)
					break;
			}
		}
		if(!realhit && presignalable())
			presignaled = true;
	}
	release();
}

void Waitable::cancel() {
	acquire();
	canceled = true;
	signal();
	release();
}

Semaphore::Semaphore(Guest<uint32_t> _vptr) : vptr(_vptr) {
}

void Semaphore::increment() {
	acquire();
	vptr = *vptr + 1;
	release();
}

void Semaphore::decrement() {
	acquire();
	vptr = *vptr - 1;
	release();
}

uint32_t Semaphore::value() {
	return *vptr;
}

Mutex::Mutex(Guest<uint32_t> _vptr) : vptr(_vptr) {
}

uint32_t Mutex::value() {
	return *vptr;
}
void Mutex::value(uint32_t val) {
	vptr = val;
}

ghandle Mutex::owner() {
	return value() & 0xBFFFFFFF;
}
void Mutex::owner(ghandle val) {
	value((value() & 0x40000000) | val);
}

bool Mutex::hasWaiters() {
	return (value() >> 28) != 0;
}
void Mutex::hasWaiters(bool val) {
	value((value() & 0xBFFFFFFF) | ((int) val << 30));
}

void Mutex::guestRelease() {
	owner(0);
	signal();
}
