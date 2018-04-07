Backend Documentation
=====================

SimpleCount
------------

Synopsis
^^^^^^^^

::

$ bin/sigil2 --frontend=FRONTEND --backend=simplecount --executable=mybinary -myoptions


Description
^^^^^^^^^^^

SimpleCount is a demonstrative backend that counts each event type received
from a given frontend. These events are aggregated across all threads.

Options
^^^^^^^
No available options

----

.. _SynchroTraceGen:

SynchroTraceGen
---------------

Synopsis
^^^^^^^^

::

$ bin/sigil2 --frontend=FRONTEND --backend=stgen OPTIONS --executable=mybinary -myoptions

Description
^^^^^^^^^^^

SynchroTraceGen is a frontend for generating trace files for the SynchroTrace simulation framework.

Each thread detected by SynchroTraceGen is given its own output trace file, named ``sigil.events-#.out``.
By default, the output is directly compressed since the trace files can grow very large.

Options
^^^^^^^

|  -c `NUMBER`
|    Default: 100
|    Will compress all SynchroTraceGen `compute events`.
|    Each `compute event` will have a maximum of `NUMBER` local reads or writes
|
|  -o `PATH`
|    Default: '.'
|    All SynchroTraceGen output will be put in `PATH`
|
|  -l `{text,capnp,null}`
|    Default: 'text'
|    Choose which logging framework to use.
|    Regardless of which logger is chosen, a sigil.pthread.out and sigil.stats.out
|      file will be output.
|    'text'  will output an ASCII formatted trace in gzipped files.
|    'capnp' will output a packed CapnProto_ serialized trace in gzipped files.
|    'null'  will not output anything.

.. _CapnProto:
   https://capnproto.org/

----
