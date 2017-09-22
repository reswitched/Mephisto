#pragma once

#include "Ctu.h"

enum AtomType {
	Number, String, Symbol, List
};

class Atom {
public:
	Atom() {
		type = List;
	}
	Atom(AtomType _type, string val) {
		assert(_type == String || _type == Symbol);
		type = _type;
		strVal = val;
	}
	Atom(guint val) {
		type = Number;
		numVal = val;
	}
	AtomType type;
	guint numVal;
	string strVal;
	vector<shared_ptr<Atom>> children;
};

shared_ptr<Atom> parseLisp(string code);
