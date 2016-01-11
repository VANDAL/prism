#include "SigilMaster.h"
#include "FnMgmt.h"
#include "IRMgmt.h"

/**
 * How Sigil handles incoming notifications 
 * Interface to instrumentation will use these 
 * functions for each funcation call/return, IOP, etc
 */

void (* _notifyFunctionCall())(UInt32 fid, const char* name)
{
	return handleFnCall;
}

void (* _notifyFunctionReturn())()
{
	return handleFnReturn;
}

void (* _notifyIOP())(IOPType type, UInt32 size)
{
	return handleIOP;
}

void (* _notifyFLOP())(FLOPType type, UInt32 size)
{
	return handleFLOP;
}

void (* _notifyRead())(UInt64 addr, UInt64 bytes)
{
	return handleDataRead;
}

void (* _notifyWrite())(UInt64 addr, UInt64 bytes)
{
	return handleDataWrite;
}

void (* _notifyInstr())(UInt64 addr, UInt64 bytes)
{
	return handleInstr;
}

void (* _notifyThreadSwap())(UInt32 tid)
{
}

void (* _notifySync())(UInt32 tid)
{
}

void (* _finishSGL())()
{
}
