#ifndef _FN_COMM_H
#define _FN_COMM_H

/*
 * Process unique and non-unique (reused) data communication existing 
 * between functions.
 *
 * Interfaces with shadow memory to update reader and writer function
 * contexts in the shadow objects for the address space.
 */

#include "Common.h"
#include "SigilData.h"

void setReaderDependencies(const FnCxtUID curr_reader, UInt64 addr,	UInt64 bytes);
void updateWriterInShadMem(UInt64 ea, UInt64 datasize);
void updateReaderInShadMem(UInt64 ea, UInt64 datasize);
void updateReaderCommStats(UInt64 ea, UInt64 datasize);

#endif
