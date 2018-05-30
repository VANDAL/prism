Quickstart
==========

This page will quickly walk you through building and running |project|.

Building Prism
--------------

.. note:: The default compiler for **CentOS 7** and older (gcc <5)
          does not support **C++14**. Install and enable the offical
          Devtoolset_ before compiling.

.. _Devtoolset:
   https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/

Clone and build |project| from source::

  $ git clone https://github.com/vandal/prism
  $ cd prism
  $ mkdir build && cd build
  $ cmake3 .. # CentOS 7 requires cmake3 package
  $ make -j

This creates a ``build/bin`` folder containing the :program:`prism` executable.
It can be run in place, or the entire ``bin`` folder can be moved,
although it's not advised to move it to a system location.

Running Prism
-------------

|project| requires at least two arguments: the ``backend`` analysis tool,
and the ``executable`` application to measure::

  $ bin/prism --backend=stgen --executable=./mybinary

The ``backend`` is the analysis tool that will analyze the requested events
in ``mybinary``. In this example, ``stgen`` is the backend that processes
events into a special event trace that is used in SynchroTrace_.

.. _SynchroTrace:
   http://vlsi.ece.drexel.edu/index.php/SynchroTrace/

A third option ``frontend`` will change the underlying method
for observing the application. By default, this is Valgrind_: ::

  $ bin/prism --frontend=valgrind --backend=stgen --executable=./mybinary

.. _Valgrind: http://valgrind.org/

Dependencies
------------

+---------------+----------+
| PACKAGE       | VERSION  |
+===============+==========+
| gcc/g++       |  5+      |
+---------------+----------+
| cmake         |  3.1.3+  |
+---------------+----------+
| make          |  3.8+    |
+---------------+----------+
| automake      |  1.13+   |
+---------------+----------+
| autoconf      |  2.69+   |
+---------------+----------+
| zlib/zlib-dev |  1.27+   |
+---------------+----------+
| git           |  1.8+    |
+---------------+----------+
