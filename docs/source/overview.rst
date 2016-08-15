Overview
========

|project| is a framework designed to help analyze the dynamic behavior of
applications. We call this dynamic behavior, with its given inputs and state,
the **workload**. Having this workload is very useful for debugging,
performance profiling, and simulation on hardware. |project| was born from
the need to generate application traces for `trace-driven simulation`_,
so low-level, detailed traces are the primary use-case.

.. _`trace-driven simulation`: https://en.wikipedia.org/wiki/Microarchitecture_simulation

Workloads
---------

A workload can be represented in many ways.

...you might represent a workload as a simple assembly instruction trace:

.. code-block:: gas

   push   %rbp
   push   %rbx
   mov    %rsi,%rbp
   mov    %edi,%ebx
   sub    $0x8,%rsp
   callq  4377b0 <_Z17myfuncv>
   callq  4261e0 <_ZN5myotherfunc>
   mov    %rbp,%rdx
   mov    %ebx,%esi
   mov    %rax,%rdi
   callq  422460 <_ZN5GO>
   add    $0x8,%rsp
   xor    %eax,%eax
   pop    %rbx
   pop    %rbp
   retq

.. todo:: ...or you might want some type of call graph:

          ...or something else...


Event Primitives
^^^^^^^^^^^^^^^^

Because of the variety of use-cases for analyzing workloads,
|project| decided to present workloads as a set of extensible primitives.

+-----------------+-----------------------------------------+
| Event Primitive | Description                             |
+=================+=========================================+
| Compute         | some transformation of data             |
+-----------------+-----------------------------------------+
| Memory          | some movement of data                   |
+-----------------+-----------------------------------------+
| Synchronization | ordering between separate event streams |
+-----------------+-----------------------------------------+
| Context         | grouping of events                      |
+-----------------+-----------------------------------------+

A serializable format is not defined, but an example abstraction could look
like: ::

  ...
  compute     FLOP,   add,   SIMD4
  memory      write,  4B,    <addr1>
  memory      read,   16B,   <addr2>
  context     func,   enter, hello_world_thread
  sync        create, <TID1>
  ...

.. todo:: More detail is discussed futher in ???

Event Generation
----------------

Many tools exist to capture workloads:

* static instrumentation tools

  * PEBIL_
  * LLVM_ (e.g. Contech_)

* :abbr:`dynamic binary instrumentation (DBI)` tools

  * Valgrind_
  * DynamoRIO_
  * PIN_
  * GPGPU specific

* :abbr:`hardware performance counter (HPC)` sampling

  * architecture-specific

* simulation probes

  * gem5_
  * SniperSim_
  * Multi2Sim_

* and others

.. _PEBIL: http://www.sdsc.edu/pmac/tools/pebil.html
.. _LLVM: http://llvm.org
.. _Contech: http://bprail.github.io/contech/
.. _Valgrind: http://valgrind.org
.. _DynamoRIO: http://dynamorio.org
.. _PIN: https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool
.. _gem5: http://www.gem5.org/Main_Page
.. _SniperSim: http://snipersim.org/
.. _Multi2Sim: http://www.multi2sim.org/

Each tool has its merits depending on the desired granularity
and source of the event trace. Execution-driven simulators are great for
fine-grained, low-level traces, but may be impractical for a large workload.
Most DBI tools do a good job of obvserving the instruction stream of general
purpose CPU workloads, but may not be useful when looking at workloads that
use peripheral devices like GPUs or third-party IP.

|project| recognizes this and creates an abstraction to the underlying
tool that observes the workload. Events are *translated* into |project|
*event primitives* that are then presented to the user for further processing.
The tool used for event generation is a |project| **frontend**, and the
user-defined processing on those events is a |project| **backend**.
Currently, backends are written as C++ static plugins to |project|,
although there is room for expansion, given enough interest.
