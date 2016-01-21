#include "MsgBuilder.hpp"
#include "Sigil2/InstrumentationIface.h" 
#include "Sigil2/PrimitiveEnums.h"

#include <iostream>
#include <cassert>

#include "valgrind-3.11.0-Sigil2/sigrind/sg_msg_fmt.h"
/******************************************************/
/* Buffer macros
 *
 * Each macro takes a 1-byte value, and checks it
 * for various encoded flags. The validity of the
 * macros/flags depends on the context of
 * the whole buffer (i.e. the previous bytes in the stream) 
 *
 * FIXME little endian is assumed */

#define NEW_EVENT(x) ((x & 0xF0) == 0xF0)
//#define END_EVENT(x) (x  == '\n')

#define EVENT_TYPE(x) (x & 0x0F)
/******************************************************/

static bool is_active = false;
static void sendCompEvent( const VGBufState& buf_state );
static void sendSyncEvent( const VGBufState& buf_state );
static void sendMemEvent( const VGBufState& buf_state );
static void sendCxtEvent( const VGBufState& buf_state );

void parseSglMsg ( const char* const buf, const int nbuf, VGBufState& buf_state )
{
	for(int i = 0; i < nbuf; i++) 
	{
		if( is_active ) 
		{
			buf_state.msg[buf_state.used++] = buf[i];
			if( buf_state.used == buf_state.size )
			{
				sendToSgl(buf_state);

				buf_state.prim_type = 0;
				buf_state.size = 0;
				buf_state.used = 0;

				is_active = false;
			}
			assert( !(buf_state.used > buf_state.size) );
		} 
		else if( NEW_EVENT(buf[i]) ) 
		{
			is_active = true;

			buf_state.msg[0] = buf[i];
			buf_state.used=1;
			buf_state.prim_type = EVENT_TYPE(buf[i]);
			switch( buf_state.prim_type )
			{
			case MSG_COMP_EV:
				buf_state.size = COMP_BUF_SIZE;
				break;
			case MSG_MEM_EV:
				buf_state.size = MEM_BUF_SIZE;
				break;
			case MSG_SYNC_EV:
				buf_state.size = SYNC_BUF_SIZE;
				break;
			case MSG_CXT_EV:
				buf_state.size = CXT_BUF_SIZE;
				break;
			default:
				break;
			}
		}
		else/*not a sigil message*/
		{
			std::cout << buf[i]; 
		}
	}
}

void sendToSgl ( const VGBufState& data )
{
	switch( data.prim_type )
	{
	case MSG_COMP_EV:
		sendCompEvent(data);
		break;
	case MSG_MEM_EV:
		sendMemEvent(data);
		break;
	case MSG_SYNC_EV:
		sendSyncEvent(data);
		break;
	case MSG_CXT_EV:
		sendCxtEvent(data);
		break;
	default:
		break;
	}
}

#define COMP_OP_TYPE(x) (x[1] & 0xF0)
#define COMP_ARITY(x) (x[1] & 0x0F)
void sendCompEvent( const VGBufState& data )
{
	SglCompEv ev;
	switch( COMP_OP_TYPE(data.msg) )
	{
	case MSG_IOP_TYPE:
		ev.type = CompCostType::SGLPRIM_COMP_IOP;
		break;
	case MSG_FLOP_TYPE:
		ev.type = CompCostType::SGLPRIM_COMP_FLOP;
		break;
	default:
		break;
	}
	switch( COMP_ARITY(data.msg) )
	{
	case MSG_NULLARY:
		ev.arity = CompArity::SGLPRIM_COMP_NULLARY;
		break;
	case MSG_UNARY:
		ev.arity = CompArity::SGLPRIM_COMP_UNARY;
		break;
	case MSG_BINARY:
		ev.arity = CompArity::SGLPRIM_COMP_BINARY;
		break;
	case MSG_TERNARY:
		ev.arity = CompArity::SGLPRIM_COMP_TERNARY;
		break;
	case MSG_QUARTERNARY:
		ev.arity = CompArity::SGLPRIM_COMP_QUARTERNARY;
		break;
	default:
		break;
	}
	SGLnotifyComp(ev);
}

#define MEM_TYPE(x) (x[1] & 0xF0)
#define MSG_MEM_LOAD 0x10
#define MSG_MEM_STORE 0x20
void sendMemEvent( const VGBufState& data )
{
	union
	{
		uint64_t data;
		char byte[8];
	} addr;
	for (int i=0; i<8; ++i)
	{
		addr.byte[i] = data.msg[i+2];
	}

	union
	{
		uint16_t data;
		char byte[2];
	} size;
	for (int i=0; i<2; ++i)
	{
		size.byte[i] = data.msg[i+10];
	}

	SglMemEv ev;
	ev.begin_addr = addr.data;
	ev.size = size.data;
	switch( MEM_TYPE(data.msg) )
	{
	case MSG_MEM_LOAD:
		ev.type = MemType::SGLPRIM_MEM_LOAD;
		break;
	case MSG_MEM_STORE:
		ev.type = MemType::SGLPRIM_MEM_STORE;
		break;
	default:
		break;
	}
	SGLnotifyMem(ev);
}

#define SYNC_TYPE(x) (x[1] & 0xFF)
#define MSG_SYNC_CREATE 0x10
void sendSyncEvent( const VGBufState& data )
{
	SglSyncEv ev;
	ev.type = (SyncType)SYNC_TYPE(data.msg); //Event Primitive Enum directly coded

	union
	{
		uint64_t data;
		uint8_t byte[8];	
	} sync_data;
	for (int i=0; i<8; ++i)
	{
		sync_data.byte[i] = data.msg[i+2];
	}

	ev.id = sync_data.data;
	SGLnotifySync(ev);
}


#define CXT_TYPE(x) (x[1] & 0xF0)
#define CXT_SUBTYPE(x) (x[1] & 0x0F)

#define MSG_CXT_FUNC 0x80
#define MSG_CXT_FUNC_CALL 0x01
#define MSG_CXT_FUNC_RET 0x02

#define MSG_CXT_THREAD 0x40
#define MSG_CXT_THREAD_SWAP 0x01

/* WIP */
void sendCxtEvent( const VGBufState& data )
{
	SglCxtEv ev;
	switch( CXT_TYPE(data.msg) )
	{
	case MSG_CXT_FUNC:
	{
		ev.type = CxtType::SGLPRIM_CXT_FUNC;
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
