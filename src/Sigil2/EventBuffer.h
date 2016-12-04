#ifndef SGL_EVENTBUFFER_H
#define SGL_EVENTBUFFER_H

#include "Primitive.h"
#include <stdlib.h>

#define ARENA_BYTES (1UL << 22)
#define MAX_EVENTS  (1UL << 22)

#ifdef __cplusplus
extern "C" {
#else
typedef struct BufferedSglEv BufferedSglEv;
typedef struct EventBuffer EventBuffer;
#endif

struct BufferedSglEv
{
    union
    {
        SglMemEv  mem;
        SglCompEv comp;
        SglCFEv   cf;
        SglCxtEv  cxt;
        SglSyncEv sync;
    };
    EvTag tag;
} __attribute__ ((__packed__));

struct EventBuffer
{
    size_t events_used;
    size_t arena_used;
    BufferedSglEv events[MAX_EVENTS];
    char arena[ARENA_BYTES];
};

#ifdef __cplusplus
}
#endif


#endif
