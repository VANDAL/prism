#ifndef SGL_SHMEM_H
#define SGL_SHMEM_H

#include "Sigil2/Primitive.h"

#define SIGRIND_BUFSIZE (1 << 20)
#define SIGRIND_SHMEM_NAME ("sgl2-vg-ipc")

#ifdef __cpluscplus
extern "C" {
#else
typedef struct SigrindSharedData SigrindSharedData;
#endif

/* Used in shared memory IPC between Valgrind and Sigil2 */
struct SigrindSharedData
{
	volatile char sigrind_finish;
	volatile unsigned int head;
	volatile unsigned int tail;
	volatile BufferedSglEv buf[SIGRIND_BUFSIZE];

};

#ifdef __cpluscplus
}
#else
#endif

#endif
