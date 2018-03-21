#ifndef PRISM_COMMON_SHMEM_IPC_H
#define PRISM_COMMON_SHMEM_IPC_H

#include "Core/EventBuffer.h"

#define PRISM_IPC_SHMEM_BASENAME     ("prism-shmem")
#define PRISM_IPC_EMPTYFIFO_BASENAME ("prism-empty")
#define PRISM_IPC_FULLFIFO_BASENAME  ("prism-full")
#define PRISM_IPC_FINISHED (0xFFFFFFFFu)
#define PRISM_IPC_BUFFERS (8) /* An empirically based fudge number;
                               * can be tweaked */

#ifdef __cplusplus
static_assert((PRISM_IPC_BUFFERS >= 2) &&
              ((PRISM_IPC_BUFFERS & (PRISM_IPC_BUFFERS - 1)) == 0),
              "PRISM_IPC_BUFFERS must be a power of 2");
#else
typedef struct PrismDBISharedData PrismDBISharedData;
typedef struct PrismPerfSharedData PrismPerfSharedData;
#endif

struct PrismDBISharedData
{
    EventBuffer eventBuffers[PRISM_IPC_BUFFERS];
    NameBuffer nameBuffers[PRISM_IPC_BUFFERS];
    /* Each EventBuffer has a corresponding NameBuffer
     * as an arena to allocate entity name strings */
};


struct PrismPerfSharedData
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
     * sent to Prism at a time. No guarantees are given as to the
     * interleaving of events. It may well be that an entire thread's
     * instruction stream arrives AFTER a parallel thread, even
     * though those two threads were running at the same time.
     * This is inherent to how perf records and decodes intel_pt data.
     * Custom modifications to the way we read in and process the
     * intel_pt data would be required to send multiple event streams
     * in parallel from perf to Prism. */

    EventBuffer eventBuffers[PRISM_IPC_BUFFERS];

    TimestampBuffer timeBuffers[PRISM_IPC_BUFFERS];
    /* Each EventBuffer has a corresponding TimestampBuffer
     * to order events between threads */

    NameBuffer nameBuffers[PRISM_IPC_BUFFERS];
    /* Each EventBuffer has a corresponding NameBuffer
     * as an arena to allocate entity name strings */
};

#endif
