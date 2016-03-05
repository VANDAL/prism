# SIGIL2
See COPYING for license details

Sigil2 helps users capture **architecture-agnostic** events from an application.

Instead of x86, ARM, PPC, etc instructions, 
Sigil2 uses a straight-forward intermediate representation (IR) designed
to aid in system architecture studies. Sigil2 provides the **dynamic** execution
of an application in the form of 4 event types:
* **Compute** Events - IOPs & FLOPs
* **Memory** Events - data retrieval and storage
* **Synchronization** Events - task-level create, join, sync, etc
* **Context** Events - markers for basic blocks, subroutines, instructions, etc
* Control Flow support is planned upon necessity

Each event has specific attributes that are accessible in the [Sigil2 API](https://github.com/mdlui/Sigil2/wiki)

### Platform support
* C++11 compiler support
  * tested with g++ version 5.3.0
* cmake v3.0+
* dependencies of each frontend
  * Valgrind 3.11.0 support - http://valgrind.org/info/platforms.html
* 64-bit Linux has been officially tested; further 32-bit and ARM testing is planned

## Quick build instructions
```
$ git clone --recursive https://github.com/mdlui/Sigil2 
$ cd Sigil2
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=release ..
$ make -j
```

## Running Sigil2
### Sigil2 with Valgrind
####Multi-threaded Workload capture
Pthread and OpenMP support is available for the **applications** compiled with the following gcc versions:
* 4.9.2
* 5.1.0
* 5.3.0

This can be different than the gcc version used to compile Sigil2.   
Support for other gcc versions is contingent on whether or not symbols in the relevant libraries change.  
Required gcc headers are provided for convenience.   

Compile the function wrapper library and copy it to the `build` directory:  
```
$ pushd ../src/Sigil2/FrontEnds/Sigrind
$ my_gcc_ver="gcc-<your gcc version>"
$ gcc -Wall -I -g -DVGO_linux=1 -fPIC sglwrapper.c -I$my_gcc_ver -Ivalgrind-3.11.0-Sigil2/ -Ivalgrind-3.11.0-Sigil2/include -shared -o sglwrapper.so
$ cp sglwrapper.so ../../../../build/bin
$ popd
```

####Workload capture
The executable will be put in `build/bin`. It can be run in place, or the entire folder can be moved to a new location.  

Users supply at least 3 arguments to Sigil2:
* which frontend instrumentation tool is used to generate events
* which backend analysis tool is used to process events
* what application to profile

Example using Valgrind frontend and [SynchroTraceGen](http://ece.drexel.edu/faculty/taskin/wiki/vlsilab/index.php/SynchroTrace) backend:  
**Make sure an environment variable `TMPDIR` is set to a directory mounted as a tmpfs**. Sigil2 uses this for IPC.  
For example, on CentOS 7, the user should set `TMPDIR` to `/dev/shm`. By default, Sigil2 will set this to `/tmp`.

`$ TMPDIR="/dev/shm" bin/sigil2 --frontend=valgrind --backend=stgen --exec="myprogram --with --args"`

**For multi-threaded capture**, make sure you've [compiled the wrapper library as above](#multi-threaded-workload-capture):

## Developing for Sigil2
See the [wiki](https://github.com/mdlui/Sigil2/wiki)
