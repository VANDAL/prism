# Event serialization for Prism

The level of detail is meant for internal documentation and ingestion by framework developers--not for the end user.
Of course, sometimes it's useful to understand internals, so feel free to skim this doc.

Prism uses a *workload abstraction*.
It's essentially what is commonly called an *intermediate representation*, or IR.
The only differentiation is that it's specifically designed for analysis, and not to
actually serve as an intermediate step for code.

Because of the research-nature of Prism, the format may change, however this document serves as a best effort documentation of the workload abstraction.

## Event Types

There are 5 event types, that are chosen to represent generic workloads and be suitable to drive
architecturally focused analyses like trace-driven simulation and modeling.

An event stream is made up of a linear ordering of these events, and are delivered to your
analysis tool in an event-driven manner. You just write hooks to fire when one of these events
occurs.

* **Compute**: represent transformation of data
* **Memory**: represent movement of data, but is mostly associated with loads and stores to memory 
* **Sync**: synchronization points. Spawns, joins, barrier. Ways to order parallel event streams.
* **Context**: a grouping of other event types, with optional, variable metadata payload.
  Think: grouping compute and memory events to an instruction, basic block, or function context.
* **Control flow**: potential divergences of an event stream, like calls/returns, indirect jumps, conditional branches.
  This **is not** implemented right now due to developer bandwidth limitations and triaging of work.
  The work supported up to this point has not been interested in speculation, so it's been put on the back-burner.
* **Config**: okay so there's actually 6 events. This is a meta-event to inform the parser of a change
  in level-of-detail, such as including/excluding addresses from memory events.

Implementation note, or: Why do I need this?
A primary goal of prism is to separate the analysis tool from the
underlying profiling methodology. So, for example, you don't have to actually worry about spilling
registers or messing with machine error state, or inserting instructions or functions at all.
*You just write your hooks*, which presumably modifies some model or trace state for your analysis.

Currently, this means your tool is run as a separate process, and reads from IPC buffers for events.
Theoretically, this interface could also be directly built into an instrumentation framework
for speed up (e.g. directly instrumenting your hooks), but that's actually more challenging than
it sounds, and requires very heavy modification of the instrumentation framework.

In order to stay relevant to new and evolving applications, platforms, and architectures, only
the *glue* of generating events to your tool needs to developed in the instrumentation framework,
and then you can run your tool, however complex, with any instrumentation or profiling technique
supported by Prism! Niche? Yes, but that's why I'm making it!

## Event Serialization Schema

Events are variable size depending on the event type and user configuration (e.g. how much detail).

Originally, events were constantly sized as unions, to improve code-gen of advancing through a buffer
of events, but that implementation because problematic and complex when requiring variable-sized
payloads, like function names, and minimizing memory footprint with variable level-of-detail in events.

The trade-off between constant-size and variable-size is of course overhead in parsing.

For all event schemas described below, only the first 'event type' header is required.
The other fields are optionally included afterwards based on the configuration. 

All events are bit-padded to be a byte-multiple (to avoid, e.g., an event that's 11-bits concatenated with another that's 6 bits, etc).
For example, a mem event that only includes the `mem type` (load/store), is 5-bits, but will consume an entire byte.
The next event starts on that next byte in the buffer.


---
### Configuration Schema


| field | bits |
| ----- | ---- |
| event type | 3 |
| event config type | 3 |
| reserved | 2 |
| config bits | variable |

<br/><br/>


----
### Memory

Note: `id` is an experimental special address code used to emulate registers.

| field | bits |
| ----- | ---- |
| event type | 3 |
| mem type | 2 |
| size | 3 |
| address | 56 (full 64b virtual addresses are unnecessary, just sign extend). |
| id | 8 |

<br/><br/>

----
### Compute

| field | bits |
| ----- | ---- |
| event type | 3 |
| op format | 2 |
| op size | 3 |
| op type | 8 |
| op arity | 2 |
| reserved | 6 |
| ids | 8 (each) |

<br/><br/>

----
### Synchronization

| field | bits |
| ----- | ---- |
| event type | 3 |
| sync type | 5 |
| custom metadata | variable |

<br/><br/>


----
### Context

| field | bits |
| ----- | ---- |
| event type | 3 |
| context type | 5 |
| custom metadata | variable |

If Function, then a string

If operator, then a string

If net, then a string

Strings are not C strings. They are max length (not null terminated) of 256.
The length is the first byte. It cannot be length zero, so it's 1-256.

if BB, insn, thread, then a 7-byte ID

Should try to make a name hash table in shared memory at some point, since the strings aren't *that* dynamic.
There's only a limited set of functions, operators, nets that might be called.
Would require a simple static heap of strings in the mapped memory (how to update other side of memory changes?)

Then, on the profiler side, strings are added to the heap, but not removed. Can't be *that* big.
And a hash table of 'operator' to the pointer into the offset into that shared memory heap.

And we just send the string pointer (8 bytes) each time.
We can make this change later on.

<br/><br/>