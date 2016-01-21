#ifndef SGL_MSG_FMT_H
#define SGL_MSG_FMT_H

/* 
 * Because Valgrind tools are not meant to utilize third party libraries,
 * per Valgrind's documentation, the Valgrind-Sigil front end sends events
 * captured in Valgrind to Sigil via Valgrind's built-in socket logging
 * functionality.
 *
 * Sigil messages are generally encoded in nibble chunks (4-bits at a time).
 * Each nibble represents a piece of data, e.g. event type and event fields,
 * however some event types may vary the number of bits in a field, such as 
 * for sending strings.
 *
 * A separate socket listener is needed to capture and decode these messages
 * for Sigil to process the events.
 */

/* TODO conversion from type-to-char, with unions are not compatible across 
 * different platforms */

#include "Sigil2/PrimitiveEnums.h"
#include <stdint.h>

#include "pub_tool_machine.h"

#define ALLOCA_SGLMSG(name, size) \
   char name[size]; \
	VG_(memset)(name, 0, size*sizeof(char)); \
	name[size-1] = '\0'; \
	name[0] = 0xF0;
   
#define SET_SGLMSG_EVTYPE(buf,type) (buf[0] |= type)
#define MSG_COMP_EV 0x01
#define MSG_MEM_EV  0x02
#define MSG_SYNC_EV 0x03
#define MSG_CXT_EV  0x04

#define COMP_BUF_SIZE 7
#define MEM_BUF_SIZE  12
#define SYNC_BUF_SIZE 10
#define CXT_BUF_SIZE  128

/* Compute bits */
#define MSG_IOP_TYPE 0x00 //byte 1
#define MSG_FLOP_TYPE 0x10 //byte 1

#define MSG_NULLARY 0x00
#define MSG_UNARY 0x01
#define MSG_BINARY 0x02
#define MSG_TERNARY 0x03
#define MSG_QUARTERNARY 0x04

/* Memory Access bits */
#define MSG_MEM_LOAD 0x10
#define MSG_MEM_STORE 0x20

/* Synchronization bits */

/* Workload Context bits */
#define MSG_CXT_TYPE_IDX 1
#define MSG_INSTR_TYPE 0x20

typedef uint64_t Word64;
typedef uint16_t Word16;
typedef uint8_t Byte;

/* msg must have MEM_BUF_SIZE allocated */
void genMemEvMsg(char* const msg, const MemType type, const Word64 addr, const Word64 size);

/* msg must have COMP_BUF_SIZE allocated */
void genCompEvMsg(char* const msg, const IRType type, const IRExprTag arity);

/* msg must have COMP_BUF_SIZE allocated */
void genSyncEvMsg(char* const msg, const UChar type, const UWord data);

#endif
