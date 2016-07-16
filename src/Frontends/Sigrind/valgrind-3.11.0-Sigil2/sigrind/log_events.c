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
#include "Frontends/Sigrind/SigrindIPC.h"

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

/* Sigrind IPC */
static int emptyfd;
static int fullfd;
static int open_fifo(const HChar *fifo_path, int flags);

static SigrindSharedData* shmem;
static SigrindSharedData* open_shmem(const HChar *shmem_path, int flags);

static BufferedSglEv* curr_buf;
static Bool is_full[SIGRIND_BUFNUM];
static UInt curr_idx;

static UInt curr_used;
static inline void incr_used(void);
static inline void update_curr_buf(void);

void SGL_(init_IPC)()
{
	if (SGL_(clo).tmpdir == NULL)
	{
	   VG_(fmsg)("No --tmp-dir argument found, shutting down...\n");
	   VG_(exit)(1);
	}
	else if (SGL_(clo).timestamp == NULL)
	{
	   VG_(fmsg)("No --timestamp argument found, shutting down...\n");
	   VG_(exit)(1);
	}

	Int tmpdir_len = VG_(strlen)(SGL_(clo).tmpdir);
	Int filename_len;

	//+1 for '/'; len should be strlen + null
	filename_len = tmpdir_len + VG_(strlen)(SIGRIND_SHMEM_NAME) + VG_(strlen)(SGL_(clo).timestamp) + 2;
	HChar shmem_path[filename_len];
	VG_(snprintf)(shmem_path, filename_len, "%s/%s%s", SGL_(clo).tmpdir, SIGRIND_SHMEM_NAME, SGL_(clo).timestamp); 

	filename_len = tmpdir_len + VG_(strlen)(SIGRIND_EMPTYFIFO_NAME) + VG_(strlen)(SGL_(clo).timestamp) + 2; 
	HChar emptyfifo_path[filename_len];
	VG_(snprintf)(emptyfifo_path, filename_len, "%s/%s%s", SGL_(clo).tmpdir, SIGRIND_EMPTYFIFO_NAME, SGL_(clo).timestamp); 

	filename_len = tmpdir_len + VG_(strlen)(SIGRIND_FULLFIFO_NAME) + VG_(strlen)(SGL_(clo).timestamp) + 2; 
	HChar fullfifo_path[filename_len];
	VG_(snprintf)(fullfifo_path, filename_len, "%s/%s%s", SGL_(clo).tmpdir, SIGRIND_FULLFIFO_NAME, SGL_(clo).timestamp); 

	///////////////////
	// init values 
	///////////////////
	shmem = open_shmem(shmem_path, VKI_O_RDWR);

	/* XXX Valgrind might get stuck waiting for Sigil
	 * if Sigil unexpectedly exits before trying
	 * to connect */
	emptyfd = open_fifo(emptyfifo_path, VKI_O_RDONLY);
	fullfd =  open_fifo(fullfifo_path, VKI_O_WRONLY);

	curr_used = 0;
	curr_idx = 0;
	for (UInt i=0; i<SIGRIND_BUFNUM; ++i)
	{
		is_full[i] = False;
	}
	curr_buf = shmem->buf[0];
}

void SGL_(finish_IPC)(void)
{
	/* send finish sequence */
	UInt finished = SIGRIND_FINISHED;
	if ( VG_(write)(fullfd, &finished, sizeof(finished)) != sizeof(finished) 
		|| VG_(write)(fullfd, &curr_idx, sizeof(curr_idx)) != sizeof(curr_idx) 
		|| VG_(write)(fullfd, &curr_used, sizeof(curr_used)) != sizeof(curr_idx) )
	{
		VG_(umsg)("error VG_(write)\n");
		VG_(umsg)("error writing to Sigrind fifo\n");
		VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
		VG_(exit)(1);
	}

	/* wait until Sigrind disconnects */
	while ( VG_(read)(emptyfd, &finished, sizeof(finished)) > 0 );

	VG_(close)(emptyfd);
	VG_(close)(fullfd);
}

/** 
 * Address not tracked yet for instructions.
 * Can track addresses by modifying addEvent_IR and log_<event>
 * to change arguments
 */
void SGL_(log_1I0D)(InstrInfo* ii)
{
	update_curr_buf();

	curr_buf[curr_used].tag = SGL_CXT_TAG;
	curr_buf[curr_used].cxt.type = SGLPRIM_CXT_INSTR;
	curr_buf[curr_used].cxt.id = ii->instr_addr;

	incr_used();
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
	update_curr_buf();

	curr_buf[curr_used].tag = SGL_MEM_TAG;
	curr_buf[curr_used].mem.type = SGLPRIM_MEM_LOAD;
	curr_buf[curr_used].mem.begin_addr = data_addr;
	curr_buf[curr_used].mem.size = data_size;

	incr_used();
}

/* See comment on log_0I1Dr. */
void SGL_(log_0I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size)
{
	update_curr_buf();

	curr_buf[curr_used].tag = SGL_MEM_TAG;
	curr_buf[curr_used].mem.type = SGLPRIM_MEM_STORE;
	curr_buf[curr_used].mem.begin_addr = data_addr;
	curr_buf[curr_used].mem.size = data_size;

	incr_used();
}

void SGL_(log_comp_event)(InstrInfo* ii, IRType type, IRExprTag arity)
{
	update_curr_buf();

	curr_buf[curr_used].tag = SGL_COMP_TAG;

	if/*IOP*/( type < Ity_F32 )
	{
		curr_buf[curr_used].comp.type = SGLPRIM_COMP_IOP;
	}
	else if/*FLOP*/( type < Ity_V128 )
	{
		curr_buf[curr_used].comp.type = SGLPRIM_COMP_FLOP;
	}
	else
	{
		/*unhandled*/
		return;
	}

	switch (arity)
	{
	case Iex_Unop:
		curr_buf[curr_used].comp.arity = SGLPRIM_COMP_UNARY;
		break;
	case Iex_Binop:
		curr_buf[curr_used].comp.arity = SGLPRIM_COMP_BINARY;
		break;
	case Iex_Triop:
		curr_buf[curr_used].comp.arity = SGLPRIM_COMP_TERNARY;
		break;
	case Iex_Qop:
		curr_buf[curr_used].comp.arity = SGLPRIM_COMP_QUARTERNARY;
		break;
	default:
		tl_assert(0);
		break;
	}

	/* See VEX/pub/libvex_ir.h : IROp for 
	 * future updates on specific ops */
	/* TODO unimplemented */
	
	incr_used();
}

void SGL_(log_sync)(UChar type, UWord data)
{
	update_curr_buf();

	curr_buf[curr_used].tag = SGL_SYNC_TAG;
	curr_buf[curr_used].sync.type = type;
	curr_buf[curr_used].sync.id = data;

	incr_used();
}

void SGL_(log_fn_entry)(fn_node* fn)
{
	//TODO
}

void SGL_(log_fn_leave)(fn_node* fn)
{
	//TODO
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


static int open_fifo(const HChar *fifo_path, int flags)
{
	int fd;
	SysRes res = VG_(open) (fifo_path, flags, 0600);
	if (sr_isError (res)) 
	{
		VG_(umsg) ("error %lu %s\n", sr_Err(res), VG_(strerror)(sr_Err(res)));
		VG_(umsg)("cannot open fifo file %s\n", fifo_path);
		VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
		VG_(exit) (1);
	} 

	fd = sr_Res(res);
	fd = VG_(safe_fd)(fd);
	if (fd == -1)
	{
		VG_(umsg)("FIFO for Sigrind failed\n");
		VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
		VG_(exit) (1);
	}

	return fd;
}

static SigrindSharedData* open_shmem(const HChar *shmem_path, int flags)
{

	SysRes res = VG_(open) (shmem_path, flags, 0600);
	if (sr_isError (res)) 
	{
		VG_(umsg) ("error %lu %s\n", sr_Err(res), VG_(strerror)(sr_Err(res)));
		VG_(umsg)("cannot open shared_mem file %s\n", shmem_path);
		VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
		VG_(exit) (1);
	} 

	int shared_mem_fd = sr_Res(res);

	res = VG_(am_shared_mmap_file_float_valgrind)
		(sizeof(SigrindSharedData), VKI_PROT_READ|VKI_PROT_WRITE, 
		 shared_mem_fd, (Off64T)0);
	if (sr_isError(res)) 
	{
		VG_(umsg) ("error %lu %s\n", sr_Err(res), VG_(strerror)(sr_Err(res)));
		VG_(umsg)("error VG_(am_shared_mmap_file_float_valgrind) %s\n", shmem_path);
		VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
		VG_(exit) (1);
	}  

	Addr addr_shared = sr_Res (res);
	VG_(close) (shared_mem_fd);

	return (SigrindSharedData*) addr_shared;
}

/* wait for an empty buffer notification if current buffer is full */
static inline void update_curr_buf(void)
{
	if ( is_full[curr_idx] )
	{
		if ( VG_(read)(emptyfd, &curr_idx, sizeof(curr_idx)) != sizeof(curr_idx) )
		{
			VG_(umsg)("error VG_(read)\n");
			VG_(umsg)("error reading from Sigrind fifo\n");
			VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
			VG_(exit)(1);
		}
		tl_assert(curr_idx < SIGRIND_BUFNUM);
		is_full[curr_idx] = 0;
		curr_buf = shmem->buf[curr_idx];
	}
}

/* inform sigil2 that the current buffer is used, increment to next buffer */
static inline void incr_used(void)
{
	tl_assert( !(curr_used > SIGRIND_BUFSIZE) );
	if (++curr_used == SIGRIND_BUFSIZE)
	{
		is_full[curr_idx] = True;

		if ( VG_(write)(fullfd, &curr_idx, sizeof(curr_idx)) != sizeof(curr_idx) )
		{
			VG_(umsg)("error VG_(write)\n");
			VG_(umsg)("error writing to Sigrind fifo\n");
			VG_(umsg)("Cannot recover from previous error. Good-bye.\n");
			VG_(exit)(1);
		}

		if (++curr_idx == SIGRIND_BUFNUM)
		{
			curr_idx = 0;
		}

		curr_buf = shmem->buf[curr_idx];
		curr_used = 0;
	}
}
