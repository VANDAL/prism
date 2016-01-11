#ifndef _FN_SHAD_MEM_H
#define _FN_SHAD_MEM_H

#include "SigilData.h"

/* 
 * Reader and writer functions are tracked for each address in
 * shadow memory in their respective shadow objects. 
 *
 * Because multiple functions can read from the same address, 
 * multiple readers are kept for each address. This happens
 * when a callee reads from the same address as the caller
 * (such as a reference to a pointer being passed and dereferenced).
 * When the callee finishes and the stack pops back to the caller,
 * the 
 *
 * and then cleared upon
 * a write because the new write implies a new dependency, breaking
 * the previous dependency between reads and the previous write.
 */

/* 
 * Sets specified addresses to a writer.
 */
void setWriterToAddrRange(const FnCxtNode* const writer, const UInt64 ea, const UInt64 size);

/* 
 * Sets specified addresses to a reader.
 */
void setReaderToAddrRange(const FnCxtNode* const reader, const UInt64 ea, const UInt64 size);

/* 
 * Returns a list of the last writers to a set of addresses.
 *
 * Checks the Sigil shadow memory for addresses ea to ea+size-1 for
 * the last functions to write to these addresses. The returned list
 * has no information linking the functions to an exact address, only
 * that the functions wrote to at least one address in the input
 * argument range.
 *
 * \attention The user of this function will be responsible for freeing
 * the returned linked list structure
 */
FnCxtNodeList* getWritersFromAddrRange(const UInt64 ea, const UInt64 size);
UInt32 getTimesWtrInAddrRange(const FnCxtNode* const fn, const UInt64 ea, const UInt64 size);
UInt32 setAlignWriterAndGetBytes(const FnCxtNode* writer, UInt64 addr, UInt64 byte_range);

/* 
 * Returns a list of the last readers to a set of addresses.
 *
 * Checks the Sigil shadow memory for addresses ea to ea+size-1 for
 * the last functions to read to these addresses. The returned list
 * has no information linking the functions to an exact address, only
 * that the functions read to at least one address in the input
 * argument range.
 *
 * \attention The user of this function will be responsible for freeing
 * the returned linked list structure
 */
FnCxtNodeList* getReadersFromAddrRange(const UInt64 ea, const UInt64 size);
UInt32 getTimesRdrInAddrRange(const FnCxtNode* const fn, const UInt64 ea, const UInt64 size);
UInt32 setAlignReaderAndGetBytes(const FnCxtNode* reader, const FnCxtNode *fn, UInt64 addr, UInt64 byte_range);

/*
 * Get the alignment of the last read/write operation for a specific address.
 */
UInt32 getReaderAlignment(const UInt64 ea);
UInt32 getWriterAlignment(const UInt64 ea);

#endif
