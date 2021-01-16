#ifndef SGL_IPC_H
#define SGL_IPC_H

#include "Frontends/CommonShmemIPC.h"
#include "global.h"

/* An implementation of interprocess communication with the Sigil2 frontend.
 * IPC includes initialization, termination, shared memory buffer writes, and
 * synchronization via named pipes */

void SGL_(init_IPC)(void);
void SGL_(term_IPC)(void);
unsigned char* SGL_(reserve_ev_buf)(uint32_t bytes_requested);

#endif
