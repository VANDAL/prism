<p align="center">
  <img src="https://cdn.rawgit.com/mdlui/Sigil2/master/docs/sigil2-torus.png" alt="mmmmmm">
</p>
<p align="center"> <i>mmmmm...Sigil...</i>  </p>


[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](./COPYING)
[![Build Status](https://travis-ci.org/mikelui/Sigil2.svg?branch=master)](https://travis-ci.org/mikelui/Sigil2)
[![Docs Status](https://readthedocs.org/projects/sigil2/badge)](http://sigil2.readthedocs.io/en/latest)

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

Each event has specific attributes that are accessible via the Sigil2 API

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

## Platform support
| Linux                                          | OSX/macOS | Windows       |
| ---------------------------------------------- | --------- | ------------- |
| 64-bit **CentOS 7** (7.2.1511) on **x86\_64**  | untested  | not supported |
| In Progress: 32-bit                            |           |               |
| In Progress: ARM                               |           |               |

## Documentation
See the [docs](http://sigil2.readthedocs.io/en/latest)
