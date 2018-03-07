[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](./COPYING)
[![Build Status](https://travis-ci.org/vandal/prism.svg?branch=master)](https://travis-ci.org/vandal/prism)
[![Docs Status](https://readthedocs.org/projects/prism/badge)](http://prism.readthedocs.io/en/latest)

The intuitive, event-driven application profiling and characterization framework.

<br>

## Install
:boom: C++14 compiler support is required :boom:  
:boom: cmake v3 required :boom:  
See [CentOS 7 Support](#centos-7-support)
```
git clone https://github.com/vandal/prism
cd prism
mkdir build && cd build
cmake ..
make -j
```
The executable will be put in `build/bin`. It can be run in place, or the folder can be moved to an install location.

## What is it?

Uses a straight-forward intermediate representation (IR) for system architecture and application behavior studies.  
Prism provides the *dynamic* behavior of an application with 4 event primitives:
* **Compute** - IOPs & FLOPs
* **Memory** - data access
* **Synchronization** - task-level create, join, sync, et al
* **Context** - markers for basic blocks, subroutines, instructions, etc
* Control Flow support is pending community demand

Each event has specific attributes that are accessible via the Prism API

## Example Usage
* Valgrind is the *default* frontend for generating events, if no option is specified
* [SynchroTraceGen](http://vlsi.ece.drexel.edu/index.php?title=SynchroTrace) backend processing events into a special event trace  

`$ bin/prism --backend=stgen --executable=./myprogram -with --args`

Users supply at least 2 arguments to Prism:
* the backend analysis tool used to process events
* the application

A third frontend argument can be supplied
* `--frontend=FRONTEND`
* `--frontend=dynamorio` is **_experimental_**

## Platform support
| Linux                              | OSX/macOS | Windows       |
| ---------------------------------- | --------- | ------------- |
| 64-bit **CentOS 7** on **x86\_64** | untested  | not supported |
| YMMV: ARM                          |           |               |

### CentOS 7 Support
```
sudo yum install epel-release
sudo yum install centos-release-scl
sudo yum install cmake3 devtoolset-6
scl enable devtoolset-6 bash
# use cmake3 and build and usual
```
See [Software Collections](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/) for details.

## Documentation
[Read the docs](http://vandal-prism.readthedocs.io/en/latest)
