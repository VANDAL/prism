User Documentation
==================

.. _backends:

The Analysis Backend
--------------------


Getting Started with a Tool
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This example will demonstrate how to get started analyzing a workload.
Let's do a simple example that **counts each event type**.

First, let's make a new folder for our backend, called ``EventCounter``,
and begin making the backend.

.. code-block:: sh

   $ cd prism
   $ mkdir src/Backends/EventCounter
   $ touch src/Backends/EventCounter/EventCounter.hpp

Currently, all backends are created in C++, and inherit from a ``BackendIface`` class.

.. Typically it's easier to analyze a trace file than to directly analyze a
   workload. That is, it's easier to generate a trace and post-process it multiple
   times, instead of analyzing the application on-the-fly. Parsing a trace file
   containing relevant data is going to be faster and more straightforward than
   running a workload *multiple* times and having |project| filter all the potential
   metadata *repeatedly*.

.. code-block:: cpp

   // EventCounter.hpp

   #include "Core/Backends.hpp"

   class EventHandler : public BackendIface { };

By default, each event is ignored.
Let's ``override`` this behavior and keep count of how many memory events pass.

.. code-block:: cpp

   // EventCounter.hpp

   #include "Core/Backends.hpp"

   class EventHandler : public BackendIface
   {
       virtual void override onMemEv(const sigil2::MemEvent &ev) {
           memory_total++;
       }

       unsigned memory_total{0};
   };

We keep track of the total memory count in a private class variable, ``memory_total``.
If multiple event streams are enabled, a new class instance is created for each stream.

This means we won't be totalling events from the entire workload!
We'll use a naive approach is to use an atomic variable that all EventCounter instances can access.

.. code-block:: cpp

   // EventCounter.hpp

   #include "Core/Backends.hpp"
   #include <atomic>

   extern std::atomic<unsigned> global_memory_total;

   class EventHandler : public BackendIface
   {
       virtual void override onMemEv(const sigil2::MemEvent &ev) {
           global_memory_total++;
       }
   };


Now let's optimize our ``EventHandler`` to only update our atomic global
once at the end when the destructor is called, instead of at every memory event.
We'll also include the two extra functions:

1. an event ``requirements`` function, to let |project| know to generate memory events
2. a ``cleanup`` function, that executes after all event generation and event analysis has been performed.

We'll integrate these with |project| in :ref:`backendregistration`.

.. code-block:: sh

   $ touch src/Backends/EventCounter/EventCounter.cpp

.. code-block:: cpp

   // EventCounter.hpp

   #ifndef EVENTCOUNTER_H
   #define  EVENTCOUNTER_H

   #include "Core/Backends.hpp"
   #include <atomic>

   extern std::atomic<unsigned> global_memory_total;

   class EventHandler : public BackendIface
   {
       ~EventHandler() {
           global_memory_total += memory_total;
       }

       virtual void override onMemEv(const sigil2::MemEvent &ev) {
           memory_total++;
       }

       unsigned memory_total{0};
   };

   #endif

.. code-block:: cpp

   // EventCounter.cpp

   #include "EventCounter.hpp"
   #include <iostream>

   std::atomic<unsigned> global_memory_total{0};

   // Event Request
   sigil2::capabilities requirements()
   {
       using namespace sigil2;
       using namespace sigil2::capability;

       auto caps = initCaps();

       caps[MEMORY] = availability::enabled;

       return caps;
   }

   // Final Clean up call
   void cleanup()
   {
       std::cout << "Total Memory Events: " << global_memory_total << std::endl;
   }


.. _backendregistration:

Registering Your Analysis
~~~~~~~~~~~~~~~~~~~~~~~~~

Let's setup |project| to use the new tool.

.. todo:: simplecount example

          * registering as a static plugin
          * running
          * link to multithreaded event stream explanation

.. _frontends:

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


FAQ
---
