# Python Parsing

See the README in .. for more information on the SynchroTraceGen CapnProto schemas.

These scripts will parse SynchroTraceGen traces via python.
**Note** that python is substantially slower at parsing traces
than the alternative C++ and Rust parser examples.

## Requirements
* The {*STEventTraceCompressed.capnp*,*STEventTraceUncompressed.capnp*}
file must exist in the user's python sys.path:
  * Add its directory to the PYTHONPATH environmental variable or
copy it to the current working directory.

* The pycapnp library is required:
  * See http://jparyani.github.io/pycapnp/install.html for further details.

  `$ pip install pycapnp`

* Generate a SynchroTraceGen trace {\*.capnp.bin,\*.capnp.bin.gz} with:

   `$ bin/prism --backend=stgen -l capnp --executable=...`

* Run the script as:

   ```
   $ export PYTHONPATH=$(readlink -f ../../STEventTraceSchemas) # example
   $ ./stgen_capnp_parser_compressed.py sigil.events-#.compressed.capnp.bin.gz
   $ ./stgen_capnp_parser_uncompressed.py sigil.events-#.uncompressed.capnp.bin.gz
   ```
