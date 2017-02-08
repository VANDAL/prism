#ifndef SGL_DYNAMORIO_IPC_H
#define SGL_DYNAMORIO_IPC_H

/* Used in shared memory IPC between DynamoRIO and Sigil2 */

#include "Sigil2/EventBuffer.h"

#define DRSIGIL_NUM_BUFFERS (4)

#define DRSIGIL_SHMEM_NAME ("sgl2-dr-shmem")
#define DRSIGIL_EMPTYFIFO_NAME ("sgl2-dr-empty")
#define DRSIGIL_FULLFIFO_NAME ("sgl2-dr-full")
#define DRSIGIL_FINISHED (0xFFFFFFFFu)

#ifdef __cplusplus
extern "C" {
#else
typedef struct DrSigilSharedData DrSigilSharedData;
#endif

    struct DrSigilSharedData
    {
        EventBuffer drsigil_buf[DRSIGIL_NUM_BUFFERS];
    };

#ifdef __cplusplus
}
#endif

#endif
