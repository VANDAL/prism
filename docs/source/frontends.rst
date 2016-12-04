Frontend Documentation
======================



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
|   Currently unsupported
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
Sigil2-Valgrind wrapper archive.
This can be found in ``BUILD_DIR/bin/libsglwrapper.a``.

----

DynamoRIO
---------------

Synopsis
^^^^^^^^

::

$ bin/sigil2 --num-threads=N --frontend=dynamorio OPTIONS --backend=BACKEND --executable=mybinary -myoptions

Description
^^^^^^^^^^^

.. note:: Custom options must be passed to **cmake** during configuration to
          build with DynamoRIO support.

**Experimental DynamioRIO support is temporarily disabled due to internal
refactoring to improve resource usage and accuracy.**

Options
^^^^^^^

.. todo:: options

::

  --num-threads=N


----
