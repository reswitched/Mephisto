#include "Ctu.h"

struct Arg: public option::Arg
{
  static void printError(const char* msg1, const option::Option& opt, const char* msg2)
  {
    fprintf(stderr, "%s", msg1);
    fwrite(opt.name, opt.namelen, 1, stderr);
    fprintf(stderr, "%s", msg2);
  }
  static option::ArgStatus Unknown(const option::Option& option, bool msg)
  {
    if (msg) printError("Unknown option '", option, "'\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg != nullptr)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires an argument\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != nullptr && option.arg[0] != 0)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires a non-empty argument\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = nullptr;
    if (option.arg != nullptr && strtol(option.arg, &endptr, 10)){};
    if (endptr != option.arg && *endptr == 0)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires a numeric argument\n");
    return option::ARG_ILLEGAL;
  }
};

enum  optionIndex { UNKNOWN, HELP, ENABLE_GDB, PORT, NSO, NRO, KIP, ENABLE_SOCKETS, RELOCATE, INITIALIZE_MEMORY };
const option::Descriptor usage[] =
{
	{UNKNOWN, 0, "", "",Arg::None, "USAGE: ctu [options] <load-directory>\n\n"
	                                       "Options:" },
	{HELP, 0,"","help",Arg::None, "  --help  \tUnsurprisingly, print this message." },
	{ENABLE_GDB, 0,"g","enable-gdb",Arg::None, "  --enable-gdb, -g  \tEnable GDB stub." },
	{PORT, 0,"p","gdb-port",Arg::Numeric, "  --gdb-port, -p  \tSet port for GDB; default 24689." },
	{NSO, 0,"","load-nso",Arg::NonEmpty, "  --load-nso  \tLoad an NSO without load directory"},
	{NRO, 0,"","load-nro",Arg::NonEmpty, "  --load-nro  \tLoad an NRO without load directory (entry point .text+0x80)"},
	{KIP, 0,"","load-kip",Arg::NonEmpty, "  --load-kip  \tLoad a KIP without load directory"},
	{ENABLE_SOCKETS, 0, "b","enable-sockets",Arg::None, "  --enable-sockets, -b  \tEnable BSD socket passthrough." },
	{RELOCATE, 0, "r","relocate",Arg::None, "  --relocate, -r  \tRelocate loaded NRO files" },
	{INITIALIZE_MEMORY, 0, "m", "initialize-memory", Arg::None, "  --initialize-memory, -m  \tInitialize memory to help catch uninitialized memory bugs" },
	{0,0,nullptr,nullptr,nullptr,nullptr}
};

bool exists(string fn) {
	struct stat buffer;
	return stat(fn.c_str(), &buffer) == 0;
}

void loadKip(Ctu &ctu, const string &lfn, gptr raddr) {
	assert(exists(lfn));
	Kip file(lfn);
	if(file.load(ctu, raddr, false) == 0) {
		LOG_ERROR(KipLoader, "Failed to load %s", lfn.c_str());
	}
	ctu.loadbase = min(raddr, ctu.loadbase);
	auto top = raddr + 0x100000000;
	ctu.loadsize = max(top - ctu.loadbase, ctu.loadsize);
	LOG_INFO(KipLoader, "Loaded %s at " ADDRFMT, lfn.c_str(), ctu.loadbase);
}

void loadNso(Ctu &ctu, const string &lfn, gptr raddr) {
	assert(exists(lfn));
	Nso file(lfn);
	if(file.load(ctu, raddr, false) == 0) {
		LOG_ERROR(NsoLoader, "Failed to load %s", lfn.c_str());
	}
	ctu.loadbase = min(raddr, ctu.loadbase);
	auto top = raddr + 0x100000000;
	ctu.loadsize = max(top - ctu.loadbase, ctu.loadsize);
	LOG_INFO(NsoLoader, "Loaded %s at " ADDRFMT, lfn.c_str(), ctu.loadbase);
}

void loadNro(Ctu &ctu, const string &lfn, gptr raddr, bool relocate) {
	assert(exists(lfn));
	Nro file(lfn);
	if(file.load(ctu, raddr, relocate) == 0) {
		LOG_ERROR(NroLoader, "Failed to load %s", lfn.c_str());
	}
	ctu.loadbase = min(raddr, ctu.loadbase);
	auto top = raddr + 0x100000000;
	ctu.loadsize = max(top - ctu.loadbase, ctu.loadsize);
	LOG_INFO(NroLoader, "Loaded %s at " ADDRFMT, lfn.c_str(), ctu.loadbase);
}

void runLisp(Ctu &ctu, const string &dir, shared_ptr<Atom> code) {
	assert(code->type == List && code->children.size() >= 1);
	auto head = code->children[0];
	assert(head->type == Symbol);
	if(head->strVal == "load-nso") {
		assert(code->children.size() == 3);
		auto fn = code->children[1], addr = code->children[2];
		assert(fn->type == String && addr->type == Number);
		auto raddr = addr->numVal;
		auto lfn = dir + "/" + fn->strVal;
		loadNso(ctu, lfn, raddr);
	} else if(head->strVal == "load-kip") {
		assert(code->children.size() == 3);
		auto fn = code->children[1], addr = code->children[2];
		assert(fn->type == String && addr->type == Number);
		auto raddr = addr->numVal;
		auto lfn = dir + "/" + fn->strVal;
		loadKip(ctu, lfn, raddr);
	} else if(head->strVal == "run-from") {
		assert(code->children.size() == 2 && code->children[1]->type == Number);
		ctu.execProgram(code->children[1]->numVal);
	} else
		LOG_ERROR(Main, "Unknown function in load script: '%s'", head->strVal.c_str());
}

int main(int argc, char **argv) {
	argc -= argc > 0;
	argv += argc > 0;

	option::Stats stats(usage, argc, argv);
	vector<option::Option> options(stats.options_max);
	vector<option::Option> buffer(stats.buffer_max);
	option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

	if(parse.error()) {
		return 1;
	} else {
		if(options[HELP].count() || options[UNKNOWN].count()) {
		  printUsage:
			option::printUsage(cout, usage);
			return 0;
		}
		if(options[NSO].count() > 0) {
			if(options[NSO].count() != 1) { goto printUsage; }
			if(options[NRO].count() != 0) { goto printUsage; }
			if(options[KIP].count() != 0) { goto printUsage; }
			if(parse.nonOptionsCount() != 0) { goto printUsage; }
		} else if(options[NRO].count() > 0) {
			if(options[NRO].count() != 1) { goto printUsage; }
			if(options[NSO].count() != 0) { goto printUsage; }
			if(options[KIP].count() != 0) { goto printUsage; }
			if(parse.nonOptionsCount() != 0) { goto printUsage; }
		} else if(options[KIP].count() > 0) {
			if(options[KIP].count() != 1) { goto printUsage; }
			if(options[NSO].count() != 0) { goto printUsage; }
			if(options[NRO].count() != 0) { goto printUsage; }
			if(parse.nonOptionsCount() != 0) { goto printUsage; }
		} else {
			if(parse.nonOptionsCount() != 1) { goto printUsage; }
		}
	}

	Ctu ctu;
	ctu.loadbase = 0xFFFFFFFFFFFFFFFF;
	ctu.loadsize = 0;

	if(options[ENABLE_GDB].count()) {
		ctu.gdbStub.enable(options[PORT].count() == 0 ? 24689 : (uint16_t) atoi(options[PORT][0].arg));
	} else
		assert(options[PORT].count() == 0);

	if(options[ENABLE_SOCKETS].count()) {
		ctu.socketsEnabled = true;
	} else {
		ctu.socketsEnabled = false;
	}

	ctu.initializeMemory = options[INITIALIZE_MEMORY].count() > 0;
	
	bool relocate = false;
	if(options[RELOCATE].count()) {
		relocate = true;
	}
	
	if(options[NSO].count()) {
		loadNso(ctu, options[NSO][0].arg, 0x7100000000);
		ctu.execProgram(0x7100000000);
	} else if(options[NRO].count()) {
		loadNro(ctu, options[NRO][0].arg, 0x7100000000, relocate);
		ctu.execProgram(0x7100000000);
	} else if(options[KIP].count()) {
		loadKip(ctu, options[KIP][0].arg, 0x7100000000);
		ctu.execProgram(0x7100000000);
	} else {
		string dir = parse.nonOption(0);
		auto lfn = dir + "/load.meph";
		if(!exists(lfn))
			LOG_ERROR(Main, "File does not exist: %s", lfn.c_str());
		auto fp = ifstream(lfn);
		auto code = string(istreambuf_iterator<char>(fp), istreambuf_iterator<char>());

		auto atom = parseLisp(code);
		assert(atom->type == List);
		for(auto elem : atom->children)
			runLisp(ctu, dir, elem);
	}

	return 0;
}
