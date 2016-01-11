#include "FnAlignment.h"

#define _STATIC_ static
#define _STATIC_INLINE_ static inline

#define IS4ALIGNED(ea)	(((ea % 4) == 0) ? 1 : 0)
#define IS8ALIGNED(ea)	(((ea % 8) == 0) ? 1 : 0)

_STATIC_INLINE_ void setAlignment(AccessAlign*(*const getAlign)(const UInt64), const UInt64 ea, const UInt64 size);
_STATIC_INLINE_ UInt32 getAlignment(const UInt64 ea, AccessAlign*(*const getAlign)(const UInt64));
_STATIC_INLINE_ AccessAlign* getWriteAlignFromAddr(const UInt64 ea);
_STATIC_INLINE_ AccessAlign* getReadAlignFromAddr(const UInt64 ea);

//typedef struct _AccessAlign AccessAlign;
//	AccessAlign read_aligns[SM_SIZE/8];
//	AccessAlign write_aligns[SM_SIZE/8];
