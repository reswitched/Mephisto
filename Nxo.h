#pragma once

#include "Ctu.h"

class Nxo {
public:
	Nxo(string fn);
	virtual ~Nxo() {}

	virtual guint load(Ctu &ctu, gptr base, bool relocate=false) = 0;

protected:
	ifstream fp;
	uint32_t length;
};

class Kip : Nxo {
public:
	Kip(string fn) : Nxo(fn) {}
	virtual ~Kip() override {}

	guint load(Ctu &ctu, gptr base, bool relocate=false) override;
};

class Nso : Nxo {
public:
	Nso(string fn) : Nxo(fn) {}
	virtual ~Nso() override {}

	guint load(Ctu &ctu, gptr base, bool relocate=false) override;
};

class Nro : Nxo {
public:
	Nro(string fn) : Nxo(fn) {}
	virtual ~Nro() override {}

	guint load(Ctu &ctu, gptr base, bool reloacte=false) override;
};
