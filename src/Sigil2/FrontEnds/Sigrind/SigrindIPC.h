#ifndef SGL_IPC_H
#define SGL_IPC_H

/* Used in shared memory IPC between Valgrind and Sigil2 */

#include "Sigil2/Primitive.h"

/* XXX From write(2) man pages:
 *
 * On Linux, write() (and similar system calls) will transfer at most
 * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
 * actually transferred.  (This is true on both 32-bit and 64-bit
 * systems.) */
#define SIGRIND_BUFSIZE (1 << 24)
#define SIGRIND_BUFNUM (3)

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
