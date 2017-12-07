#pragma once
#include "Ctu.h"

class Thread : public Waitable {
public:
	Thread(Ctu *_ctu, int _id);
	void assignHandle(ghandle handle);
	void terminate();
	void suspend(function<void()> cb=nullptr);
	void resume(function<void()> cb=nullptr);
	void freeze();
	void thaw();

	void onWake(function<void()> cb);

	int id;
	bool started;
	ghandle handle;
	bool active;
	ThreadRegisters regs;
	gptr tlsBase;

private:
	Ctu *ctu;
	list<function<void()>> wakeCallbacks;
};

class NativeThread {
public:
	NativeThread(Ctu *_ctu, function<void()> _runner, int _id);
	void terminate();
	void suspend();
	void resume();

	void run();

	int id;
	bool active;

private:
	Ctu *ctu;
	function<void()> runner;
};

class ThreadManager {
public:
	ThreadManager(Ctu *_ctu);
	void start();
	shared_ptr<Thread> create(gptr pc=0, gptr sp=0);
	shared_ptr<NativeThread> createNative(function<void()> runner);
	void requeue();
	void enqueue(int id);
	void enqueue(shared_ptr<Thread> thread);
	void next(bool force=false);
	void terminate(int id);
	shared_ptr<Thread> current();
	shared_ptr<Thread> last();
	bool setCurrent(int id);
	bool switched;

	vector<shared_ptr<Thread>> thread_list();

private:
	void tryRunNative();
	bool isNative(int id);

	Ctu *ctu;
	unordered_map<int, shared_ptr<Thread>> threads;
	unordered_map<int, shared_ptr<NativeThread>> nativeThreads;
	int threadId;
	list<shared_ptr<Thread>> running;
	list<shared_ptr<NativeThread>> runningNative;
	shared_ptr<Thread> _current, _last;
	bool wasNativeLast;
	bool first;
};
