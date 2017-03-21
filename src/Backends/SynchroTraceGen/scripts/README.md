# Parsing SynchroTraceGen CapnProto Traces

The python scripts in this repo demonstrate parsing CapnpProto
event traces.

There are **two** different CapnProto trace formats:

* The ***compressed*** format is for the default case, where  multiple
reads and writes are aggregated into a single *event*. This effectively
compresses the trace, resulting in a smaller trace size, but loses ordering
information between reads and writes.

  * Additionally, addresses are only displayed once per aggregate event.
If the address is accessed multiple times within the aggregate event,
it will only appear once in the *event*.

* The ***uncompressed*** format allows only one read or write per event.
The event trace is larger but has more granularity. A separate CapnProto
schema is used for improved tracing speed and size.

:exclamation: The compression referred to here is a logical compression
of the trace. Additional zlib compression is used regardless on the traces.

## Requirements
* The {*STEventTraceCompressed.capnp*,*STEventTraceUncompressed.capnp*}
file must exist in the user's python sys.path:
  * Add its directory to the PYTHONPATH environmental variable or
copy it to the current working directory.

* The pycapnp library is required:
  * See http://jparyani.github.io/pycapnp/install.html for further details.

* Generate the \*.capnp.bin file with:
   `$ bin/sigil2 --backend=stgen -l capnp --executable=...`

* Run the script as:
    ./stgen_capnp_parser_compressed.py sigil.events-#.compressed.capnp.bin.gz
    ./stgen_capnp_parser_uncompressed.py sigil.events-#.uncompressed.capnp.bin.gz
