#ifndef SGL_VALGRIND_IPC_H
#define SGL_VALGRIND_IPC_H

/* Used in shared memory IPC between Valgrind and Sigil2 */

#include "Sigil2/Primitive.h"

#define SIGRIND_BUFSIZE (1 << 24)
#define SIGRIND_BUFNUM (4)

#define SIGRIND_SHMEM_NAME ("sgl2-vg-shmem")
#define SIGRIND_EMPTYFIFO_NAME ("sgl2-vg-empty")
#define SIGRIND_FULLFIFO_NAME ("sgl2-vg-full")
#define SIGRIND_FINISHED (0xFFFFFFFFu)

#ifdef __cpluscplus
extern "C" {
#else
typedef struct SigrindSharedData SigrindSharedData;
#endif

struct SigrindSharedData
{
	BufferedSglEv buf[SIGRIND_BUFNUM][SIGRIND_BUFSIZE];
};

#ifdef __cpluscplus
}
#else
#endif

#endif
