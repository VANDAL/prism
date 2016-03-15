# [SIGIL2](https://github.com/mdlui/Sigil2/wiki)

Captures **architecture-agnostic** events from applications.

## Install
```
$ git clone --recursive https://github.com/mdlui/Sigil2 
$ cd Sigil2
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=release ..
$ make -j<JOBS>
```
The executable will be put in `build/bin`. It can be run in place, or the folder can be moved to an install location.  

## What is it?

Uses a straight-forward intermediate representation (IR) for system architecture and application behavior studies.  
Sigil2 provides the *dynamic* behavior of an application with 4 event primitives:
* **Compute** - IOPs & FLOPs
* **Memory** - data access
* **Synchronization** - task-level create, join, sync, et al
* **Context** - markers for basic blocks, subroutines, instructions, etc
* Control Flow support is pending community demand

Each event has specific attributes that are accessible via the [Sigil2 API](https://github.com/mdlui/Sigil2/wiki/Event-API)

## Platform support
* Linux (64-bit CentOS 7 and ArchLinux tested on x86\_64)
  * 32-bit and ARM testing planned
* C++11 compiler support (g++ 4.8.5 and 5.3.0 tested)
* CMake 3.0+
* dependencies for frontends
  * Valgrind 3.11.0 support - http://valgrind.org/info/platforms.html

## Example Usage
* Valgrind frontend generating events
* [SynchroTraceGen](http://ece.drexel.edu/faculty/taskin/wiki/vlsilab/index.php/SynchroTrace) backend processing events into a special event trace  

`$ bin/sigil2 --frontend=valgrind --backend=stgen --executable=./myprogram -with --args`

Users supply at least 3 arguments to Sigil2:
* the frontend instrumentation tool used to generate events
* the backend analysis tool used to process events
* the application

####Multithreaded Support
The Valgrind frontend has Pthread and OpenMP support for *applications* compiled with the following GCC versions:
* 4.9.2

This version can be different than the GCC version used to compile Sigil2.  Support for other GCC versions is contingent on whether or not symbols in the relevant libraries change.  Most pthread synchronization events *should* be captured with recent GCC versions, however OpenMP synchronization events may not be captured. 

####Additional Notes

**Make sure** an environment variable **`TMPDIR` is set** to a directory mounted as a tmpfs.  Sigil2 uses this for IPC.  
For example, on CentOS 7, the user should set `TMPDIR` to `/dev/shm`. By default, Sigil2 will set this to `/tmp`:

`$ TMPDIR="/dev/shm" bin/sigil2 --frontend=valgrind --backend=stgen --executable=./myprogram -with --args`

## Developing for Sigil2
See the [wiki](https://github.com/mdlui/Sigil2/wiki)
