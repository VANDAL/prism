#ifndef SGL_DYNAMORIO_IPC_H
#define SGL_DYNAMORIO_IPC_H

/* Used in shared memory IPC between DynamoRIO and Sigil2 */

#include "Sigil2/Primitive.h"

#define DRSIGIL_BUFSIZE (1 << 22)
#define DRSIGIL_BUFNUM (4)

#define DRSIGIL_SHMEM_NAME ("sgl2-dr-shmem")
#define DRSIGIL_EMPTYFIFO_NAME ("sgl2-dr-empty")
#define DRSIGIL_FULLFIFO_NAME ("sgl2-dr-full")
#define DRSIGIL_FINISHED (0xFFFFFFFFu)

#ifdef __cpluscplus
extern "C" {
#else
typedef struct DrSigilEvent DrSigilEvent;
typedef struct DrSigilSharedData DrSigilSharedData;
#endif

struct DrSigilEvent
{
	uint16_t thread_id;
	BufferedSglEv ev;
};

struct DrSigilSharedData
{
	DrSigilEvent buf[DRSIGIL_BUFNUM][DRSIGIL_BUFSIZE];
};

#ifdef __cpluscplus
}
#else
#endif

#endif
