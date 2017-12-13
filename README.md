# Mephisto
[![Build Status](https://travis-ci.org/reswitched/Mephisto.svg?branch=master)](https://travis-ci.org/reswitched/Mephisto)

## Dependencies

### All Platforms

ReSwitched unicorn fork:

```
git clone git@github.com:reswitched/unicorn.git
cd unicorn
UNICORN_ARCHS="aarch64" ./make.sh
sudo ./make.sh install
```

Python packages:

```
pip install -r requirements.txt
```

### Ubuntu

Install Clang 5 from the LLVM PPA: http://apt.llvm.org/

You may need to update libc++ as well, if you get tuple errors.

### OSX

Install llvm (will take a while)

```
brew install llvm --HEAD
```

Patch `Makefile`

```diff
diff --git a/Makefile b/Makefile
index e4c921b..4d53420 100644
--- a/Makefile
+++ b/Makefile
@@ -1,4 +1,4 @@
-CC := clang++-4.0
+CC := clang++
```

## Running

Much like the original CageTheUnicorn, the default use of Mephisto is via the load files.  Create a directory, e.g. `ns23`, and then copy in the NSO file(s).  Create a file inside this, called `load.meph` with the following format:

```
(load-nso "main" 0x7100000000)
(run-from 0x7100000000)
```

Running it is then as simple as:

```
./ctu ns23
```

Alternatively, you can pass a single NSO file on the command line:

```
./ctu --load-nso ns23/main
```

See help for other info, e.g. enabling GDB support.

### Run through Docker
First build the docker image, this may take some time

```bash
docker build -t reswitched/mephisto .
```

To run Mephisto it needs access to your NSO/NRO files, make sure to bind mount the location into the container.

__Example:__
```bash
docker run -ti --rm -p 24689:24689 -v $HOME:$HOME -u $UID reswitched/mephisto --load-nro $HOME/Coding/libtransistor/build/test/test_helloworld.nro
```

You can also create a bash alias.

```
alias ctu='docker run -ti --rm -p 24689:24689 -v $HOME:$HOME -u $UID reswitched/mephisto'
```

Now you can simply run `ctu` with your desired arguments. 
__Example:__
```bash
ctu --load-nro $HOME/Coding/libtransistor/build/test/test_helloworld.nro
```
