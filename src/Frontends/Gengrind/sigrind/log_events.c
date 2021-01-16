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
#include "sigil2_ipc.h"
#include "coregrind/pub_core_libcprint.h"

//TODO(someday) these aren't needed for Sigil, but deleting them causes
/* Following global vars are setup before by setup_bbcc():
 *
 * - Addr   CLG_(bb_base)     (instruction start address of original BB)
 * - ULong* CLG_(cost_base)   (start of cost array for BB)
 */
Addr   CLG_(bb_base);
ULong* CLG_(cost_base);

//#define COUNT_EVENT_CHECK
#ifdef COUNT_EVENT_CHECK
static unsigned long long mem_events = 0;
static unsigned long long comp_events = 0;
static unsigned long long sync_events = 0;
static unsigned long long cxt_events = 0;
#endif

void SGL_(end_logging)()
{
#ifdef COUNT_EVENT_CHECK
    VG_(printf)("Total Mem Events: %llu\n",  mem_events);
    VG_(printf)("Total Comp Events: %llu\n", comp_events);
    VG_(printf)("Total Sync Events: %llu\n", sync_events);
    VG_(printf)("Total Cxt Events: %llu\n",  cxt_events);
#endif
}


void SGL_(log_1I0D)(InstrInfo* ii)
{
    if (EVENT_GENERATION_ENABLED)
    {
#ifdef COUNT_EVENT_CHECK
        cxt_events++;
#endif
        unsigned char* buf = SGL_(reserve_ev_buf)(PRISM_BYTES_CXT + PRISM_BYTES_ADDR);
        *((uintptr_t*)buf) = 0;
        SET_EV_CXT_W_ID(buf, ii->instr_addr, PRISM_CXT_INSTR);
    }
}


/* Note that addEvent_D_guarded assumes that log_0I1Dr and log_0I1Dw
   have exactly the same prototype.  If you change them, you must
   change addEvent_D_guarded too. */
static inline void log_mem(Int type, Addr data_addr, Word data_size)
{
    if (EVENT_GENERATION_ENABLED)
    {
#ifdef COUNT_EVENT_CHECK
        ++mem_events;
#endif

        unsigned char* buf = SGL_(reserve_ev_buf)(PRISM_BYTES_MEM + PRISM_BYTES_ADDR);
        *((uintptr_t*)buf) = 0;
        SET_EV_TYPE(buf, PRISM_EVENTTYPE_MEM);
        SET_EV_MEM_TYPE(buf, type);
        SET_EV_MEM_SIZE(buf, data_size);
        SET_EV_MEM_ADDR(buf, data_addr);
    }
}
void SGL_(log_0I1Dr)(InstrInfo* ii, Addr data_addr, Word data_size_bytes)
{
    log_mem(PRISM_MEM_LOAD, data_addr, data_size_bytes >> 1);
}
void SGL_(log_0I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size_bytes)
{
    log_mem(PRISM_MEM_STORE, data_addr, data_size_bytes >> 1);
}


void SGL_(log_comp_event)(InstrInfo* ii, IRType op_type, IRExprTag arity)
{
    /* SIMD and decimal floating point are unsupported
     * See VEX/pub/libvex_ir.h : IROp
     * for future updates on specific ops */
    tl_assert(op_type < Ity_D32 || op_type == Ity_F128);

    if (EVENT_GENERATION_ENABLED)
    {
#ifdef COUNT_EVENT_CHECK
        ++comp_events;
#endif
        unsigned char* buf = SGL_(reserve_ev_buf)(1);
        SET_EV_TYPE(buf, PRISM_EVENTTYPE_COMP);

        if (op_type < Ity_F16)
        {
            SET_EV_COMP_FMT(buf, PRISM_COMP_IOP);
        }
        else
        {
            SET_EV_COMP_FMT(buf, PRISM_COMP_FLOP);
        }

        //switch (arity)
        //{
        //case Iex_Unop:
        //    slot->comp.arity = PRISM_COMP_UNARY;
        //    break;
        //case Iex_Binop:
        //    slot->comp.arity = PRISM_COMP_BINARY;
        //    break;
        //case Iex_Triop:
        //    slot->comp.arity = PRISM_COMP_TERNARY;
        //    break;
        //case Iex_Qop:
        //    slot->comp.arity = PRISM_COMP_QUARTERNARY;
        //    break;
        //default:
        //    tl_assert(False);
        //    break;
        //}
    }
}


void SGL_(log_sync)(UChar type, UWord data1, UWord data2)
{
    if (SGL_(clo).gen_sync == True)
    {
#ifdef COUNT_EVENT_CHECK
        ++sync_events;
#endif
        unsigned char* buf;
        if (data2 == UNUSED_SYNC_DATA)
        {
            buf = SGL_(reserve_ev_buf)(1 + sizeof(uintptr_t));
            SET_EV_TYPE(buf, PRISM_EVENTTYPE_SYNC);
            SET_EV_SYNC_TY(buf, type);
            *(uintptr_t*)(buf+1) = data1;
        }
        else
        {
            buf = SGL_(reserve_ev_buf)(1 + 2*sizeof(uintptr_t));
            SET_EV_TYPE(buf, PRISM_EVENTTYPE_SYNC);
            SET_EV_SYNC_TY(buf, type);
            *(uintptr_t*)(buf+1) = data1;
            *(uintptr_t*)(buf+1+sizeof(uintptr_t)) = data2;
        }
    }
}


static inline void log_fn(Int type, fn_node* fn)
{
    if (EVENT_GENERATION_ENABLED && SGL_(clo).gen_fn == True)
    {
#ifdef COUNT_EVENT_CHECK
        cxt_events++;
#endif
        Int actual_len = VG_(strlen)(fn->name);
        Int len = actual_len < 256 ? actual_len : 256;
        unsigned char* buf = SGL_(reserve_ev_buf)(2 + len);

        *buf = 0;
        SET_EV_TYPE(buf, PRISM_EVENTTYPE_CXT);
        SET_EV_CXT_TY(buf, type);
        ((unsigned char*)buf)[1] = (uint8_t)len - 1;
        VG_(memcpy)((unsigned char*)buf+2, fn->name, len);
    }
}
void SGL_(log_fn_entry)(fn_node* fn)
{
    log_fn(PRISM_CXT_FUNC_ENTER, fn);
}
void SGL_(log_fn_leave)(fn_node* fn)
{
    log_fn(PRISM_CXT_FUNC_EXIT, fn);
}


/***************************
 * Aggregate event logging
 ***************************/
void SGL_(log_2I0D)(InstrInfo* ii1, InstrInfo* ii2)
{
    if (EVENT_GENERATION_ENABLED)
    {
        SGL_(log_1I0D)(ii1);
        SGL_(log_1I0D)(ii2);
    }
}
void SGL_(log_3I0D)(InstrInfo* ii1, InstrInfo* ii2, InstrInfo* ii3)
{
    if (EVENT_GENERATION_ENABLED)
    {
        SGL_(log_1I0D)(ii1);
        SGL_(log_1I0D)(ii2);
        SGL_(log_1I0D)(ii3);
    }
}
void SGL_(log_1I1Dr)(InstrInfo* ii, Addr data_addr, Word data_size)
{
    if (EVENT_GENERATION_ENABLED)
    {
        SGL_(log_1I0D)(ii);
        SGL_(log_0I1Dr)(ii, data_addr, data_size);
    }
}
void SGL_(log_1I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size)
{
    if (EVENT_GENERATION_ENABLED)
    {
        SGL_(log_1I0D)(ii);
        SGL_(log_0I1Dw)(ii, data_addr, data_size);
    }
}

/***************************
 * Unimplemented Logging
 ***************************/
void SGL_(log_global_event)(InstrInfo* ii)
{
}
void SGL_(log_cond_branch)(InstrInfo* ii, Word taken)
{
}
void SGL_(log_ind_branch)(InstrInfo* ii, UWord actual_dst)
{
}
