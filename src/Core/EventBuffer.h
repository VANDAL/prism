#ifndef PRISM_EVENTBUFFER_H
#define PRISM_EVENTBUFFER_H

#include "Primitive.h"
#include <stdlib.h>

#define PRISM_NAMES_BUFFER_SIZE (1UL << 19)
#define PRISM_EVENTS_BUFFER_SIZE (1UL << 19)

#ifdef __cplusplus
#include <memory>
extern "C" {
#else
typedef struct EventBuffer EventBuffer;
#endif


struct EventBuffer
{
    /** Prism core event primitives */
    unsigned char events[PRISM_EVENTS_BUFFER_SIZE];
};


#ifdef __cplusplus
using EventBufferPtr = std::unique_ptr<EventBuffer>;
}
#endif


#endif
