# [SIGIL2](https://github.com/mdlui/Sigil2/wiki)
[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](./COPYING)

Provides an extensible interface to  **architecture-agnostic** application events.

<br>

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
* Linux
  * Tested
    * 64-bit **CentOS 7** (7.2.1511) on **x86\_64**
    * 64-bit **ArchLinux** (as of July 22, 2016) on **x86\_64**
  * In progress
    * 32-bit
    * ARM
* OS X/macOS
  * untested
* Windows
  * not supported
* Requirements
  * C++11 compiler support (g++ 4.8.5 and 6.1.1 tested)
  * CMake 2.8+
  * dependencies for frontends
    * Valgrind 3.11.0 support - http://valgrind.org/info/platforms.html
    * DynamoRIO support - https://github.com/DynamoRIO/dynamorio

## Example Usage
* Valgrind is the *default* frontend for generating events, if no option is specified
* [SynchroTraceGen](http://ece.drexel.edu/faculty/taskin/wiki/vlsilab/index.php/SynchroTrace) backend processing events into a special event trace  

`$ bin/sigil2 --backend=stgen --executable=./myprogram -with --args`

Users supply at least 2 arguments to Sigil2:
* the backend analysis tool used to process events
* the application

A third frontend argument can be supplied
* `--frontend=FRONTEND`
* `--frontend=dynamorio` is **_experimental_**

####Multithreaded Support
The Valgrind frontend has Pthread and OpenMP support for *applications* compiled with the following GCC versions:
* 4.9.2

This version can be different than the GCC version used to compile Sigil2.
Support for other GCC versions is possible, but contingent on whether or not symbols in the relevant libraries change.
Most pthread synchronization events *should* be captured with recent GCC versions,
however OpenMP synchronization events may not be captured. 

## Developing for Sigil2
See the [wiki](https://github.com/mdlui/Sigil2/wiki)
