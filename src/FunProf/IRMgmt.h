#ifndef _IR_MGMT_
#define _IR_MGMT_

#include "Common.h"

void handleDataRead(UInt64 addr, UInt64 bytes);
void handleDataWrite(UInt64 addr, UInt64 bytes);
void handleIOP(IOPType type, UInt32 size);
void handleFLOP(FLOPType type, UInt32 size);
void handleInstr(UInt64 addr, UInt64 bytes);

#endif
