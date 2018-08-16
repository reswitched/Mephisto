CXX := clang++-5.0
PYTHON2 := python2
CPP_FILES := $(wildcard *.cpp)
IPCIMPL_FILES := $(wildcard ipcimpl/*.cpp)
H_FILES := $(wildcard *.h)
ID_FILES := $(wildcard ipcdefs/*.id)
CXX_FLAGS := -std=c++1z -I. -isystem unicorn/include/ -Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-header-hygiene -Wno-shadow-field-in-constructor -Wno-old-style-cast -Wno-missing-prototypes -Wno-unused-parameter -Wno-padded -Wno-sign-conversion -Wno-sign-compare -Wno-shadow-uncaptured-local -Wno-weak-vtables -Wno-switch -Wno-unused-variable -Wno-unused-private-field -Wno-variadic-macros -Wno-unused-macros -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-reorder -Wno-missing-noreturn -Wno-unreachable-code -Wno-gnu-zero-variadic-macro-arguments -Wno-cast-align -DLZ4_DISABLE_DEPRECATE_WARNINGS -Wno-undefined-func-template -Wno-format-nonliteral -Wno-documentation-unknown-command $(EXTRA_CXX_FLAGS)
LD_FLAGS := -lpthread -llz4 $(EXTRA_LD_FLAGS)

OBJ_FILES := $(CPP_FILES:.cpp=.o) $(IPCIMPL_FILES:.cpp=.o)

all: ctu

SwIPC/ipcdefs/auto.id: SwIPC/scripts/genallipc.py
	cd SwIPC && $(PYTHON2) scripts/genallipc.py

IpcStubs.h: $(ID_FILES) $(IPCIMPL_FILES) SwIPC/ipcdefs/auto.id generateIpcStubs.py SwIPC/idparser.py partialparser.py
	$(PYTHON2) generateIpcStubs.py

%.pch: % IpcStubs.h $(H_FILES)
	$(CXX) -x c++-header $(CXX_FLAGS) -o $@ $<

ctu: Ctu.h.pch $(OBJ_FILES) $(H_FILES) unicorn/libunicorn.a
	$(CXX) -o ctu $(OBJ_FILES) $(LD_FLAGS) unicorn/libunicorn.a

Ipc.o: Ipc.cpp Ctu.h.pch
	$(CXX) $(CXX_FLAGS) -c -g -o $@ $<

%.o: %.cpp Ctu.h.pch
	$(CXX) $(CXX_FLAGS) -include Ctu.h -c -g -o $@ $<

unicorn/libunicorn.a: unicorn/make.sh
	cd unicorn/ && UNICORN_QEMU_FLAGS="--python=$(PYTHON2)" UNICORN_ARCHS="aarch64" ./make.sh

clean:
	rm -f *.o
	rm -f *.pch
	rm -f ipcimpl/*.o
	rm -f ctu
	$(MAKE) -C unicorn clean
