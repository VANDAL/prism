#include "IRMgmt.h"
#include "ShadowMemory.h"
#include "FnMgmt.h"
#include "FnComm.h"

#define _STATIC_ static
#define _STATIC_INLINE_ static inline

void handleDataRead(UInt64 addr, UInt64 bytes)
{
	FnCxtNode* curr_fn = getCurrFnCxt();

	setReaderDependencies(curr_fn, addr, bytes);

	setReaderToAddrRange(curr_fn,addr,bytes);
}

void handleDataWrite(UInt64 addr, UInt64 bytes)
{
	FnCxtNode* curr_fn = getCurrFnCxt();

	setWriterToAddrRange(curr_fn,addr,bytes);
}

void handleIOP(IOPType type, UInt32 size)
{
	FnCxtNode* curr_fn = getCurrFnCxt();

	curr_fn->iops_cnt += size;
}

void handleFLOP(FLOPType type, UInt32 size)
{
	FnCxtNode* curr_fn = getCurrFnCxt();

	curr_fn->flops_cnt += size;
}

void handleInstr(UInt64 addr, UInt64 bytes)
{
	//nothing to do here
}
