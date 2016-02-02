#ifndef SGL_SHMEM_H
#define SGL_SHMEM_H

/* XXX see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60932
 * for relevant concerns */
#ifdef __cplusplus
#include <atomic>
using namespace std;
#else
#include "stdatomic.h"
#endif

#include "Sigil2/Primitive.h"

#define SIGRIND_BUFSIZE (1 << 25)
#define SIGRIND_SHMEM_NAME ("sgl2-vg-ipc")

#ifdef __cpluscplus
extern "C" {
#else
typedef struct SigrindSharedData SigrindSharedData;
#endif

/* Used in shared memory IPC between Valgrind and Sigil2 */
struct SigrindSharedData
{
	volatile atomic_char sigrind_finish;
	volatile atomic_uint head;
	volatile atomic_uint tail;
	BufferedSglEv buf[SIGRIND_BUFSIZE];
};

#ifdef __cpluscplus
}
#else
#endif

#endif
