#include "ReaderMemPool.h"

static inline void readerPoolResize(ReaderPool* pool);

void readerPoolSetReader(ReaderPool* pool, UInt32 reader, UInt32 idx)
{
	(pool->pool + idx)->reader = reader;
}

void readerPoolSetNext(ReaderPool* pool, UInt32 next, UInt32 idx)
{
	(pool->pool + idx)->next_reader_idx = next;
}


UInt32 readerPoolGetReader(const ReaderPool* pool, UInt32 idx)
{
	return (pool->pool + idx)->reader;
}

UInt32 readerPoolGetNext(const ReaderPool* pool, UInt32 idx)
{
	return (pool->pool + idx)->next_reader_idx;
}


Bool readerPoolInit(ReaderPool* pool)
{
	pool->pool = SGL_MALLOC("readerpool", POOL_INIT_SIZE);
	pool->num_free_blocks = POOL_INIT_BLOCKS;
	pool->total_blocks = POOL_INIT_BLOCKS;
	pool->num_initialized = 0;
	pool->head = 0;

	memset( pool->pool, UNDEF, POOL_INIT_BLOCKS*sizeof(*(pool->pool)) );

	return (NULL == pool->pool) ? false : true;
}


UInt32 readerPoolAlloc(ReaderPool* pool)
{
	if (0 == pool->num_free_blocks)
	{
		readerPoolResize(pool);
	}
	
	UInt32 pool_idx = pool->head;

	if (pool->head < pool->num_initialized)
	{
		pool->head = *(UInt32*)(pool->pool + pool_idx);
	}
	else 
	{
		pool->head++;
		pool->num_initialized++;
	}

	pool->num_free_blocks--;
	return pool_idx;
}


void readerPoolFree(ReaderPool* pool, UInt32 idx)
{
	*(UInt32*)(pool->pool + idx) = pool->head;
	pool->head = idx;
	pool->num_free_blocks++;
}


static inline
void readerPoolResize(ReaderPool* pool)
{
	UInt32 prev_num_blocks = pool->total_blocks;
	UInt32 new_num_blocks = 2*prev_num_blocks;
	if ( POOL_MAX_BLOCKS < new_num_blocks )
	{
		exitAtError("readerpoolresize", "Error expanding reader pool");
	}

	pool->pool = SGL_REALLOC("readerpool resize", pool->pool, new_num_blocks*sizeof(Reader));
	if ( NULL == pool->pool )
	{
		exitAtError("readerpoolresize", "Error expanding reader pool");
	}

	memset( pool->pool+prev_num_blocks, UNDEF, prev_num_blocks*sizeof(*(pool->pool)) );

	pool->num_free_blocks = new_num_blocks - prev_num_blocks;
	pool->total_blocks = new_num_blocks;
}
