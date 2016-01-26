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
Sigil2's base requires 
* C++11 compiler support
* cmake v3.0+
* and any dependencies of each frontend
and uses:
* https://github.com/philsquared/Catch
* https://github.com/gabime/spdlog

Separate support is required from Sigil2 frontends that instrument the application:
* Valgrind - 3.11.0 support - http://valgrind.org/info/platforms.html
* Planned frontends: TBD

## Building and Installing
Quick build instructions:
```
$ cd <sigil2 root directory>
$ mkdir build
$ cd build
$ cmake ..
$ make -j[jobs]
```

No installation is currently available. Typical use is to run in place.

## Running Sigil2
Users supply at least 3 arguments to Sigil2:
* which frontend instrumentation tool is used to generate events
* which backend analysis tool is used to process events
* what application to profile

Example using Valgrind frontend and [SynchroTraceGen](http://ece.drexel.edu/faculty/taskin/wiki/vlsilab/index.php/SynchroTrace) backend:

`$ ./sigil2 --frontend=vg --backend=STGen --exec="myprogram --with --args"`

## Developing for Sigil2
See the [wiki](https://github.com/mdlui/Sigil2/wiki)
