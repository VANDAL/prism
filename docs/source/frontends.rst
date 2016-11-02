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

Options
^^^^^^^

.. todo:: options

::

  --num-threads=N


----
