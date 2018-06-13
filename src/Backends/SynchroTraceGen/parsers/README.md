# Parsing SynchroTraceGen CapnProto Traces

These subprojects demonstrate parsing CapnpProto event traces.
CapnProto traces can be generated with the `--backend=stgen -l capnp` option.
These traces are generally smaller and faster to parse through than the default ASCII text traces.

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
