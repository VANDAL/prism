Building your first Prism tool
==============================

This example will demonstrate how to get started analyzing a workload.
We'll generate a simple tool that counts the number of memory events in a workload.

Writing Your Tool
~~~~~~~~~~~~~~~~~

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

.. code-block:: sh

   $ touch src/Backends/EventCounter/EventCounter.cpp

.. code-block:: cpp

   // EventCounter.hpp

   #ifndef EVENTCOUNTER_H
   #define EVENTCOUNTER_H

   #include "Core/Backends.hpp"
   #include <atomic>

   // forward function declarations
   void cleanup(void);
   sigil2::capabilities requirements(void);

   // global memory event counter
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

Registering Your Tool
~~~~~~~~~~~~~~~~~~~~~

Let's setup our new tool in |project|.
|project| uses static plugins at the moment.
This requires altering a bit of |project| source code, but is easier to maintain as a small project.

.. code-block:: sh

   $ cd src/Core
   $ $EDITOR main.cpp

.. code-block:: cpp

   // main.cpp

   int main(int argc, char* argv[])
   {
       auto config = Config()
           .registerFrontend(/* ... */)
           // register more frontends
           .registerBackend(/* ... */)
           // register more backends
           .parseCommandLine(argc, argv);
       return startPrism(config);
   }

We can see all enabled backends and frontends here in one spot.
This is clear and efficient when working with a smaller number of tools.
Let's register our backend.

.. code-block:: cpp

   // main.cpp

   int main(int argc, char* argv[])
   {
       auto config = Config()
           .registerFrontend(/* ... */)
           // register more frontends
           .registerBackend(/* ... */)
           // register more backends
           .registerBackend("EventCounter",
                            {[]{return std::make_unique<::EventHandler>();},
                             {},
                             ::cleanup,
                             ::requirements})
           .parseCommandLine(argc, argv);
       return startPrism(config);
   }

The ``registerBackend`` member function takes 5 arguments:

1. The name of the tool---this is used in the command line option.
#. A function that returns a new instance of our event handler---we'll use an anonymous function.
#. A function to take any extra command line options---we aren't using this so it'll stay blank.
#. An end function that is called after all events have been passed to the tool.
#. A function that returns a set of events required by the |project| tool.

Now let's make sure the build system knows about our tool.
We need to add our tool as a static library to |project|.

.. code-block:: sh

   $ cd src/Backend/EventCounter
   $ cat > CMakeLists.txt <<EOF
   > set(TOOLNAME EventCounter)
   > set(SOURCES EventCounter.cpp)
   >
   > add_library(${TOOLNAME} STATIC ${SOURCES})
   > set(PRISM_TOOLS_LIBS ${TOOLNAME} PARENT_SCOPE)
   > EOF

And now we recompile |project|:

.. code-block:: sh

   $ cd build
   $ cmake ..
   $ make -j

Running Your Tool
~~~~~~~~~~~~~~~~~

The new tool can be invoked as:


.. code-block:: sh

   $ cd build
   $ bin/prism --backend=EventCounter --executable=ls
