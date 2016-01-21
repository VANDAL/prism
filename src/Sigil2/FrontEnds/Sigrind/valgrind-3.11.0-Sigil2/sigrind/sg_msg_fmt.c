#include "sg_msg_fmt.h"
#include "pub_tool_libcassert.h"

void genMemEvMsg(char* const msg, const MemType type, const Word64 addr, const Word64 size)
{
	SET_SGLMSG_EVTYPE(msg, MSG_MEM_EV);

	if ( type == SGLPRIM_MEM_LOAD)
	{
		msg[1] |= MSG_MEM_LOAD; 
	}
	else if ( type == SGLPRIM_MEM_STORE )
	{
		msg[1] |= MSG_MEM_STORE; 
	}

	union 
	{
		Word64 data;
		Byte byte[8];
	} addr_ = {0};

	union 
	{
		Word16 data;
		Byte byte[2];
	} size_ = {0};

	addr_.data = addr;
	for (UInt i=0; i<8; ++i)
	{
		msg[i+2] = addr_.byte[i];
	}

//TODO 	tl_assert( size < (1<<16) );
	size_.data = size;
	for (UInt i=0; i<2; ++i)
	{
		msg[i+10] = size_.byte[i];
	}
}

void genCompEvMsg(char* const msg, const IRType type, const IRExprTag arity)
{
	SET_SGLMSG_EVTYPE(msg, MSG_COMP_EV);

	if/*IOP*/( type < Ity_F32 )
	{
		msg[1] |= MSG_IOP_TYPE; 
	}
	else if/*FLOP*/( type < Ity_V128 )
	{
		msg[1] |= MSG_FLOP_TYPE; 
	}
	else
	{
		/*unhandled*/
		return;
	}

	switch (arity)
	{
	case Iex_Unop:
		msg[1] |= MSG_UNARY;
		break;
	case Iex_Binop:
		msg[1] |= MSG_BINARY;
		break;
	case Iex_Triop:
		msg[1] |= MSG_TERNARY;
		break;
	case Iex_Qop:
		msg[1] |= MSG_QUARTERNARY;
		break;
	default:
//TODO		tl_assert(0);
		break;
	}

	/* See VEX/pub/libvex_ir.h : IROp for 
	 * future updates on specific ops */
	/* TODO unimplemented */
	msg[2] = 0xFF;
	msg[3] = 0xFF;
	msg[4] = 0xFF;
}

void genSyncEvMsg(char* const msg, const UChar type, const UWord data)
{
	SET_SGLMSG_EVTYPE(msg, MSG_SYNC_EV);
	msg[1] |= type;

	union
	{
		UWord data;
		Byte byte[8];
	} sync_data;
	sync_data.data = data;

	for (UInt i=0; i<8; ++i)
	{
		msg[i+2] = sync_data.byte[i];
	}
}

