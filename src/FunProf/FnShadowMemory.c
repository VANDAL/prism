#include "FnShadowMemory.h"
#include "SigilData.h"
#include "ReaderMemPool.h"
#include "Common.h"

#define PM_IDX(x)		((x) >> SM_BITS)
#define SM_IDX(x)		((x) & ((1ULL<<SM_BITS)-1))

#define _STATIC_INLINE_	static inline
#define _STATIC_	static

/* 
 * Reader and writer functions are tracked for each address in
 * shadow memory in their respective shadow objects. 
 *
 * Because multiple functions can read from the same address, 
 * multiple readers are kept for each address, and then cleared upon
 * a write because the new write implies a new dependency, breaking
 * the previous dependency between reads and the previous write.
 *
 * Reader and Writer Shadow Objects can be expanded to track more data
 */

typedef FnCxtUID WriterID;
typedef UInt32 ReaderID;
typedef struct _SecondaryMap
{
	WriterID LastWriter[SM_SIZE];
	ReaderID LastReaders[SM_SIZE];
	ReaderPool pool;
} SecondaryMap;

_STATIC_INLINE_ void setWriterAtAddr(const FnCxtUID writer, const UInt64 ea);
_STATIC_INLINE_ void resetReadersOnWrite(UInt64 ea);
_STATIC_INLINE_ void resetReaderOnWrite(ReaderPool* pool, const UInt32 idx);

_STATIC_INLINE_ void setReaderAtAddr(const FnCxtUID reader, const UInt64 ea);
_STATIC_INLINE_ Bool isAlreadyReader(const ReaderPool* pool, const FnCxtUID reader, const UInt32 idx);
_STATIC_INLINE_ void appendNewReader(ReaderPool* pool, const FnCxtUID reader, const UInt32 idx);

_STATIC_INLINE_ SecondaryMap* getSMFromAddr(UInt64 ea) __attribute__((always_inline));
_STATIC_INLINE_ void initSM(SecondaryMap** SM);
_STATIC_INLINE_ void initShadowMemory();
_STATIC_INLINE_ void checkAddrValid(UInt64 ea);

_STATIC_ SecondaryMap DSM;
_STATIC_ SecondaryMap* PM[PM_SIZE];

_STATIC_ ReaderPool DRP;

_STATIC_ UInt32  num_sms = 0;
_STATIC_ Bool is_init_ShadowMem = false;

void setWriterToAddrRange(const FnCxtUID writer, const UInt64 ea, const UInt64 size)
{
	checkAddrValid(ea+size-1);
	
	for (UInt64 i=0;i<size;++i)
	{
		setWriterAtAddr(writer,ea+i);
	}

	for(UInt64 i=0;i<size;++i)
	{
		/* The readers are kept to track unique accesses. A new write will
		   reset the readers, informing the next reader that the next access 
		   is unique */
		resetReadersOnWrite(ea+i);
	}
}

void setReaderToAddrRange(const FnCxtUID reader, const UInt64 ea, const UInt64 size)
{
	checkAddrValid(ea+size-1);

	for(UInt64 i=0;i<size;++i)
	{
		setReaderAtAddr(reader, ea+i);
	}
}

FnCxtUID getWriterFromAddr(const UInt64 ea)
{
	SecondaryMap* sm = getSMFromAddr(ea);
	return sm->LastWriter[SM_IDX(ea)];
}

Bool isReaderAtAddr(const FnCxtUID reader, const UInt64 ea)
{
	SecondaryMap* sm = getSMFromAddr(ea);
	ReaderPool* pool = &(sm->pool);
	UInt32 idx = sm->LastReaders[SM_IDX(ea)];

	return isAlreadyReader(pool, reader, idx); 
}

_STATIC_INLINE_ 
void setReaderAtAddr(const FnCxtUID reader, const UInt64 ea)
{
	SecondaryMap* sm = getSMFromAddr(ea);
	ReaderPool* pool = &(sm->pool);
	UInt32 idx = sm->LastReaders[SM_IDX(ea)];

	if ( !isAlreadyReader(pool, reader, idx) )
	{
		appendNewReader(pool, reader, idx);
	}
}

_STATIC_INLINE_
Bool isAlreadyReader(const ReaderPool* pool, const FnCxtUID reader, const UInt32 idx)
{
	if ( reader == readerPoolGetReader(pool, idx) )
	{
		return true;
	}

	UInt32 next = readerPoolGetNext(pool, idx);
	if ( UNDEF_FN == next )
	{
		return false;
	}

	return isAlreadyReader(pool, reader, next);
}

_STATIC_INLINE_
void appendNewReader(ReaderPool* pool, const FnCxtUID reader, const UInt32 idx)
{
	FnCxtUID this_reader = readerPoolGetReader(pool, idx);
	if ( UNDEF_FN == this_reader )
	{
		readerPoolSetReader(pool, reader, idx);
	}
	else
	{
		UInt32 next = readerPoolGetNext(pool, idx);
		if ( UNDEF_FN == next )
		{
			next = readerPoolAlloc(pool);
			readerPoolSetReader(pool, reader, next);
			readerPoolSetNext(pool, next, idx);
		}
		else
		{
			appendNewReader(pool, reader, next);
		}
	}
}

_STATIC_INLINE_ 
void setWriterAtAddr(const FnCxtUID writer, const UInt64 ea)
{
	SecondaryMap* sm = getSMFromAddr(ea);
	sm->LastWriter[SM_IDX(ea)] = writer;
}

_STATIC_INLINE_
void resetReadersOnWrite(const UInt64 ea)
{
	SecondaryMap* sm = getSMFromAddr(ea);
	ReaderPool* pool = &(sm->pool);

	UInt32 idx = sm->LastReaders[SM_IDX(ea)];
	resetReaderOnWrite(pool, idx);
}

_STATIC_INLINE_
void resetReaderOnWrite(ReaderPool* pool, const UInt32 idx)
{
	if ( UNDEF_FN != readerPoolGetReader(pool, idx) )
	{
		readerPoolSetReader(pool, UNDEF_FN, idx);
		readerPoolFree(pool, idx);

		UInt32 next = readerPoolGetNext(pool, idx);
		if ( UNDEF_FN != next )
		{
			readerPoolSetNext(pool, UNDEF_FN, idx);
			resetReaderOnWrite(pool, next);
		}
	}
}

_STATIC_INLINE_
SecondaryMap* getSMFromAddr(const UInt64  ea)
{
	checkAddrValid(ea);

	if (!is_init_ShadowMem)
	{
		initShadowMemory();
	}

	SecondaryMap** SM = &PM[PM_IDX(ea)];
	if (*SM == &DSM)
	{
		initSM(SM);
	}

	return *SM;
}

_STATIC_INLINE_
void initSM(SecondaryMap** SM)
{
	*SM = SGL_MALLOC("SGL.initSM",sizeof(**SM));
	SGL_MEMCPY((void*)*SM, (void*)&DSM, sizeof(**SM));

	ReaderPool* rdpool = &((*SM)->pool);
	readerPoolInit(rdpool);

	/* It is implied that the initial blocks 
	   have already been allocated; 
	   faster this way but more dangerous */
	rdpool->num_free_blocks = DRP.num_free_blocks;
	rdpool->total_blocks = DRP.total_blocks;
	rdpool->num_initialized = DRP.num_initialized;
	rdpool->head = DRP.head;

	num_sms++;
}

_STATIC_INLINE_
void initShadowMemory()
{
	if (!is_init_ShadowMem)
	{
		/* Writer/Reader init */
		for( UInt64 i=0; i<SM_SIZE; ++i )
		{
			DSM.LastWriter[i] = UNDEF_FN;
			DSM.LastReaders[i] = UNDEF_FN;
		}

		/* Multiple reader init */
		readerPoolInit(&DRP);
		for( UInt64 i=0; i<SM_SIZE; ++i )
		{
			DSM.LastReaders[i] = readerPoolAlloc(&DRP);
		}

		DSM.pool = DRP;

		/* Primary Map init */
		for( UInt64 i=0; i<PM_SIZE; i++ )
		{
			PM[i]=&DSM;
		}

		is_init_ShadowMem = true;
	}
}

_STATIC_INLINE_
void checkAddrValid(UInt64 ea)
{
	if ( ea > MAX_PRIMARY_ADDRESS )
	{
		exitAtError("ShadowMemory.checkAddrValid",
					"Tried to use shadow memory for INVALID ADDRESS");
	}
}
