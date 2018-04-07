====================
Events Documentation
====================

Events List
-----------

Five event primitives:

1. **memory**
#. **compute**
#. **synchronization**
#. **context**
#. **control flow**

Memory
~~~~~~

+-------------+--------------+
| Attribute   | Details      |
+=============+==============+
| Type        || none        |
|             || read        |
|             || write       |
+-------------+--------------+
| Address     |   *numeric*  |
+-------------+--------------+
| Size (Bytes)|   *numeric*  |
+-------------+--------------+

Compute
~~~~~~~

+----------------+----------------------------------+
| Attribute      | Details                          |
+================+==================================+
| Type           || Integer Operation (IOP)         |
|                || Floating Point Operation (FLOP) |
+----------------+----------------------------------+
| Arity          |  *numeric*                       |
+----------------+----------------------------------+
| Size           |  *numeric*                       |
+----------------+----------------------------------+
| Cost Operation || add                             |
|                || sub                             |
|                || mult                            |
|                || div                             |
|                || shift                           |
|                || mov                             |
+----------------+----------------------------------+

Synchronization
~~~~~~~~~~~~~~~

+----------------+------------------------+
| Attribute      | Details                |
+================+========================+
| Type           || none                  |
|                || spawn                 |
|                || join                  |
|                || barrier               |
|                || sync                  |
|                || swap                  |
|                || lock                  |
|                || unlock                |
|                || conditional wait      |
|                || conditional signal    |
|                || conditional broadcast |
|                || spin lock             |
|                || spin unlock           |
+----------------+------------------------+
| data1          |  *numeric*             |
+----------------+------------------------+
| data2          |  *numeric*             |
+----------------+------------------------+

.. todo:: data1/2 is currently a hack for :ref:`SynchroTraceGen`.
          Eventually we want to have the amount of data change depending on *Type*.
          Each datum is not necessarily used, depending on the *Type*.
          Ideally the amount of data tupled in the event will depend on its *Type*,
          but it's faster to iterate over when there's a definitive size.

Context
~~~~~~~

+------------------+------------------------+
| Attribute        | Details                |
+==================+========================+
| Type             || none                  |
|                  || instruction           |
|                  || basic Block           |
|                  || function Enter        |
|                  || function Exit         |
|                  || thread                |
+------------------+------------------------+
|| id              || *numeric*             |
|| name (function) || *string*              |
+------------------+------------------------+

.. todo:: Currently threads are delimited in the event stream with a
          *Sync-Swap* event. This should eventually move to a *Cxt-Thread* event,
          since the event does not strictly order the threads, and is intended to
          just group events that follow it.

Control Flow
~~~~~~~~~~~~

.. note:: Control Flow is currently **not implemented**.
          This table is intended as a guide for future support.

+------------------+----------------------------------+
| Attribute        | Details                          |
+==================+==================================+
| Type             || jump                            |
|                  || call                            |
|                  || return                          |
|                  || suspend                         |
+------------------+-----------+----------------------+
| Conditional      ||  true    | condition            |
|                  ||  false   |                      |
+------------------+-----------+----------------------+
| Destination Type || instruction                     |
|                  || other                           |
+------------------+----------------------------------+
| Destination      |  *numeric*                       |
+------------------+----------------------------------+

Notes
-----
