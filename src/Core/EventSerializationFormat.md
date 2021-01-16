# Event serialization for Prism

The level of detail is meant for internal documentation and ingestion by framework developers--not for the end user.
Of course, sometimes it's useful to understand internals, so feel free to skim this doc.

Prism uses a *workload abstraction*.
It's essentially what is commonly called an *intermediate representation*, or IR.
The only differentiation is that it's specifically designed to analyze workloads, and not to
actually serve as an intermediate step for code generation.
The abstraction level is designed to be configurable, from load/store addresses up to domain-specific blocks like deep learning operators.

Because the events are communicated over a shared memory right now, the format aims to minimize the overhead of writing/reading the shared memory by reducing bandwidth via event-size.

## Aside: Why?

More specifically, the *workload abstraction* is designed for analysis of workloads *on a system*, but more useful than a pure instruction stream.
It's abstracted from the architecture (i.e. not a pure instruction stream), but may include architectural side effects, like register spilling.
I want to separate the challenges of (1) analyzing the workload for interesting modeling and system exploration, and (2) trying to capture this workload at varying abstraction levels.

So, for example, you don't have to actually worry about spilling
registers or messing with machine error state, or inserting instructions or functions at all.
*You just write your hooks* to act on each 'event'. The event hooks presumably modify some model or trace state for your analysis.

This loose-coupling naturally loses some performance, compared to workflows that tightly-integrate the two, but my hypothesis is that the benefits of (1) easily switching "event capture" methods and (2) abstraction levels,  (3) more flexible software design of "event analysis", and (4) separation of responsibilities will outweigh the initial overheads—the
initial overheads being another process running (consuming extra resources like TLB, icache, and dedicated CPU via context switches) and extra memory bandwidth from interprocess communication (and the associated cache footprint).

Most prior work is concerned with pure latency and performance of tools and thus focuses on tightly integrated instrumentation techniques.
However, for *very* heavy-weight analysis, beyond memcheck, I also hypothesize this tight-coupling is unnecessary and hurtful to system designers and researchers who have expertise in the system stack and architecture, but do not have expertise in the multitude of complex instrumentation frameworks, profiling tools, and application frameworks required to study modern applications.
Those users (mostly in research and system exploration) are the targeted users. Not software developers who would benefit from faster feedback and require a limited toolbox like ASAN, perf record/stat, flamegraphs, strace, and the like.

In order to stay relevant to new and evolving applications, platforms, and architectures, only
the *glue* of generating events to your tool needs to developed in the instrumentation framework,
and then you can run your tool, however complex, with any instrumentation or profiling technique
supported by Prism! Niche? Yes, but that's why I'm making it!

Lastly, for posterity, note that tight integration is indeed possible, with additional engineering.
Such a solution requires a far more complex system that takes the user code, and, for example, sends it to an instrumentation framework, which then JITs it into itself.
It would require that the user-tool be statically compiled (to avoid link issues) and large changes to the instrumentation framework to add a JIT, and/or a custom language a-la eBPF.
An interesting aside, but not a direction I have the bandwidth to explore.

Well this section turned into more of a of soap-box than intended. Apologies and thanks for reading.

## Event Types

**Because of the research-nature of Prism, the format may change, however this document serves as a best effort documentation of the workload abstraction.**

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
* **Config**: okay so there's actually 6 events. This is a meta-event to inform the event parser of a change
  in level-of-detail, such as including/excluding addresses from memory events.


## Event Serialization Schema

Events are variable size depending on the event type and user configuration (e.g. how much detail).

Originally, events were constantly sized as unions, to improve code-gen of advancing through a buffer
of events, but that implementation because problematic and complex when requiring variable-sized
payloads, like function names, and minimizing memory footprint with variable level-of-detail in events.

The trade-off between constant-size and variable-size is of course overhead and boiler-plate in reading/writing events.

For all event schemas described below, only the first `event type` header is required.
The other fields are optionally included afterwards based on the configuration. 

All events are bit-padded to be at least byte-aligned (to avoid, e.g., an event that's 11-bits concatenated with another that's 6 bits, etc).
The next event starts on that next byte in the buffer.


---
### Configuration Event


| byte| field | bits | notes |
| ----- | ----- | ---- | ---- |
| 0 | reserved | 2 | Each byte starts with LSB field in these tables
| 0 | event config type (mem, comp, et al) | 3 |
| 0 | event type | 3 | And end with MSB field for a given byte
| 1 | config bits | variable (max 8) | Represents every field (for the event type) **other than** `event type`, from MSB to LSB

<br/><br/>

----
### Memory Event
> **Note**: Byte `+2(a)` corresponds to the '*next*' 2 bytes, and all `(a)` fields are part of the same byte

> **Note**: `id` is an experimental special address code used to emulate registers.

| byte| field | bits | notes |
| ----- | ----- | ---- | ---- |
| 0 | size (2^n bytes) | 3 |
| 0 | mem type | 2 |
| 0 | event type | 3 |
| +7(a) | address | 56  |Lower 56 bits of virtual address. MSB is in +7th byte (8th overall).
| +1(b) | id | 8 |

<br/><br/>

----
### Compute Event

| byte| field | bits | notes |
| ----- | ----- | ---- | ---- |
| 0 | op size | 3 |
| 0 | op format | 2 |
| 0 | event type | 3 |
| +1(b)  | op arity | 2 | arity corresponds to 1-4 arguments
| +1(b)  | op type | 6 |
| +1(c) | ids | 8  | w/ IDs enabled, each arg has an assoicated id
| +1(d) | ids | 8  |
| +1(e) | ids | 8  |
| +1(f) | ids | 8  |

<br/><br/>

----
### Synchronization

| byte| field | bits |
| ----- | ----- | ---- |
| 0 | sync type | 5 |
| 0 | event type | 3 |
| +8(a) | custom metadata | depends on `sync type`, e.g. thread ID |
| +8(b) | custom metadata | depends on `sync type`, e.g. an extra condition variable |

<br/><br/>


----
### Context

| byte| field | bits | notes |
| ----- | ----- | ---- | ---- |
| 0 | context type | 5 | e.g. instruction, basic block, function, "operator"
| 0 | event type | 3 |
| +N | custom metadata | variable | For IDs/addrs: a 7-byte value. For names: usually a `length` byte (1-256), followed by a string (max 256 bytes)

If performance becomes a concern (e.g. if sending repeated strings for repeated functions causes performance issues), I might opt for a string interning solution, with some large heap in shared memory.
This is the ideal solution for larger strings, but adds complexity I don't have bandwidth for.
Would need to profile how many unique functions exist times average size.
May be able to get away with just a reasonably sized, static heap size.
