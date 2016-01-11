#include "ShadowMemory.h"
#include "FnShadMem.h"
#include "FnAlignment.h"

#define _STATIC_ static
#define _STATIC_INLINE_ static inline

typedef struct _ReaderObject ReaderObject;
typedef struct _WriterObject WriterObject;
typedef struct _ReaderNode ReaderNode;
typedef struct _ShadowObject ShadowObject;
typedef struct _ReaderAlign ReaderAlign;
typedef struct _WriterAlign WriterAlign;

struct _ReaderNode
{
	const FnCxtNode* reader_fn;
	ReaderAlign* alignment;

	/* ML: It's necessary to keep track of multiple readers 
	 * to know if a read is unique or not. Even if a function 
	 * is not the most recent reader (in the case a callee read
	 * from that address), the next read from a given function
	 * shouldn't count as unique if there hasn't been a write.
	 * 
	 * Each consecutive reader should be added to this linked list, 
	 * and every reader should get cleared upon a write
	 */
	ReaderNode* prev_reader;
};

struct _ReaderObject
{
	ReaderNode* reader_node;
};

struct _WriterObject
{
	const FnCxtNode* writer_fn;
	WriterAlign* alignment;
};

struct _ShadowObject
{
	WriterObject writer;
	ReaderObject reader;
};


/* Setting Shadow Objs */
_STATIC_INLINE_ void setWriterObjectAtAddr(const FnCxtNode* const writer, const UInt64 ea);
_STATIC_INLINE_ void resetReadersOnWrite(UInt64 ea);

_STATIC_INLINE_ void setReaderObjectAtAddr(const FnCxtNode* const reader, const UInt64 ea);
_STATIC_INLINE_ int initReaderNode(const FnCxtNode* const reader, const UInt64 ea);
_STATIC_INLINE_ int hasDupReaderNode(const FnCxtNode* const reader, const UInt64 ea);
_STATIC_INLINE_ void appendNewReaderNode(const FnCxtNode* const reader, const UInt64 ea);

/* Getting Shadow Objs */
_STATIC_INLINE_ const FnCxtNode* getWriterFnFromAddr(const UInt64 ea) __attribute__((always_inline));
_STATIC_INLINE_ WriterObject* getWriterObjectFromAddr(const UInt64 ea);

_STATIC_INLINE_ ReaderNode** getReaderNodeFromAddr(const UInt64 ea);
_STATIC_INLINE_ ReaderObject* getReaderObjectFromAddr(const UInt64 ea);
_STATIC_INLINE_ int isReaderFromAddr(const FnCxtNode* const reader, const UInt64 ea);

_STATIC_ char is_init_SM = 0;
_STATIC_ AccessAlign ReaderAlignment;
_STATIC_ AccessAlign WriterAlignment;

void initShadowObject(void** newSO, UInt64 sm_size)
{
	ShadowObject** SO = (ShadowObject**)newSO;
	*SO = SGL_MALLOC("init Shadow Memory", sm_size*sizeof(ShadowObject));
	//malloc big shadow memory chunk
	//malloc big allign memory chunk
	for (UInt64 i=0; i<sm_size; ++i)
	{
		SO[i]=SO[0]+sizeof(ShadowObject);
		SO[i]->reader.reader_node = NULL;
		SO[i]->writer.writer_fn = NULL;
		
		unsigned char j = i % 8;
		if (0 == j)
		{
		} else 
		{
			//set reader/writer align to previously initialized memory
		}
	}
}

/*********************************************************************/
/*                     Setting Shadow Objects                        */
/*********************************************************************/
/* 
 * TODO optimize alignments so that if an unaligned read/write is made
 * to an aligned address with the same previous reader/writer, no change 
 * in alignment occurs. Essentially this means ignoring alignment when 
 * the reader/writer doesn't change.
 *
 * TODO optimize reads/writes to be ignored if the reader/writer is the
 * same. Alignments should be used, so that an 8-byte aligned write with 
 * the same writer results in one check and then an exit.
 */
void setWriterToAddrRange(const FnCxtNode* const writer, const UInt64 ea, const UInt64 size)
{
	//set the writer for each shadow object
	for (UInt64 i=0;i<size;++i)
		setWriterObjectAtAddr(writer,ea+i);
	
	//set the write-alignment independent of the writer function
	setAlignment(getWriteAlignFromAddr,ea,size);

	/* The readers are kept to track unique accesses. A new write will
	   reset the readers, informing the next reader that the next access 
	   is unique */
	for(UInt64 i=0;i<size;++i)
		resetReadersOnWrite(ea+i);
}

void setReaderToAddrRange(const FnCxtNode* const reader, UInt64 ea, UInt64 size)
{
	checkAddrValid(ea+size-1);

	//set the reader for each shadow object
	for(UInt64 i=0;i<size;++i)
		setReaderObjectAtAddr(reader, ea+i);

	//set the read-alignment independent of the writer function
	setAlignment(getReadAlignFromAddr,ea,size);
}


/*********************************************************************/
/*                     Getting Shadow Objects                        */
/*********************************************************************/
UInt32 getTimesWtrInAddrRange(const FnCxtNode* const fn, const UInt64 ea, const UInt64 size)
{
	UInt32 cnt=0;;
	for(UInt64 i=0;i<size;)
	{
		UInt32 align_bytes = getAlignment(ea,getWriteAlignFromAddr);

		if (fn == getWriterFnFromAddr(ea+i))
			cnt+=align_bytes;

		i+=align_bytes;
	}
	return cnt;
}

FnCxtNodeList* getWritersFromAddrRange(const UInt64 ea, const UInt64 size)
{
	checkAddrValid(ea+size-1);

	FnCxtNodeList* writer_list = SGL_MALLOC("getWriters",sizeof(*writer_list));
	writer_list->next = NULL;
	writer_list->fn = NULL;
	for(UInt64 i=0;i<size;i++)
		appendFnToList(getWriterFnFromAddr(ea+i), writer_list);

	return writer_list;
}

_STATIC_INLINE_
const FnCxtNode* getWriterFnFromAddr(const UInt64 ea)
{
	const FnCxtNode* fn = getWriterObjectFromAddr(ea)->writer_fn;
	return fn;
}

UInt32 getTimesRdrInAddrRange(const FnCxtNode* const fn, const UInt64 ea, const UInt64 size)
{
	UInt32 cnt=0;;
	for(UInt64 i=0;i<size;)
	{
		UInt32 align_bytes = getAlignment(ea,getReadAlignFromAddr);

		if (isReaderFromAddr(fn,ea))
			cnt+=align_bytes;

		i+=align_bytes;
	}
	return cnt;
}

FnCxtNodeList* getReadersFromAddrRange(const UInt64 ea, const UInt64 size)
{
	checkAddrValid(ea+size-1);

	FnCxtNodeList* reader_list = SGL_MALLOC("getReaders",sizeof(*reader_list));
	reader_list->next = NULL;
	reader_list->fn = NULL;
	for(UInt64 i=0;i<size;i++){
		ReaderNode* reader_node;
		if ((reader_node=getReaderObjectFromAddr(ea+i)->reader_node))
		{
			do {
				appendFnToList( reader_node->reader_fn, reader_list );
				reader_node = reader_node->prev_reader;
			} while (NULL != reader_node);
		}
	}
	return reader_list;
}

_STATIC_INLINE_ 
int isReaderFromAddr(const FnCxtNode* const reader, const UInt64 ea)
{
	ReaderNode* reader_node;
	if ((reader_node=getReaderObjectFromAddr(ea)->reader_node))
	{
		do {
			if ( reader_node->reader_fn == reader )
				return 1;
			reader_node = reader_node->prev_reader;
		} while (NULL != reader_node);
	} 

	return 0;
}

/*********************************************************************/
/*                         Tracking Alignment                        */
/*********************************************************************/

_STATIC_INLINE_
void setWriterAlignment(AccessAlign*(*const getAlign)(const UInt64), const UInt64 ea, const UInt64 size)
{
}

_STATIC_INLINE_
void setReaderAlignment(AccessAlign*(*const getAlign)(const UInt64), const UInt64 ea, const UInt64 size)
{
}

_STATIC_INLINE_
void setAlignment(AccessAlign*(*const getAlign)(const UInt64), const UInt64 ea, const UInt64 size)
{
	UInt64 bytes_to_write = size;
	UInt64 curr_addr = ea;

	unsigned char is4aligned;
	unsigned char is8aligned;

	do {
		curr_addr = ea + size - bytes_to_write;
		is4aligned = IS4ALIGNED(curr_addr);
		is8aligned = IS8ALIGNED(curr_addr);

		if (bytes_to_write > 7 && is8aligned)
		{
			getAlign(curr_addr)->is8ByteAligned = 1;
			getAlign(curr_addr)->isUpper4ByteAligned = 1;
			getAlign(curr_addr)->isLower4ByteAligned = 1;

			bytes_to_write-=8;
		}else if ( bytes_to_write > 3 && is4aligned)
		{
			if ( (curr_addr % 8) > 3 )
				getAlign(curr_addr)->isUpper4ByteAligned = 1;
			else
				getAlign(curr_addr)->isLower4ByteAligned = 1;
			getAlign(curr_addr)->is8ByteAligned = 0;

			bytes_to_write-=4;
		}else
		{
			if ( (curr_addr % 8) > 3 )
				getAlign(curr_addr)->isUpper4ByteAligned = 0;
			else
				getAlign(curr_addr)->isLower4ByteAligned = 0;
			getAlign(curr_addr)->is8ByteAligned = 0;

			bytes_to_write--;
		}
	} while (bytes_to_write > 0);
}

UInt32 getReaderAlignment(const UInt64 ea)
{
	return getAlignment(ea, getReadAlignFromAddr);
}

UInt32 getWriterAlignment(const UInt64 ea)
{
	return getAlignment(ea, getReadAlignFromAddr);
}

_STATIC_INLINE_
UInt32 getAlignment(const UInt64 ea, AccessAlign*(*const getAlign)(const UInt64))
{
	UInt32 align_bytes = 1;
	if (getAlign(ea)->is8ByteAligned) 
		align_bytes = 8; 
	else if ((getAlign(ea)->isLower4ByteAligned) && (ea % 8 < 4)) 
		align_bytes = 4;
	else if (getAlign(ea)->isUpper4ByteAligned)
		align_bytes = 4;

	return align_bytes;
}

/*********************************************************************/
/*                        Utility Functions                          */
/*********************************************************************/
/********************************/
/*       Reader Helpers         */
/********************************/
_STATIC_INLINE_ 
void setReaderObjectAtAddr(const FnCxtNode* const reader, const UInt64 ea)
{
	if (!initReaderNode(reader,ea) && !hasDupReaderNode(reader,ea)){
		appendNewReaderNode(reader,ea);
	}
}

_STATIC_INLINE_
int initReaderNode(const FnCxtNode* const reader, const UInt64 ea)
{
	ReaderNode** curr = getReaderNodeFromAddr(ea);
	if (NULL == *curr){
		*curr = SGL_MALLOC("SM.newReader",sizeof(**curr));
		(*curr)->reader_fn = reader;
		(*curr)->prev_reader = NULL;
		return 1;
	}
	return 0;
}

_STATIC_INLINE_
int hasDupReaderNode(const FnCxtNode* const reader, const UInt64 ea)
{
	ReaderNode* dup = *getReaderNodeFromAddr(ea);
	do{
		if (dup->reader_fn == reader)
			return 1;
		dup = dup->prev_reader;
	} while(NULL != dup);
	return 0;
}

_STATIC_INLINE_
void appendNewReaderNode(const FnCxtNode* const reader, const UInt64 ea)
{
	ReaderNode* new = SGL_MALLOC("SM.newReader",sizeof(*new));
	new->reader_fn = reader;

	ReaderNode** curr = getReaderNodeFromAddr(ea);
	new->prev_reader = *curr;

	*curr = new;
}

_STATIC_INLINE_
ReaderNode** getReaderNodeFromAddr(const UInt64 ea)
{
	ReaderObject* ro = getReaderObjectFromAddr(ea);
	return &(ro->reader_node);
}

_STATIC_INLINE_
ReaderObject* getReaderObjectFromAddr(const UInt64 ea)
{
	ShadowObject* so = getSOFromAddr(ea);
	return &(so->reader);
}

_STATIC_INLINE_
AccessAlign* getReadAlignFromAddr(const UInt64 ea)
{
	return &(getSMFromAddr(ea)->read_aligns[SM_IDX(ea)>>3]);
}

/********************************/
/*       Writer Helpers         */
/********************************/
_STATIC_INLINE_ 
void setWriterObjectAtAddr(const FnCxtNode* const writer, const UInt64 ea)
{
	getWriterObjectFromAddr(ea)->writer_fn = writer;
}

_STATIC_INLINE_
void resetReadersOnWrite(const UInt64 ea)
{
	//traverse linked list and free each reader node
	ReaderNode* reader_node = getReaderObjectFromAddr(ea)->reader_node;

	while (NULL != reader_node){
		ReaderNode* prev_reader_node = reader_node->prev_reader;

		/* important to keep list 
		 * of readers duplicate free 
		 * to prevent double frees */
		free(reader_node);
		reader_node = prev_reader_node;
	}
	getReaderObjectFromAddr(ea)->reader_node = NULL;
}

_STATIC_INLINE_
WriterObject* getWriterObjectFromAddr(const UInt64 ea)
{
	ShadowObject* so = getSOFromAddr(ea);
	return &(so->writer);
}

_STATIC_INLINE_
AccessAlign* getWriteAlignFromAddr(const UInt64 ea)
{
	return &(getSMFromAddr(ea)->write_aligns[SM_IDX(ea)>>3]);
}


/* Helper to create a unique linked list */
_STATIC_INLINE_
void appendFnToList(const FnCxtNode* fn, FnCxtNodeList* list)
{
	if (NULL != list && NULL != fn){
		if (NULL == list->fn){
			list->fn = fn;
			list->next = NULL;
		}
		if (list->fn != fn){
			if (NULL == list->next){
				list->next = SGL_MALLOC("SGL.appendFnList",sizeof(*list));
				list->next->fn=fn;
				list->next->next=NULL;
			}
			else
				appendFnToList(fn, list->next);
		}
	}
	else{
		exitAtError("ShadowMemory.appendFnToList","Invalid list parameter");
	}
}

UInt32 getSizeOfList(const FnCxtNodeList* const list)
{
	//lists should typically be short enough
	//to allow for O(n) traversal
	if (NULL == list->fn)
		return 0;
	if (NULL == list->next)
		return 1;
	return 1 + getSizeOfList(list->next);
}

//TODO does this belong here?
void freeFnList(FnCxtNodeList* list)
{
	FnCxtNodeList* next_list;
	while (NULL != list)
	{
		next_list = list->next;
		free(list);
		list = next_list;
	}
}

