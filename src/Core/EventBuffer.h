#ifndef PRISM_EVENTBUFFER_H
#define PRISM_EVENTBUFFER_H

#include "Primitive.h"
#include <stdlib.h>

#define PRISM_NAMES_BUFFER_SIZE (1UL << 12)
#define PRISM_EVENTS_BUFFER_SIZE (1UL << 12)

#ifdef __cplusplus
#include <memory>
extern "C" {
#else
typedef struct PrismEvVariant PrismEvVariant;
typedef struct EventBuffer EventBuffer;
typedef struct NameBuffer NameBuffer;
typedef struct TimestampBuffer TimestampBuffer;
#endif

struct PrismEvVariant
{
    /* Prism event variant */

    EvTag tag;
    union
    {
        PrismMemEv  mem;
        PrismCompEv comp;
        PrismCFEv   cf;
        PrismCxtEv  cxt;
        PrismSyncEv sync;
    };
} __attribute__ ((__packed__));

struct EventBuffer
{
    /* Prism core event primitives */

    size_t used;
    PrismEvVariant events[PRISM_EVENTS_BUFFER_SIZE];
};

struct TimestampBuffer
{
    /* Timestamps are an optional feature to order events.
     * This is mainly useful when ordering events between different threads,
     * especially when each thread's event stream arrives at Prism in
     * large gaps in time (i.e. not real time).
     *
     * This was added to support perf event streams, which are captured from
     * hardware but parsed by Prism at a later time.
     * Perf expects the order of the events to be inferred from timestamps.
     *
     * Corresponding TimestampBuffer.used and EventBuffer.used values
     * should always match */

    size_t used;
    uint64_t timestamps[PRISM_EVENTS_BUFFER_SIZE];
};

struct NameBuffer
{
    /* Optional context names for a Prism EventBuffer.
     * Any context events with names (e.g. function names)
     * will have a cstring that exists in this
     * corresponding memory arena. */

    size_t used;
    char names[PRISM_NAMES_BUFFER_SIZE];
};


#ifdef __cplusplus
using EventBufferPtr = std::unique_ptr<EventBuffer>;
}
#endif


#endif
