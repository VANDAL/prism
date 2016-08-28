User Documentation
==================

Getting Started with Profiling
------------------------------

This example will demonstrate how to get started analyzing a workload.
Typically it's easier to analyze a trace file than to directly analyze a
workload. That is, it's easier to generate a trace and post-process it multiple
times, instead of analyzing the application on-the-fly. Parsing a trace file
containing relevant data is going to be faster and more straightforward than
running a workload multiple times and having |project| filter all the potential
metadata repeatedly. However, there is nothing preventing you from doing all of
it has already run.

With that said, let's do an example

.. todo:: example

The Profiling Frontend
----------------------

A *frontend* is the component that is generating the event stream.
By default, this is Valgrind (mostly due to historical reasons).

While it's tempting to assume that the event generation *just works*
you should be aware of the intrinsic nature of the chosen frontend
before making any large assumptions. For example, Valgrind is a
*copy & annotate* dynamic binary instrumentation tool. This means that
instructions are grouped into blocks, disassembled into Valgrind's IR,
instrumented, and then recompiled just-in-time.

event streams are
created from a combination of Valgrind IR and the instructions Valgrind
sees

Down the Rabbit Hole: Multithreaded Streams
-------------------------------------------

|project| allows for multithreaded analysis of event streams
if the frontend supports it.

.. note:: Currently only DynamoRIO (*experimental*) supports multithreaded
          analysis.


FAQ
---
