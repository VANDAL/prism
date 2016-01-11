#ifndef _FN_ALIGNMENT_H
#define _FN_ALIGNMENT_H

#include "Common.h"
/* 
 * ML: Alignment information is not necessary but may speed
 * up queries to the last writer/reader of multiple bytes
 * 
 * Theoretically only one access needs to be kept for an
 * aligned access (for example only one pointer needs to track
 * a writer if a write is made to 4 bytes, instead of 4 pointers,
 * but this is an optimization for later profiling
 *
 * Alignment is stored per 8-byte word. Each alignment metadata
 * tracks whether the same function read/wrote the same:
 *	1) 8 bytes
 *	2) upper 4 bytes
 *	3) lower 4 bytes
 *
 *	for a given 8-byte word span
 *
 *	Because multiple readers are tracked, the alignment of those
 *	reads also needs to be tracked
 */
typedef struct _AccessAlign AccessAlign;
struct _AccessAlign
{
	unsigned char isUpper4ByteAligned : 1;
	unsigned char isLower4ByteAligned : 1;
	unsigned char is8ByteAligned : 1;
};

#endif
