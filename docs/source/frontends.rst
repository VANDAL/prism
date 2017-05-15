Frontend Documentation
======================

Each frontend generates one or more event streams to a Sigil2 backend analysis
tool. Each frontend has it's own internal representation (IR) of events, so the process
of converting frontend IR to Sigil2 event primitives is different for each frontend.
For example, Valgrind will disassemble each machine instruction into multiple VEX IR
statements and expressions;
DynamoRIO annotates each instruction in a basic block with specific attributes;
the current Perf frontend only supports x86_64 decoding via the Intel XED library.


Valgrind
--------

Synopsis
^^^^^^^^

::

$ bin/sigil2 --frontend=valgrind OPTIONS --backend=BACKEND --executable=mybinary -myoptions

Description
^^^^^^^^^^^

Uses a heavily modified Callgrind tool, *Sigrind*, to observe |project| *event
primitives* and pass them to the backend.  Valgrind serializes all threads in
the target executable, so only one thread's event stream is passed to the
backend at a time. A context switch is signaled with a |project| context event.
Because threads are serialized by Valgrind, the target executable is mostly
deterministic.

Options
^^^^^^^

| --at-func=\ `FUNCTION_NAME`
|   Default: (NULL)
|
| --start-func=\ `FUNCTION_NAME`
|   Default: (NULL)
|   Start collecting events at `FUNCTION_NAME`
|   If (NULL), then start from beginning of execution
|
| --stop-func=\ `FUNCTION_NAME`
|   Default: (NULL)
|   Stop collecting events at `FUNCTION_NAME`
|   If (NULL), then stop at the end of execution
|
| --gen-mem={`yes,no`}
|   Default: yes
|   Generate memory events to Sigil2
|
| --gen-comp={`yes,no`}
|   Default: yes
|   Generate compute events to Sigil2
|
| --gen-cf={`yes,no`}
|   Default: no
|   Currently unsupported
|
| --gen-sync={`yes,no`}
|   Default: yes
|   Generate synchronization (thread) events to Sigil2
|
| --gen-instr={`yes,no`}
|   Default: yes
|   Generate ISA instructions to Sigil2
|   Only instruction addresses are currently supported
|
| --gen-bb={`yes,no`}
|   Default: no
|   Currently unsupported
|
| --gen-fn={`yes,no`}
|   Default: no
|   Sends function enter/exit events along with the function name
|   Be sure to compile with less optimizations and debug flags for best results
|


Multithreaded Application Support
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Valgrind frontend automatically supports *synchronization events* in
applications that use the **POSIX threads** library and/or the **OpenMP**
library by intercepting relevant API calls.

Pthreads
~~~~~~~~

Pthreads should be supported for most versions of GCC/libc, because the Pthread
API is quite stable.

Pthreads support exists for any application dynamically linked to the Pthreads
library.

See `Static Library Support`_ for applicatons that are statically linked.

OpenMP
~~~~~~

Only **GCC 4.9.2** is officially supported for synchronization event capture,
because the implementation of the library is more likely to change between GCC versions.

Dynamically linked OpenMP applications are not supported.
Only `Static Library Support`_ exists.

Static Library Support
~~~~~~~~~~~~~~~~~~~~~~

Applications that use a static Pthreads or OpenMP library must be manually linked with the
sigil2-valgrind wrapper archive.
This can be found in ``BUILD_DIR/bin/libsglwrapper.a``.

For example: ::

$CC $CFLAGS main.c -Wl,--whole-archive $BUILD_DIR/bin/libsglwrapper.a -Wl,--no-whole-archive

----

DynamoRIO
---------

Synopsis
^^^^^^^^

::

$ bin/sigil2 --num-threads=N --frontend=dynamorio OPTIONS --backend=BACKEND --executable=mybinary -myoptions

Description
^^^^^^^^^^^

.. note:: -DDYNAMORIO_ENABLE=ON must be passed to **cmake** during configuration to
          build with DynamoRIO support.

DynamoRIO is a cross-platform dynamic binary instrumentation tool. DynamoRIO runs multithreaded
applications natively. This makes results less reproducible than Valgrind, however analysis is
potentially faster on a multi-core architecture. This enables multiple event streams to be
processed at once, by setting --num-threads > 1.

Options
^^^^^^^

.. todo:: options

::

  --num-threads=N


----

Intel Process Trace
-------------------

Synopsis
^^^^^^^^

::

$ bin/sigil2 --frontend=perf --backend=BACKEND --executable=perf.data

Description
^^^^^^^^^^^

.. note:: -DPERF_ENABLE=ON must be passed to **cmake** during configuration to
          build with Perf PT support.

Intel Process Trace is a new CPU feature available on Intel processors that are
Broadwell or more recent. The trace is captured via branch results. The entire
trace is then reconstructed by perf by replaying the binary, including all shared
library loading and context switches. A side effect of only capturing branch results
is that all runtime information within the trace is lost, such as some memory access
addresses; e.g. the Perf 'replay' mechanism does not support replaying malloc results.

For more usage details, see: `perf design document for Intel PT`_

For more technical details see: `Intel Software Developer's Manual Volume Three`_

.. _perf design document for Intel PT:
   https://github.com/torvalds/linux/blob/master/tools/perf/Documentation/intel-pt.txt

.. _Intel Software Developer's Manual Volume Three:
   https://software.intel.com/en-us/articles/intel-sdm

Options
^^^^^^^

.. note:: The ``perf.data`` file is generated with: ``perf record -e intel_pt//u ./myexec``

          If you receive '*AUX data lost N times out of M!*', try increasing the size of the AUX
          buffer. Otherwise a significant of the portion of the trace may not be reproduced:
          ``perf record -m,AUXTRACE_PAGES -e intel_pt//u ./myexec``

.. todo:: options

----
