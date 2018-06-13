# C++ Parsing

See the README in .. for more information on the SynchroTraceGen CapnProto schemas.

This example will parse a SynchroTraceGen capnproto file.

## Requirements
* c++17 compiler support
* libkj and libcapnp installed (typically via a capnproto package)
* zlib-devel package

## Build
  ```
  $ mkdir build && cd build
  $ cmake ..
  $ make -j
  ```

* Generate a SynchroTraceGen trace {\*.capnp.bin,\*.capnp.bin.gz} with:

   `$ bin/prism --backend=stgen -l capnp --executable=...`

* Run the executable as:

   `$ ./stgenparser_[un]compressed sigil.events-#.[un]compressed.capnp.bin.gz`
