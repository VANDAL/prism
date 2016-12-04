#ifndef SGL_VALGRIND_IPC_H
#define SGL_VALGRIND_IPC_H

/* Used in shared memory IPC between Valgrind and Sigil2 */

#include "Sigil2/EventBuffer.h"
#define NUM_BUFFERS (8)

/* A timestamp should be appended to these names
 * to avoid conflicts when multiple instances
 * are run concurrently */
#define SIGRIND_SHMEM_NAME ("sgl2-vg-shmem")
#define SIGRIND_EMPTYFIFO_NAME ("sgl2-vg-empty")
#define SIGRIND_FULLFIFO_NAME ("sgl2-vg-full")
#define SIGRIND_FINISHED (0xFFFFFFFFu)

#ifdef __cplusplus
extern "C" {
#else
typedef struct SigrindSharedData SigrindSharedData;
#endif

    /* TODO all shared memory accesses should be atomic to guarantee avoiding
     * race conditions */
    struct SigrindSharedData
    {
        EventBuffer sigrind_buf[NUM_BUFFERS];
    };

#ifdef __cplusplus
}
#else
#endif

#endif
