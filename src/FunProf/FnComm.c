#include "FnMgmt.h"
#include "FnComm.h"
#include "FnShadowMemory.h"

#define _STATIC_ static
#define _STATIC_INLINE_ static inline

_STATIC_INLINE_ void addUniqueLocalBytes(UInt64 num_bytes) __attribute__((always_inline));
_STATIC_INLINE_ void addNonUniqueLocalBytes(UInt64 num_bytes) __attribute__((always_inline));
_STATIC_INLINE_ void addUniqueDepBytes(const FnCxtUID producer, UInt64 num_bytes) __attribute__((always_inline));
_STATIC_INLINE_ void addNonUniqueDepBytes(const FnCxtUID producer, UInt64 num_bytes) __attribute__((always_inline));
_STATIC_INLINE_ DepNode* getDepNode(const FnCxtUID consumer, const FnCxtUID producer);

/*********************************************************************/
/*                    Data Write Instrumentation                     */
/*********************************************************************/
//nothing to do for data writes, no dependencies
//TODO call this file/functions FnDeps? 


/*********************************************************************/
/*                    Data Read Instrumentation                      */
/*********************************************************************/
/* This can be improved by tracking alignment. 
 * However, tracking alignment has time overhead,
 * and potentially huge memory overhead. Needs to be
 * profiled before implementation */
void setReaderDependencies(const FnCxtUID curr_reader, UInt64 addr,	UInt64 bytes)
{
	for (UInt64 i=addr; i<(addr+bytes); ++i)
	{
		FnCxtUID last_writer = getWriterFromAddr(i);

		if/*local read*/(curr_reader == last_writer)
		{
			if ( isReaderAtAddr(curr_reader, i) )
			{
				addUniqueLocalBytes(1);
			} else
			{
				addNonUniqueLocalBytes(1);
			}
		} else 
		{
			if ( isReaderAtAddr(curr_reader, i) )
			{
				addUniqueDepBytes(last_writer, 1);
			} else
			{
				addNonUniqueDepBytes(last_writer, 1);
			}
		}
	}
}

_STATIC_INLINE_ 
void addUniqueLocalBytes(UInt64 num_bytes) {
	getCurrFnCxt()->unique_local_cnt += num_bytes;
}

_STATIC_INLINE_ 
void addNonUniqueLocalBytes(UInt64 num_bytes) {
	getCurrFnCxt()->nonunique_local_cnt += num_bytes;
}

_STATIC_INLINE_ 
void addUniqueDepBytes(const FnCxtUID producer, UInt64 num_bytes) {
	getDepNode(consumer,producer)->unique_bytes_read+=num_bytes;
}

_STATIC_INLINE_ 
void addNonUniqueDepBytes(const FnCxtUID producer, UInt64 num_bytes) {
	getDepNode(consumer,producer)->reused_bytes_read+=num_bytes;
}

_STATIC_INLINE_ 
DepNode* getDepNode(const FnCxtNode* consumer, const FnCxtNode* producer)
{
}
