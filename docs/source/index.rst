.. Prism documentation master file, created by
   sphinx-quickstart on Thu Aug 11 18:44:28 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to |project|'s Documentation!
=====================================

|project| is a technology for building platform-agnostic workload analysis tools.
Tools are built once and are able to run across multiple architectures and environments.
|project| targets complex analyses that are latency-tolerant, in contrast to real-time analyses.

|project| aims to improve three main components of designing new analysis tools for research: 1) *modularity*, 2) *design flexibility*,
and 3) *productivity*.

.. toctree::
   :maxdepth: 2

   overview
   getting-started
   events
   backends
   frontends
   about
   changelog

Features
--------
* Flexible application analysis

  * Use multiple frontends for capturing software workloads
    like Valgrind and DynamoRIO
  * Use custom C++14 libraries for analyzing event streams

* Platform-independent events

  * Straight-forward and extensible format,
    simplifying analysis

Installation
------------

See the :doc:`Quickstart <quickstart>` for information installation instructions.

Contribute
----------

Source Code: https://github.com/vandal/prism

Issue Tracker: https://github.com/vandal/prism/issues

License
-------

This project is licensed under the `BSD3 license`_.

.. _`BSD3 license`: https://github.com/vandal/prism/blob/master/COPYING
