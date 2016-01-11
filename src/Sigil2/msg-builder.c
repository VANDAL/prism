#include "msg-builder.h"
#include "FrontIface.h" 
#include <iostream>
/******************************************************/
/* Buffer macros
 *
 * Each macro takes a 1-byte value, and checks it
 * for various encoded flags. The validity of the
 * macros/flags depends on the context of
 * the whole buffer (i.e. the previous bytes in the stream) */

#define NEW_EVENT(x) ((x & 0xF0) == 0xF0)
#define END_EVENT(x) (x  == '\n')

#define EVENT_TYPE(x) (x & 0x0F)

#define MSG_COMP_TYPE 0x01
#define MSG_MEM_TYPE  0x02
#define MSG_SYNC_TYPE 0x03
#define MSG_CXT_TYPE  0x04

#define COMP_BUF_SIZE 7;
#define MEM_BUF_SIZE  0;
#define SYNC_BUF_SIZE 0;
#define CXT_BUF_SIZE  128;
/******************************************************/

static void sendComp( const VGBufState& buf_state );
static void sendSync( const VGBufState& buf_state );
static void sendMem( const VGBufState& buf_state );
static void sendCxt( const VGBufState& buf_state );

void parseSglMsg ( unsigned char* buf, int nbuf, VGBufState& buf_state )
{
	for(int i = 0; i < nbuf; i++) 
	{
		if( END_EVENT(buf[i]) ) 
		{
			sendToSgl(buf_state);
			buf_state.prim_type = 0;
			buf_state.size = 0;
			buf_state.used = 0;
		} 
		else if( NEW_EVENT(buf[i]) ) 
		{
			buf_state.msg[0] = buf[i];
			buf_state.used=1;
			buf_state.prim_type = EVENT_TYPE(buf[i]);
			switch( buf_state.prim_type )
			{
				case MSG_COMP_TYPE:
					buf_state.size = COMP_BUF_SIZE;
					break;
				case MSG_MEM_TYPE:
					buf_state.size = MEM_BUF_SIZE;
					break;
				case MSG_SYNC_TYPE:
					buf_state.size = SYNC_BUF_SIZE;
					break;
				case MSG_CXT_TYPE:
					buf_state.size = CXT_BUF_SIZE;
					break;
				default:
					break;
			}
		}
		else/*continuing previous event*/
		{
			//TODO check used < size
			buf_state.msg[buf_state.used++] = buf[i];
		}
	}
}

void sendToSgl ( VGBufState data )
{
	switch( data.prim_type )
	{
		case MSG_COMP_TYPE:
			sendComp(data);
			break;
		case MSG_MEM_TYPE:
			break;
		case MSG_SYNC_TYPE:
			break;
		case MSG_CXT_TYPE:
			sendCxt(data);
			break;
		default:
			break;
	}
}

#define COMP_OP_TYPE(x) (x[1] & 0xF0)
#define MSG_IOP_TYPE 0x00
#define MSG_FLOP_TYPE 0x10

#define COMP_ARITY(x) (x[1] & 0x0F)
#define MSG_NULLARY 0x00
#define MSG_UNARY 0x01
#define MSG_BINARY 0x02
#define MSG_TERNARY 0x03
#define MSG_QUARTERNARY 0x04

void sendComp( const VGBufState& data )
{
	SglCompEv ev;
	switch( COMP_OP_TYPE(data.msg) )
	{
		case MSG_IOP_TYPE:
			ev.type = COMP_IOP;
			break;
		case MSG_FLOP_TYPE:
			ev.type = COMP_FLOP;
			break;
		default:
			break;
	}
	switch( COMP_ARITY(data.msg) )
	{
		case MSG_NULLARY:
			ev.arity = COMP_NULLARY;
			break;
		case MSG_UNARY:
			ev.arity = COMP_UNARY;
			break;
		case MSG_BINARY:
			ev.arity = COMP_BINARY;
			break;
		case MSG_TERNARY:
			ev.arity = COMP_TERNARY;
			break;
		case MSG_QUARTERNARY:
			ev.arity = COMP_QUARTERNARY;
			break;
		default:
			break;
	}
	//SGLnotifyComp(ev);
}

#define SYNC_TYPE(x) (x[1] & 0xF0)

#define MSG_SYNC_CREATE 0x10

void sendSync( const VGBufState& data )
{
	SglSyncEv ev;
	switch( SYNC_TYPE(data.msg) )
	{
		case MSG_SYNC_CREATE:
			//we have the thread id
			//we have the addresss
			break;
		default:
			break;
	}
}

void sendMem( const VGBufState& data )
{
}

#define CXT_TYPE(x) (x[1] & 0xF0)
#define CXT_SUBTYPE(x) (x[1] & 0x0F)

#define MSG_CXT_FUNC 0x80
#define MSG_CXT_FUNC_CALL 0x01
#define MSG_CXT_FUNC_RET 0x02

#define MSG_CXT_THREAD 0x40
#define MSG_CXT_THREAD_SWAP 0x01

void sendCxt( const VGBufState& data )
{
	SglCxtEv ev;
	switch( CXT_TYPE(data.msg) )
	{
		case MSG_CXT_FUNC:
		{
			ev.type = CXT_FUNC;
			std::string fn_name(reinterpret_cast<const char*>(data.msg+2), data.used-2);
			switch ( CXT_SUBTYPE(data.msg) )
			{
				case MSG_CXT_FUNC_CALL:
					//std::cout << fn_name << " CALL " << std::endl;
					ev.op = 0;
					break;
				case MSG_CXT_FUNC_RET:
					//std::cout << fn_name << " RET " << std::endl;
					ev.op = 1;
					break;
				default:
					break;
			}
		}
			break;
		case MSG_CXT_THREAD:
		{
			switch( CXT_SUBTYPE(data.msg) )
			{
				case MSG_CXT_THREAD_SWAP:
				std::cout <<  "ThreadSwap " << std::endl;
					break;
				default:
					break;
			}
		}
			break;
		default:
			break;
	}
	//SGLnotifyCxt(ev);
}

