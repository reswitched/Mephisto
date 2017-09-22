#include "Ctu.h"

#define BETWEEN(x, a, b) (((x) >= (a)) && ((x) <= (b)))

string nextToken(string &code, int &off) {
	auto clen = code.length();
	while(off < clen && (code[off] == ' ' || code[off] == '\n' || code[off] == '\t' || code[off] == '\r'))
		++off;
	auto rlen = clen - off;
	auto orig = off;
	if(rlen <= 0)
		return "";
	if(code[off] == '(') {
		off++;
		return "(";
	} else if(code[off] == ')') {
		off++;
		return ")";
	} else if(rlen >= 3 && code[off] == '0' && code[off+1] == 'x') {
		int te;
		for(te = off + 2; te < clen; ++te) {
			if(code[te] == ' ' || code[te] == '\n' || code[te] == '\r' || code[te] == '\t' || code[te] == ')')
				break;
			assert(BETWEEN(code[te], '0', '9') || BETWEEN(code[te], 'a', 'f') || BETWEEN(code[te], 'A', 'F'));
		}
		off = te;
		return code.substr(orig, off - orig);
	} else if(BETWEEN(code[off], '0', '9')) {
		int te;
		for(te = off + 1; te < clen; ++te) {
			if(code[te] == ' ' || code[te] == '\n' || code[te] == '\r' || code[te] == '\t' || code[te] == ')')
				break;
			assert(BETWEEN(code[te], '0', '9'));
		}
		off = te;
		return code.substr(orig, off - orig);
	} else if(code[off] == '"') {
		string out = "\""; // Signal to the parser that this should be a String rather than Symbol
		for(++off ; off < clen && code[off] != '"'; ++off) {
			if(code[off] == '\\') {
			} else
				out += code[off];
		}
		assert(code[off++] == '"');
		return out;
	} else if(BETWEEN(code[off], '!', '\'') || BETWEEN(code[off], '*', '~')) {
		int te;
		for(te = off + 1; te < clen; ++te) {
			if(code[te] == ' ' || code[te] == '\n' || code[te] == '\r' || code[te] == '\t' || code[te] == ')')
				break;
			assert(BETWEEN(code[off], '!', '\'') || BETWEEN(code[off], '*', '~'));
		}
		off = te;
		return code.substr(orig, off - orig);
	}
	LOG_ERROR(Lisparser, "Unknown token at offset %i", off);
}

shared_ptr<Atom> parseLisp(string code) {
	auto off = 0;
	auto cur = make_shared<Atom>();
	forward_list<shared_ptr<Atom>> stack;
	while(true) {
		auto token = nextToken(code, off);
		if(token == "")
			break;
		else if(token == "(") {
			auto ne = make_shared<Atom>();
			cur->children.push_back(ne);
			stack.push_front(cur);
			cur = ne;
		} else if(token == ")") {
			cur = stack.front();
			stack.pop_front();
		} else if(token[0] == '"')
			cur->children.push_back(make_shared<Atom>(String, token.substr(1)));
		else if(token.length() >= 3 && token[0] == '0' && token[1] == 'x')
			cur->children.push_back(make_shared<Atom>(stoull(token.substr(2), nullptr, 16)));
		else if(BETWEEN(token[0], '0', '9'))
			cur->children.push_back(make_shared<Atom>(stoull(token, nullptr, 10)));
		else
			cur->children.push_back(make_shared<Atom>(Symbol, token));
	}
	assert(stack.empty());
	return cur;
}
