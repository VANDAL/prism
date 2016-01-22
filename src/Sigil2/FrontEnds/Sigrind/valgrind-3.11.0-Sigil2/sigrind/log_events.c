/*
   This file is part of Callgrind, a Valgrind tool for call graph profiling programs.

   Copyright (C) 2003-2015, Josef Weidendorfer (Josef.Weidendorfer@gmx.de)

   This tool is derived from and contains code from Cachegrind
   Copyright (C) 2002-2015 Nicholas Nethercote (njn@valgrind.org)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "log_events.h"
#include "sg_msg_fmt.h"

#include <stdint.h>

/* Sigrind only supports socket logging */
#include "coregrind/pub_core_libcprint.h"
#include "coregrind/pub_core_libcfile.h"


//FIXME these isn't needed for Sigil, but deleting them causes 
/* Following global vars are setup before by setup_bbcc():
 *
 * - Addr   CLG_(bb_base)     (instruction start address of original BB)
 * - ULong* CLG_(cost_base)   (start of cost array for BB)
 */
Addr   CLG_(bb_base);
ULong* CLG_(cost_base);



/*------------------------------------------------------------*/
/*--- Helper functions called by instrumented code         ---*/
/*------------------------------------------------------------*/

static void send_to_sigil_socket(const char *const buf, const UInt size)
{
	Int rc = VG_(write_socket)( VG_(log_output_sink).fd, buf, size );

	if (rc == -1) 
	{
		 // FIXME handle gracefully
         // For example, the listener process died.  Switch back to stderr.
		 const char msg[32] = "Could not write to socket\n";
         VG_(log_output_sink).is_socket = False;
         VG_(log_output_sink).fd = 2;
         VG_(write)( VG_(log_output_sink).fd, msg, 32 );

		 VG_(exit)(-1);
	}
}


/** 
 * Address not tracked yet for instructions.
 * Can track addresses by modifying addEvent_IR and log_<event>
 * to change arguments
 */
void log_1I0D(InstrInfo* ii)
{
	ALLOCA_SGLMSG(sglmsg, CXT_BUF_SIZE);
	SET_SGLMSG_EVTYPE(sglmsg, MSG_CXT_EV);

	sglmsg[MSG_CXT_TYPE_IDX] |= 0x20; //< Instruction Marker
}
void log_2I0D(InstrInfo* ii1, InstrInfo* ii2)
{
	log_1I0D(ii1);
	log_1I0D(ii2);
}
void log_3I0D(InstrInfo* ii1, InstrInfo* ii2, InstrInfo* ii3)
{
	log_1I0D(ii1);
	log_1I0D(ii2);
	log_1I0D(ii3);
}

/* Instruction doing a read access */
void log_1I1Dr(InstrInfo* ii, Addr data_addr, Word data_size)
{
	log_1I0D(ii);
	log_0I1Dr(ii, data_addr, data_size);
}

/* Note that addEvent_D_guarded assumes that log_0I1Dr and log_0I1Dw
   have exactly the same prototype.  If you change them, you must
   change addEvent_D_guarded too. */
void log_0I1Dr(InstrInfo* ii, Addr data_addr, Word data_size)
{
	ALLOCA_SGLMSG(dr_sglmsg, MEM_BUF_SIZE);
	genMemEvMsg(dr_sglmsg, SGLPRIM_MEM_LOAD, data_addr, data_size);
	send_to_sigil_socket(dr_sglmsg, MEM_BUF_SIZE);
}

/* Instruction doing a write access */
void log_1I1Dw(InstrInfo* ii, Addr data_addr, Word data_size)
{
	log_1I0D(ii);
	log_0I1Dw(ii, data_addr, data_size);
}

/* See comment on log_0I1Dr. */
void log_0I1Dw(InstrInfo* ii, Addr data_addr, Word data_size)
{
	ALLOCA_SGLMSG(dw_sglmsg, MEM_BUF_SIZE);
	genMemEvMsg(dw_sglmsg, SGLPRIM_MEM_STORE, data_addr, data_size);
	send_to_sigil_socket(dw_sglmsg, MEM_BUF_SIZE);
}

void log_comp_event(InstrInfo* ii, IRType type, IRExprTag arity)
{
	ALLOCA_SGLMSG(comp_sglmsg, COMP_BUF_SIZE);
	genCompEvMsg(comp_sglmsg, type, arity);
	send_to_sigil_socket(comp_sglmsg, COMP_BUF_SIZE);
}

void log_fn_entry(fn_node* fn)
{
}

void log_fn_leave(fn_node* fn)
{
}

void log_sync(UChar type, UWord data)
{
	ALLOCA_SGLMSG(sync_sglmsg, SYNC_BUF_SIZE);
	SET_SGLMSG_EVTYPE(sync_sglmsg, MSG_SYNC_EV);
	genSyncEvMsg(sync_sglmsg, type, data); 
	send_to_sigil_socket(sync_sglmsg, SYNC_BUF_SIZE);
}

void log_global_event(InstrInfo* ii)
{
}


/********************************************/
/* Branches aren't implemented by Sigil yet */
/********************************************/
void log_cond_branch(InstrInfo* ii, Word taken)
{
}
void log_ind_branch(InstrInfo* ii, UWord actual_dst)
{
}
