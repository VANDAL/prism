#ifndef SGL_FRONTEND_DBI_IPC_H
#define SGL_FRONTEND_DBI_IPC_H

#include "Sigil2/EventBuffer.h"

/* IPC between DBI frontends and Sigil2 */

/* A unique id should be appended to these names
 * to avoid conflicts when multiple instances
 * are run concurrently */
#define SIGIL2_DBI_SHMEM_NAME     ("sgl2-shmem")
#define SIGIL2_DBI_EMPTYFIFO_NAME ("sgl2-empty")
#define SIGIL2_DBI_FULLFIFO_NAME  ("sgl2-full")
#define SIGIL2_DBI_FINISHED       (0xFFFFFFFFu)
#define SIGIL2_DBI_BUFFERS        (8)

#ifdef __cplusplus
extern "C" {
#else
typedef struct Sigil2DBISharedData Sigil2DBISharedData;
#endif

struct Sigil2DBISharedData
{
    EventBuffer buf[SIGIL2_DBI_BUFFERS];
};

#ifdef __cplusplus
}
#endif

#endif
