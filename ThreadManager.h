#pragma once
#include "Ctu.h"

typedef struct {
	float64_t low;
	float64_t high;
} float128;

typedef struct {
	guint SP, PC, NZCV;
	union {
		struct {
			guint gprs[31];
			float128 fprs[32];
		};
		struct {
			guint  X0,  X1,  X2,  X3, 
			       X4,  X5,  X6,  X7, 
			       X8,  X9, X10, X11, 
			      X12, X13, X14, X15, 
			      X16, X17, X18, X19, 
			      X20, X21, X22, X23, 
			      X24, X25, X26, X27, 
			      X28, X29, X30;
			float128   Q0,  Q1,  Q2,  Q3,
			           Q4,  Q5,  Q6,  Q7, 
			           Q8,  Q9, Q10, Q11, 
			          Q12, Q13, Q14, Q15, 
			          Q16, Q17, Q18, Q19, 
			          Q20, Q21, Q22, Q23, 
			          Q24, Q25, Q26, Q27, 
			          Q28, Q29, Q30, Q31;
		};
	};
} ThreadRegisters;

class Thread : public KObject {
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

	bool switched;

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
