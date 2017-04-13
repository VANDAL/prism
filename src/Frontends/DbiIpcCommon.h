#ifndef SGL_FRONTEND_DBI_IPC_H
#define SGL_FRONTEND_DBI_IPC_H

#include "Core/EventBuffer.h"

#define SIGIL2_DBI_SHMEM_BASENAME     ("sgl2-shmem")
#define SIGIL2_DBI_EMPTYFIFO_BASENAME ("sgl2-empty")
#define SIGIL2_DBI_FULLFIFO_BASENAME  ("sgl2-full")
#define SIGIL2_DBI_FINISHED (0xFFFFFFFFu)
#define SIGIL2_DBI_BUFFERS (8)

#ifdef __cplusplus
static_assert((SIGIL2_DBI_BUFFERS >= 2) &&
              ((SIGIL2_DBI_BUFFERS & (SIGIL2_DBI_BUFFERS - 1)) == 0),
              "SIGIL2_DBI_BUFFERS must be a power of 2");
#else
typedef struct Sigil2DBISharedData Sigil2DBISharedData;
#endif

struct Sigil2DBISharedData
{
    EventBuffer eventBuffers[SIGIL2_DBI_BUFFERS];
    NameBuffer nameBuffers[SIGIL2_DBI_BUFFERS];
    /* Each EventBuffer has a corresponding NameBuffer
     * as an arena to allocate entity name strings */
};

#endif
