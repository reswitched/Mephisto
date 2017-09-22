#pragma once

#include "Ctu.h"

class KObject {
public:
	virtual ~KObject() {}
	virtual void close() {}
};

class Process : public KObject {
public:
	Process(Ctu *_ctu) : ctu(_ctu) {}

private:
	Ctu *ctu;
};

class FauxHandle : public KObject {
public:
	FauxHandle(uint32_t _val) : val(_val) {}

	uint32_t val;
};
