#ifndef SGL_IPC_H
#define SGL_IPC_H

#include "Frontends/CommonShmemIPC.h"
#include "global.h"

/* An implementation of interprocess communication with the Sigil2 frontend.
 * IPC includes initialization, termination, shared memory buffer writes, and
 * synchronization via named pipes */

typedef struct EventNameSlotTuple
{
    SglEvVariant*  event_slot;
    char*          name_slot;
    UInt           name_idx;
} EventNameSlotTuple;

void SGL_(init_IPC)(void);
void SGL_(term_IPC)(void);

SglEvVariant* SGL_(acq_event_slot)(void);
/* Get a buffer slot to add an event */

EventNameSlotTuple SGL_(acq_event_name_slot)(UInt size);
/* Get a buffer slot to add an event (probably a context event)
 * and a name slot to add a name with it (like a function name) */

#endif
