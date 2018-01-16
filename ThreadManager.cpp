#include "Ctu.h"

#define INSN_PER_SLICE 1000000

Thread::Thread(Ctu *_ctu, int _id) : ctu(_ctu), id(_id), started(false) {
	active = false;
	memset(&regs, 0, sizeof(ThreadRegisters));

	auto tlsSize = 0x1000;
	tlsBase = (1 << 24) + tlsSize * _id;
	ctu->cpu.map(tlsBase, tlsSize);
}

void Thread::assignHandle(uint32_t _handle) {
	handle = _handle;
	ctu->cpu.guestptr<uint32_t>(tlsBase + 0x3B8) = _handle;
}

void Thread::terminate() {
	signal();
	ctu->tm.terminate(id);
	auto tlsSize = 0x1000;
	ctu->cpu.unmap(tlsBase, tlsSize);
}

void Thread::suspend(function<void()> cb) {
	if(!active)
		return;

	active = false;
	if(id == ctu->tm.current()->id) {
		freeze();
		if(cb != nullptr)
			cb();
		ctu->tm.next(true);
	} else if(cb != nullptr)
		cb();
}

void Thread::resume(function<void()> cb) {
	if(active)
		return;

	if(cb != nullptr)
		onWake(cb);
	started = true;
	active = true;
	ctu->tm.enqueue(id);
}

void Thread::freeze() {
	ctu->cpu.storeRegs(regs);
	LOG_DEBUG(Thread, "Froze thread 0x%x with PC 0x" ADDRFMT, id, regs.PC);
}

void Thread::thaw() {
	ctu->cpu.tlsBase(tlsBase);
	if(wakeCallbacks.size()) {
		for(auto cb : wakeCallbacks)
			cb();
		wakeCallbacks.clear();
	}
	ctu->cpu.loadRegs(regs);
	LOG_DEBUG(Thread, "Thawed thread 0x%x with PC 0x" ADDRFMT, id, regs.PC);
}

void Thread::onWake(function<void()> cb) {
	wakeCallbacks.push_back(cb);
}

NativeThread::NativeThread(Ctu *_ctu, function<void()> _runner, int _id) : ctu(_ctu), runner(_runner), id(_id), active(false) {
}

void NativeThread::terminate() {
	ctu->tm.terminate(id);
}

void NativeThread::suspend() {
	active = false;
}

void NativeThread::resume() {
	if(active)
		return;

	active = true;
	ctu->tm.enqueue(id);
}

void NativeThread::run() {
	runner();
}

ThreadManager::ThreadManager(Ctu *_ctu) : ctu(_ctu) {
	threadId = 0;
	_current = nullptr;
	_last = nullptr;
	switched = false;
	first = true;
	wasNativeLast = false;
}

void ThreadManager::start() {
	next();
	while(true) {
		if(ctu->gdbStub.enabled) {
			ctu->gdbStub.handlePacket();
			if(ctu->gdbStub.remoteBreak) {
				ctu->gdbStub.remoteBreak = false;
				if(_current != nullptr)
					_current->freeze();
				ctu->gdbStub.sendSignal(SIGTRAP);
			}
			if(ctu->gdbStub.haltLoop && !ctu->gdbStub.stepLoop)
				continue;
			auto wasStep = ctu->gdbStub.stepLoop;
			if(_current == nullptr) {
				ctu->gdbStub.haltLoop = false;
				next();
				ctu->gdbStub.haltLoop = ctu->gdbStub.haltLoop || wasStep;
				continue;
			}
			ctu->cpu.exec(wasStep ? 1 : INSN_PER_SLICE);
			if(wasStep) {
				ctu->gdbStub.haltLoop = ctu->gdbStub.stepLoop = false;
				if (_current != nullptr)
					_current->freeze();
				ctu->gdbStub._break();
			}
		} else {
			next();
			ctu->cpu.exec(INSN_PER_SLICE);
		}
	}
}

shared_ptr<Thread> ThreadManager::create(gptr pc, gptr sp) {
	auto thread = make_shared<Thread>(ctu, ++threadId);
	thread->assignHandle(ctu->newHandle(thread));
	threads[thread->id] = thread;
	thread->regs.PC = pc;
	thread->regs.SP = sp;
	thread->regs.X30 = 0;
	return thread;
}

shared_ptr<NativeThread> ThreadManager::createNative(function<void()> runner) {
	auto thread = make_shared<NativeThread>(ctu, runner, ++threadId);
	nativeThreads[thread->id] = thread;
	return thread;
}

void ThreadManager::requeue() {
	if(_current == nullptr)
		return;

	_current->freeze();
	running.push_back(_current);
	_last = _current;
	_current = nullptr;
}

void ThreadManager::enqueue(int id) {
	if(isNative(id)) {
		for(auto other : runningNative)
			if(other->id == id)
				return;
		runningNative.push_back(nativeThreads[id]);
		return;
	}
	if(threads.find(id) == threads.end())
		return;
	enqueue(threads[id]);
}

void ThreadManager::enqueue(shared_ptr<Thread> thread) {
	for(auto other : running)
		if(other->id == thread->id)
			return;
	running.push_back(thread);
}

void ThreadManager::tryRunNative() {
	if(wasNativeLast)
		wasNativeLast = false;
	else {
		while(runningNative.size() > 0) {
			auto native = runningNative.front();
			runningNative.pop_front();
			if(!native->active)
				continue;
			native->run();
			if(native->active)
				runningNative.push_back(native);
			break;
		}
		wasNativeLast = true;
	}
}

void ThreadManager::next(bool force) {
	tryRunNative();

	if(force) {
		if(_current != nullptr)
			_last = _current;
		_current = nullptr;
	}

	shared_ptr<Thread> nt = nullptr;
	while(true) {
		if(running.size() == 0) {
			tryRunNative();
			if(ctu->gdbStub.enabled) {
				if(ctu->gdbStub.haltLoop) {
					ctu->cpu.stop();
					return;
				}
				ctu->gdbStub.handlePacket();
				if(ctu->gdbStub.haltLoop) {
					ctu->cpu.stop();
					return;
				}
			}
			if(_current == nullptr) {
				if(first) {
					LOG_DEBUG(Thread, "No threads left to run!");
					ctu->bridge.start();
				}
				first = false;
			} else
				return;
			usleep(50);
			continue;
		}
		nt = running.front();
		running.pop_front();
		if(nt->active)
			break;
	}

	switched = true;

	if(_current != nullptr) {
		_current->freeze();
		if(_current->active)
			enqueue(_current);
		_last = _current;
	}
	nt->thaw();
	_current = nt;
}

void ThreadManager::terminate(int id) {
	if(threads.find(id) == threads.end()) {
		if(isNative(id)) {
			nativeThreads[id]->active = false;
			nativeThreads.erase(id);
		}
		return;
	}
	threads[id]->active = false;
	threads.erase(id);
	next(true);
}

shared_ptr<Thread> ThreadManager::current() {
	return _current;
}

shared_ptr<Thread> ThreadManager::last() {
	return _last;
}

bool ThreadManager::setCurrent(int id) {
	auto thread = threads.find(id);
	if (thread == threads.end())
		return false;
	if(_current != nullptr) {
		_current->freeze();
		_last = _current;
	}
	_current = thread->second;
	_current->thaw();
	return true;
}


bool ThreadManager::isNative(int id) {
	return nativeThreads.find(id) != nativeThreads.end();
}

vector<shared_ptr<Thread>> ThreadManager::thread_list() {
	vector<shared_ptr<Thread>> vals;
	transform(threads.begin(), threads.end(), back_inserter(vals), [](auto val){return val.second;} );
	return vals;
}
