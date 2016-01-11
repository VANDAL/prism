#ifndef _READER_MEM_POOL_H_
#define _READER_MEM_POOL_H_

#include "Common.h"
#include "ShadowMemory.h"

/*
 * Memory pool implementation for tracking multiple readers of the
 * same address.
 *
 * Credit goes to:
 * 'Fast Efficient Fixed-Size Memory Pool'
 * Ben Kenwright, Newcastle University
 * Newcastle, UK
 *
 * Because of the current algorithm used for tracking function dependencies,
 * and due to the unknown runtime limit on how many functions dependencies 
 * will exist for a given address, a situation may occur where billions of
 * mallocs can occur (one for each new reader to an address). This has been
 * profiled to show significant slowdown and fragmentation memory overhead.
 * Instead, a fixed memory pool will be used to track dependencies. 
 */

#define POOL_MAX_BLOCKS (UINT32_MAX-1)
#define POOL_INIT_BLOCKS (SM_SIZE)
#define POOL_INIT_SIZE (POOL_INIT_BLOCKS*sizeof(Reader))

/* 'next reader' indices are defined only 
   from 0 to (2^64-2)   */
#define UNDEF_READER_IDX UINT64_MAX

/* fncxt ids are defined only 
   from 0 to (2^32-2)   */
#define UNDEF_FN UINT32_MAX

#define UNDEF UINT32_MAX

typedef struct _Reader
{
	UInt64 next_reader_idx;
	UInt32 reader;
} Reader;

typedef struct _ReaderPool
{
	UInt32 head;
	UInt32 num_free_blocks;
	UInt32 num_initialized;
	Reader* pool;

	UInt32 total_blocks;
} ReaderPool;

/* Set and get reader as a function-index via a memory pool index */
void	readerPoolSetReader(ReaderPool* pool, UInt32 reader, UInt32 idx);
void	readerPoolSetNext(ReaderPool* pool, UInt32 next, UInt32 idx);
UInt32	readerPoolGetReader(const ReaderPool* pool, UInt32 idx);
UInt32	readerPoolGetNext(const ReaderPool* pool, UInt32 idx);

/* Managing with the memory pool */
Bool	readerPoolInit(ReaderPool* pool);
UInt32	readerPoolAlloc(ReaderPool* pool);
void	readerPoolFree(ReaderPool* pool, UInt32 idx);

#endif
