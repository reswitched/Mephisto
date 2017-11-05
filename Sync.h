#pragma once

#include "Ctu.h"

class Waitable : public KObject {
public:
	Waitable();

	void acquire();
	void release();

	void wait(function<int()> cb);
	virtual void wait(function<int(bool)> cb);

	void signal(bool one=false);
	void cancel();

	virtual void close() override {}

protected:
	virtual bool presignalable() { return true; }

private:
	recursive_mutex lock;
	list<function<int(bool)>> waiters;
	bool presignaled, canceled;
};

class InstantWaitable : public Waitable {
public:
	virtual void wait(function<int(bool)> cb) {
		Waitable::wait(cb);
		signal(true);
	}
};

class Semaphore : public Waitable {
public:
	Semaphore(Guest<uint32_t> _vptr);

	void increment();
	void decrement();

	uint32_t value();

protected:
	bool presignalable() override { return false; }

private:
	Guest<uint32_t> vptr;
};

class Mutex : public Waitable {
public:
	Mutex(Guest<uint32_t> _vptr);

	uint32_t value();
	void value(uint32_t val);
	
	ghandle owner();
	void owner(ghandle val);

	bool hasWaiters();
	void hasWaiters(bool val);

	void guestRelease();

protected:
	bool presignalable() override { return false; }

private:
	Guest<uint32_t> vptr;
};
