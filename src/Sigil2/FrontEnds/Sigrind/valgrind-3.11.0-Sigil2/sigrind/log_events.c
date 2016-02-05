/* This file is part of Callgrind, a Valgrind tool for call graph profiling programs.

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
#include "Sigil2/FrontEnds/Sigrind/ShMemData.h"

#include "coregrind/pub_core_libcfile.h"

#include "coregrind/pub_core_aspacemgr.h"
#include "coregrind/pub_core_syscall.h"
#include "pub_tool_basics.h"

//FIXME these aren't needed for Sigil, but deleting them causes 
/* Following global vars are setup before by setup_bbcc():
 *
 * - Addr   CLG_(bb_base)     (instruction start address of original BB)
 * - ULong* CLG_(cost_base)   (start of cost array for BB)
 */
Addr   CLG_(bb_base);
ULong* CLG_(cost_base);

/* Shared memory IPC */
static SigrindSharedData* SGL_(shared);

static inline unsigned int SGL_(incr)(unsigned int head)
{
	/* cache read */
	if ( head == SIGRIND_BUFSIZE-1 )
	{
		while (atomic_load_explicit(&(SGL_(shared)->tail), memory_order_relaxed) == 0);
		head = 0;
	}
	else
	{
		while (head == (atomic_load_explicit(&(SGL_(shared)->tail), memory_order_relaxed)-1));
		++head;
	}
	return head;
}

void SGL_(open_shmem)(HChar* tmp_dir, Int len)
{
	if (len < 2)
	{
	   VG_(fmsg)("No --tmp-dir argument found, shutting down...\n");
	   VG_(exit)(1);
	}

	//+1 for '/'; len should be strlen + null
	Int filename_len = len + VG_(strlen)(SIGRIND_SHMEM_NAME) + 1; 
	
	HChar *filename = VG_(malloc)("sgl.open_shmem",filename_len*sizeof(*filename));
	VG_(snprintf)(filename, filename_len, "%s/%s", tmp_dir, SIGRIND_SHMEM_NAME); 

	/* from remote-utils.c */
	SysRes o = VG_(open) (filename, VKI_O_RDWR, 0600);
	if (sr_isError (o)) 
	{
		VG_(umsg) ("error %lu %s\n", sr_Err(o), VG_(strerror)(sr_Err(o)));
		VG_(umsg)("cannot open shared_mem file %s\n", SIGRIND_SHMEM_NAME);
		VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
		VG_(exit) (1);
	} 

	int shared_mem_fd = sr_Res(o);

	SysRes res = VG_(am_shared_mmap_file_float_valgrind)
		(sizeof(SigrindSharedData), VKI_PROT_READ|VKI_PROT_WRITE, 
		 shared_mem_fd, (Off64T)0);
	if (sr_isError(res)) 
	{
		VG_(umsg) ("error %lu %s\n", sr_Err(res), VG_(strerror)(sr_Err(res)));
		VG_(umsg)("error VG_(am_shared_mmap_file_float_valgrind) %s\n", SIGRIND_SHMEM_NAME);
		VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
		VG_(exit) (1);
	}  

	Addr addr_shared = sr_Res (res);
	VG_(close) (shared_mem_fd);
	SGL_(shared) = (SigrindSharedData*) addr_shared;
}

void SGL_(close_shmem)(void)
{
	atomic_store_explicit(&(SGL_(shared)->sigrind_finish), True, memory_order_relaxed);
}


/** 
 * Address not tracked yet for instructions.
 * Can track addresses by modifying addEvent_IR and log_<event>
 * to change arguments
 */
void SGL_(log_1I0D)(InstrInfo* ii)
{
}
void SGL_(log_2I0D)(InstrInfo* ii1, InstrInfo* ii2)
{
	SGL_(log_1I0D)(ii1);
	SGL_(log_1I0D)(ii2);
}
void SGL_(log_3I0D)(InstrInfo* ii1, InstrInfo* ii2, InstrInfo* ii3)
{
	SGL_(log_1I0D)(ii1);
	SGL_(log_1I0D)(ii2);
	SGL_(log_1I0D)(ii3);
}

/* Instruction doing a read access */
void SGL_(log_1I1Dr)(InstrInfo* ii, Addr data_addr, Word data_size)
{
	SGL_(log_1I0D)(ii);
	SGL_(log_0I1Dr)(ii, data_addr, data_size);
}
/* Instruction doing a write access */
void SGL_(log_1I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size)
{
	SGL_(log_1I0D)(ii);
	SGL_(log_0I1Dw)(ii, data_addr, data_size);
}

/* Note that addEvent_D_guarded assumes that log_0I1Dr and log_0I1Dw
   have exactly the same prototype.  If you change them, you must
   change addEvent_D_guarded too. */
void SGL_(log_0I1Dr)(InstrInfo* ii, Addr data_addr, Word data_size)
{
	unsigned int head = atomic_load_explicit(&(SGL_(shared)->head), memory_order_relaxed);
	SGL_(shared)->buf[head].tag = SGL_MEM_TAG;
	SGL_(shared)->buf[head].mem.type = SGLPRIM_MEM_LOAD;
	SGL_(shared)->buf[head].mem.begin_addr = data_addr;
	SGL_(shared)->buf[head].mem.size = data_size;

	atomic_store_explicit(&(SGL_(shared)->head), SGL_(incr)(head), memory_order_release);
}

/* See comment on log_0I1Dr. */
void SGL_(log_0I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size)
{
	unsigned int head = atomic_load_explicit(&(SGL_(shared)->head), memory_order_relaxed);
	SGL_(shared)->buf[head].tag = SGL_MEM_TAG;
	SGL_(shared)->buf[head].mem.type = SGLPRIM_MEM_STORE;
	SGL_(shared)->buf[head].mem.begin_addr = data_addr;
	SGL_(shared)->buf[head].mem.size = data_size;

	atomic_store_explicit(&(SGL_(shared)->head), SGL_(incr)(head), memory_order_release);
}

void SGL_(log_comp_event)(InstrInfo* ii, IRType type, IRExprTag arity)
{
	unsigned int head = atomic_load_explicit(&(SGL_(shared)->head), memory_order_relaxed);
	SGL_(shared)->buf[head].tag = SGL_COMP_TAG;

	if/*IOP*/( type < Ity_F32 )
	{
		SGL_(shared)->buf[head].comp.type = SGLPRIM_COMP_IOP;
	}
	else if/*FLOP*/( type < Ity_V128 )
	{
		SGL_(shared)->buf[head].comp.type = SGLPRIM_COMP_FLOP;
	}
	else
	{
		/*unhandled*/
		return;
	}

	switch (arity)
	{
	case Iex_Unop:
		SGL_(shared)->buf[head].comp.arity = SGLPRIM_COMP_UNARY;
		break;
	case Iex_Binop:
		SGL_(shared)->buf[head].comp.arity = SGLPRIM_COMP_BINARY;
		break;
	case Iex_Triop:
		SGL_(shared)->buf[head].comp.arity = SGLPRIM_COMP_TERNARY;
		break;
	case Iex_Qop:
		SGL_(shared)->buf[head].comp.arity = SGLPRIM_COMP_QUARTERNARY;
		break;
	default:
		tl_assert(0);
		break;
	}

	/* See VEX/pub/libvex_ir.h : IROp for 
	 * future updates on specific ops */
	/* TODO unimplemented */

	atomic_store_explicit(&(SGL_(shared)->head), SGL_(incr)(head), memory_order_release);
}

void SGL_(log_sync)(UChar type, UWord data)
{
	unsigned int head = atomic_load_explicit(&(SGL_(shared)->head), memory_order_relaxed);
	SGL_(shared)->buf[head].tag = SGL_SYNC_TAG;
	SGL_(shared)->buf[head].sync.type = type;
	SGL_(shared)->buf[head].sync.id = data;

	atomic_store_explicit(&(SGL_(shared)->head), SGL_(incr)(head), memory_order_release);
}

void SGL_(log_fn_entry)(fn_node* fn)
{
}

void SGL_(log_fn_leave)(fn_node* fn)
{
}


void SGL_(log_global_event)(InstrInfo* ii)
{
}
void SGL_(log_cond_branch)(InstrInfo* ii, Word taken)
{
}
void SGL_(log_ind_branch)(InstrInfo* ii, UWord actual_dst)
{
}
