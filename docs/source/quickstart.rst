Quickstart
==========

This document will go through building and running |project|.

Building Sigil2
---------------

Clone and build |project| from source::

  $ git clone --recursive https://github.com/mdlui/Sigil2
  $ cd Sigil2
  $ mkdir build && cd build
  $ cmake -DCMAKE_BUILD_TYPE=release ..
  $ make -j<JOBS>

This creates a ``build/bin`` folder containing the :program:`sigil2` executable.
It can be run in place, or the entire ``bin`` folder can be moved,
although it's not advised to move it to a system location.

Running Sigil2
--------------

|project| requires at least two arguments: the ``backend`` analysis tool,
and the ``executable`` application to measure::

  $ bin/sigil2 --backend=stgen --executable=./mybinary

The ``backend`` is the analysis tool that will process all the events
in ``mybinary``. In this example, ``stgen`` is the backend that processes
events into a special event trace that is used in SynchroTrace_.

See :doc:`here for more information on backends <backends>`.

.. _SynchroTrace:
   http://ece.drexel.edu/faculty/taskin/wiki/vlsilab/index.php/SynchroTrace

A third option ``frontend`` will change the underlying method
for observing the application. By default, this is Valgrind_: ::

  $ bin/sigil2 --frontend=valgrind --backend=stgen --executable=./mybinary

.. _Valgrind: http://valgrind.org/

Dependencies
------------

+-------------+---------+
| PACKAGE     | VERSION |
+=============+=========+
| gcc/g++     |  4.8    |
+-------------+---------+
| cmake       |  2.8.11 |
+-------------+---------+
| make        |  3.8    |
+-------------+---------+
| automake    |  1.13   |
+-------------+---------+
| autoconf    |  2.69   |
+-------------+---------+
| zlib        |  1.27   |
+-------------+---------+
| git         |  1.8    |
+-------------+---------+
