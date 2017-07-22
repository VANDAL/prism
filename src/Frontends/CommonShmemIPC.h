#ifndef SIGIL2_COMMON_SHMEM_IPC_H
#define SIGIL2_COMMON_SHMEM_IPC_H

#include "Core/EventBuffer.h"

#define SIGIL2_IPC_SHMEM_BASENAME     ("sgl2-shmem")
#define SIGIL2_IPC_EMPTYFIFO_BASENAME ("sgl2-empty")
#define SIGIL2_IPC_FULLFIFO_BASENAME  ("sgl2-full")
#define SIGIL2_IPC_FINISHED (0xFFFFFFFFu)
#define SIGIL2_IPC_BUFFERS (8) /* An empirically based fudge number;
                                * can be tweaked */

#ifdef __cplusplus
static_assert((SIGIL2_IPC_BUFFERS >= 2) &&
              ((SIGIL2_IPC_BUFFERS & (SIGIL2_IPC_BUFFERS - 1)) == 0),
              "SIGIL2_IPC_BUFFERS must be a power of 2");
#else
typedef struct Sigil2DBISharedData Sigil2DBISharedData;
typedef struct Sigil2PerfSharedData Sigil2PerfSharedData;
#endif

struct Sigil2DBISharedData
{
    EventBuffer eventBuffers[SIGIL2_IPC_BUFFERS];
    NameBuffer nameBuffers[SIGIL2_IPC_BUFFERS];
    /* Each EventBuffer has a corresponding NameBuffer
     * as an arena to allocate entity name strings */
};


struct Sigil2PerfSharedData
{
    /* Shared event buffer for a modified linux perf tool
     * to send event data. The event stream is generated from
     * an intel_pt trace. The intel_pt trace decoder is built
     * into perf and leveraged to handle the complex task of
     * combining a raw CPU log with side-channel data (task
     * switchting, interrupts, et al) to produce a coherent
     * instruction trace. This trace is not sent in real-time,
     * thus it's necessary to have timestamp data available in
     * order to understand the stream in the context of parallel
     * threads.
     *
     * XXX MDL20170414
     * Right now only one perf event stream can be
     * sent to Sigil2 at a time. No guarantees are given as to the
     * interleaving of events. It may well be that an entire thread's
     * instruction stream arrives AFTER a parallel thread, even
     * though those two threads were running at the same time.
     * This is inherent to how perf records and decodes intel_pt data.
     * Custom modifications to the way we read in and process the
     * intel_pt data would be required to send multiple event streams
     * in parallel from perf to Sigil2. */

    EventBuffer eventBuffers[SIGIL2_IPC_BUFFERS];

    TimestampBuffer timeBuffers[SIGIL2_IPC_BUFFERS];
    /* Each EventBuffer has a corresponding TimestampBuffer
     * to order events between threads */

    NameBuffer nameBuffers[SIGIL2_IPC_BUFFERS];
    /* Each EventBuffer has a corresponding NameBuffer
     * as an arena to allocate entity name strings */
};

#endif
