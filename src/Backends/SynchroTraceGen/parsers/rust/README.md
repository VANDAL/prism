# Rust Parsing

See the README in .. for more information on the SynchroTraceGen CapnProto schemas.

This example will parse a SynchroTraceGen capnproto file.

## Requirements
* rustc v1.26+

## Build
  ```
  $ cd parse_stgen_capnp
  $ cargo build # or cargo build --release
  ```

* Generate a SynchroTraceGen trace {\*.capnp.bin,\*.capnp.bin.gz} with:

   `$ bin/sigil2 --backend=stgen -l capnp --executable=...`

* Run:

   `$ cargo run [--release] --bin parse_[un]compressed sigil.events-#.[un]compressed.capnp.bin.gz`
