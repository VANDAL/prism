About
=====

|project| comes from Drexel University's `VLSI & Architecture Lab (VANDAL)
<http://vlsi.ece.drexel.edu>`_, headed by |Baris|_ and in
collaboration with Tufts University's |Mark|_.

.. |Baris| replace:: **Dr. Baris Taskin**
.. _Baris: http://drexel.edu/ece/contact/faculty-directory/TaskinBaris/
.. |Mark| replace:: **Dr. Mark Hempstead**
.. _Mark: http://engineering.tufts.edu/ece/people/hempstead.htm

The goal of |project| is modular application analysis.  It was formed from the
need to support multiple projects that study application traces, aimed at
data-driven architecture design. This has included early hardware accelerator
co-design [SIGIL]_, as well as uncore design space exploration with
multi-threaded workloads [SYNCHROTRACE]_ [UNCORERPD]_.
|project| is not interested in changing the functional behavior of an application,
but instead aims to classify events in the application and present those events
for further analysis. In this way, |project| does not require that each researcher
have an in depth understanding of the binary instrumentation tools.

----

.. [SIGIL] |sigil_link|_
.. |sigil_link| replace:: S.  Nilakantan and M.  Hempstead, "*Platform-independent analysis of
                          function-level communication in workloads*", 2013 IEEE International Symposium
                          on Workload Characterization (IISWC), pp. 196 - 206, 2013.
.. _sigil_link: http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=6704685

.. [SYNCHROTRACE] |synchrotrace_link|_
.. |synchrotrace_link| replace:: S.  Nilakantan, K.  Sangaiah, A.  More, G.  Salvadory, B.
                                 Taskin and M.  Hempstead, "*Synchrotrace: synchronization-aware
                                 architecture-agnostic traces for light-weight multicore simulation*", 2015 IEEE
                                 International Symposium on Performance Analysis of Systems and Software
                                 (ISPASS), pp. 278 - 287, 2015.
.. _synchrotrace_link: http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=7095813

.. [UNCORERPD] |uncorerpd_link|_
.. |uncorerpd_link| replace:: K.  Sangaiah, M.  Hempstead and B.  Taskin, "*Uncore RPD: Rapid
                              design space exploration of the uncore via regression modeling*", 2015 IEEE/ACM
                              International Conference on Computer-Aided Design (ICCAD), pp. 365 - 372, 2015.
.. _uncorerpd_link: http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=7372593
