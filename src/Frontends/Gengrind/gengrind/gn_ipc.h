#ifndef GN_IPC_H
#define GN_IPC_H

#include "Frontends/CommonShmemIPC.h"
#include "gn.h"

/* An implementation of interprocess communication with the Sigil2 frontend.
 * IPC includes initialization, termination, shared memory buffer writes, and
 * synchronization via named pipes */

typedef struct EventNameSlotTuple
{
    PrismEvVariant*  event_slot;
    char*          name_slot;
    UInt           name_idx;
} EventNameSlotTuple;

void GN_(initIPC)(void);
void GN_(termIPC)(void);

void GN_(setNextBuffer)(void);
void GN_(flushCurrBuffer)(void);
void GN_(flushCurrAndSetNextBuffer)(void);

// TODO delete
//PrismEvVariant* GN_(acq_event_slot)(void);
/* Get a buffer slot to add an event */
//EventNameSlotTuple GN_(acq_event_name_slot)(UInt size);
/* Get a buffer slot to add an event (probably a context event)
 * and a name slot to add a name with it (like a function name) */

extern PrismEvVariant *GN_(currEv);
extern PrismEvVariant *GN_(endEv);
extern size_t *GN_(usedEv);
/* Points to current location in event buffer,
 * to generate events to,
 * and the end position in the buffer
 * 
 * A null pointer means no buffer has been acquired,
 * currEv == endEv means the buffer is full
 */

#endif
