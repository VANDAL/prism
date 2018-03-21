The Profiling Frontend
----------------------

A *frontend* is the component that is generating the event stream.
By default, this is Valgrind (mostly due to historical reasons).

.. include:: <isonum.txt>

While it's tempting to assume that the event generation *just works*\ |trade|
you should be aware of the intrinsic nature of the chosen frontend before
making any large assumptions.

Valgrind
~~~~~~~~
Valgrind is the default frontend. No additional options are required. The
following two command lines are equivalent.

.. code-block:: none

   $ bin/sigil2 --backend=simplecount --executable=ls -lah
   $ bin/sigil2 --frontend=valgrind --backend=simplecount --executable=ls -lah

Valgrind is a *copy & annotate* dynamic binary instrumentation tool. This means
that the dynamic instruction stream is grouped into blocks, disassembled into
Valgrind's VEX IR, instrumented, and then recompiled just-in-time.

DynamoRIO
~~~~~~~~~
DynamoRIO is not built with |project| by default. To enable DynamoRIO as a frontend,
build |project| using the following cmake build command:

.. code-block:: none

   $ cmake .. -DCMAKE_BUILD_TYPE=release -DENABLE_DRSIGIL:bool=true

DynamoRIO can now be invoked as a frontend:

.. code-block:: none

   $ bin/sigil2 --frontend=dynamorio --backend=simplecount --executable=ls -lah

DynamoRIO's IR exists closer to the ISA than the IR used by Valgrind.
|project| converts DynamoRIO IR to event primitives by inspection of each
opcode.

.. todo:: mmm475 to fill in more details


Future
~~~~~~
Additional frontends being explored include:

* LLVM-tracer
* Contech
* GPU Ocelot
