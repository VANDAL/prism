#ifndef SIGIL2_EVENTBUFFER_H
#define SIGIL2_EVENTBUFFER_H

#include "Primitive.h"
#include <stdlib.h>

#define SIGIL2_NAMES_BUFFER_SIZE (1UL << 20)
#define SIGIL2_EVENTS_BUFFER_SIZE (1UL << 20)

#ifdef __cplusplus
#include <memory>
extern "C" {
#else
typedef struct SglEvVariant SglEvVariant;
typedef struct EventBuffer EventBuffer;
typedef struct NameBuffer NameBuffer;
typedef struct TimestampBuffer TimestampBuffer;
#endif

struct SglEvVariant
{
    /* Sigil2 event variant */

    EvTag tag;
    union
    {
        SglMemEv  mem;
        SglCompEv comp;
        SglCFEv   cf;
        SglCxtEv  cxt;
        SglSyncEv sync;
    };
} __attribute__ ((__packed__));

struct EventBuffer
{
    /* Sigil2 core event primitives */

    size_t used;
    SglEvVariant events[SIGIL2_EVENTS_BUFFER_SIZE];
};

struct TimestampBuffer
{
    /* Timestamps are an optional feature to order events.
     * This is mainly useful when ordering events between different threads,
     * especially when each thread's event stream arrives at Sigil2 in
     * large gaps in time (i.e. not real time).
     *
     * This was added to support perf event streams, which are captured from
     * hardware but parsed by Sigil2 at a later time.
     * Perf expects the order of the events to be inferred from timestamps.
     *
     * Corresponding TimestampBuffer.used and EventBuffer.used values
     * should always match */

    size_t used;
    uint64_t timestamps[SIGIL2_EVENTS_BUFFER_SIZE];
};

struct NameBuffer
{
    /* Optional context names for a Sigil2 EventBuffer.
     * Any context events with names (e.g. function names)
     * will have a cstring that exists in this
     * corresponding memory arena. */

    size_t used;
    char names[SIGIL2_NAMES_BUFFER_SIZE];
};


#ifdef __cplusplus
using EventBufferPtr = std::unique_ptr<EventBuffer>;
}
#endif


#endif
