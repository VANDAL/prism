
/*--------------------------------------------------------------------*/
/*--- Gengrind: The event generation Valgrind tool.      gn_main.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Gengrind, the event generation Valgrind tool

   Copyright (C) 2017-2018 Michael Lui
      mike.d.lui@gmail.com

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

/* N.B. much of the function tracking code throughout is based on
 * Callgrind's implementation.
 * Most modifications to the Callgrind base are removing unused
 * functionality or stylistic changes. */

#include "gn.h"
#include "gn_ipc.h"
#include "gn_clo.h"
#include "gn_bb.h"
#include "gn_events.h"
#include "gn_threads.h"
#include "gn_crq.h"
#include "gn_callstack.h"
#include "gn_jumps.h"
#include "gn_debug.h"


static void gn_post_clo_init(void)
{
    if (GN_(clo).gen_fn) {
        GN_(clo).bbinfo_needed = True;
    }

    GN_(initIPC)();
    GN_(initializeThreadState)();

    if (GN_(clo).gen_fn == True) {
        GN_(initBB)();
        GN_(initCallStack)();
        GN_(initJumpTable)();
        GN_(lastJmpsPassed) = 0;
    }

    /* start generating events right away or not */
    if (GN_(clo).start_collect_func == NULL)
        GN_(afterStartFunc) = True;
    else
        GN_(afterStartFunc) = False;
}


static
IRSB* gn_instrument ( VgCallbackClosure* closure,
                      IRSB* obb,
                      const VexGuestLayout* layout,
                      const VexGuestExtents* vge,
                      const VexArchInfo* archinfo_host,
                      IRType gWordTy, IRType hWordTy )
{
    /* Each uninstrumented basic block (OBB) is passed to this function.
     * An instrumented basic block (NBB) is generated from OBB and then passed
     * back to Valgrind to use for execution.
     *
     * Each VEX IR statement (see libvex_ir.h) in the OBB is iterated upon,
     * and metadata is temporarily stored, e.g. memory addresses.
     * After each iteration, the VEX IR stmt is placed in the NBB.
     *
     * Upon reaching a possible Exit or the end of the OBB, all the temporary
     * metadata captured up to that point is flushed before adding that VEX IR
     * stmt.
     * Flushing happens by inserting additional IR to write all the captured
     * metadata to a buffer, and then resetting the metadata.
     * Resetting metadata should take place during a flush.
     *
     * Some metadata needs to be available before any other VEX IR is inserted
     * into the NBB. In this case, instrumentation will be inserted before
     * iteration of the OBB.
     */

    if (gWordTy != hWordTy) {
        // We don't currently support this case.
        // From 'pub_tool_tooliface.h' 20171026:
        // "They [gWordTy/hWordTy] will by [be] either Ity_I32 or Ity_I64.
        // So far we have never built a cross-architecture
        // Valgrind so they should always be the same."
        VG_(tool_panic)("host/guest word size mismatch");
    }

    // No instrumentation if it is switched off
    if (!GN_(clo).enable_instrumentation) {
        GN_DEBUG(5, "instrument(BB %#lx) [Instrumentation OFF]\n",
                  (Addr)closure->readdr);
        return obb;
    }

    BBState bbState;
    Int i = GN_(initBBState)(&bbState, obb, hWordTy);

    GN_DEBUG(3, "+ instrument(BB %#lx)(%u)\n", (Addr)closure->readdr, bbState.bbInfo->uid);

    // pre-BB instrumentation
    {
        if (GN_(clo).gen_fn == True)
            GN_(add_TrackFns)(&bbState); // Function call/return tracking

        if (GN_(clo).gen_sync == True)
            GN_(add_TrackSyncs)(&bbState); // Need to setup thread context instrumentation
    }

    // BB instrumentation
    Int curr_instr_idx = -1;
    Int prev_flushed_idx = -1;

    for (/*use current i*/; i < obb->stmts_used; ++i) {
        const IRStmt *st = obb->stmts[i];
        GN_ASSERT(isFlatIRStmt(st));

        switch (st->tag) {
        case Ist_NoOp:
        case Ist_AbiHint:
        case Ist_Put:
        case Ist_PutI:
        case Ist_MBE:
            break;
        case Ist_IMark:
            curr_instr_idx = i;
            GN_(addEvent_Instr)(&bbState, st);
            break;
        case Ist_WrTmp:
            switch (st->Ist.WrTmp.data->tag) {
            case Iex_Load:
                GN_(addEvent_Memory_Load)(&bbState, st);
                break;
            case Iex_Unop:
            case Iex_Binop:
            case Iex_Triop:
            case Iex_Qop:
                GN_(addEvent_Compute)(&bbState, st);
                break;
            default:
                break;
            }
            break;
        case Ist_Store:
            GN_(addEvent_Memory_Store)(&bbState, st);
            break;
        case Ist_StoreG:
            GN_(addEvent_Memory_Guarded_Store)(&bbState, st);
            break;
        case Ist_LoadG:
            GN_(addEvent_Memory_Guarded_Load)(&bbState, st);
            break;
        case Ist_Dirty:
            GN_(addEvent_Dirty)(&bbState, st);
            break;
        case Ist_CAS:
            GN_(addEvent_CAS)(&bbState, st);
            break;
        case Ist_LLSC:
            GN_(addEvent_LLSC)(&bbState, st);
            break;
        case Ist_Exit:
            GN_(addEvent_Exit)(&bbState, st);

            /* gather instrumentation before any basic block exits */
            GN_(Flush) f = {GN_FLUSH_EXIT_ST, curr_instr_idx, i};
            GN_(flushEvents)(&bbState, prev_flushed_idx, f);
            prev_flushed_idx = i;
            break;
        default:
            tl_assert(0);
            break;
        } //end switch

        GN_DEBUGIF(5) {
            VG_(printf)("   pass  ");
            ppIRStmt(st);
            VG_(printf)("\n");
        }
    } //end foreach statement

    // post-BB instrumentation
    {
        GN_(Flush) f = {GN_FLUSH_BB_END, -1, -1};
        GN_(flushEvents)(&bbState, prev_flushed_idx, f);
        GN_(addEvent_BBEnd)(&bbState);
    }

    return bbState.nbb;
}

static void gn_fini(Int exitcode)
{
    GN_(termIPC)();

    finishCallstack();
}

static void gn_pre_clo_init(void)
{
    VG_(details_name)            ("Gengrind");
    VG_(details_version)         (NULL);
    VG_(details_description)     ("an event generation Valgrind tool");
    VG_(details_copyright_author)(
        "Copyright (C) 2017-2018, and GNU GPL'd, by Michael Lui.");
    VG_(details_bug_reports_to)  (VG_BUGS_TO);

    VG_(details_avg_translation_sizeB) (500);

    /* Following example set by Callgrind, to make analysis easier */
    VG_(clo_vex_control).iropt_unroll_thresh = 0;   // cannot be overriden.
    VG_(clo_vex_control).guest_chase = 0;    // cannot be overriden.

    VG_(basic_tool_funcs)        (gn_post_clo_init,
                                  gn_instrument,
                                  gn_fini);

    VG_(needs_command_line_options)(GN_(processCmdLineOption),
                                    NULL, NULL);

    /* Track when a new VG-level thread is created or destroyed
     * This is needed, for example, to generate unique-id's for each thread */
    VG_(track_pre_thread_ll_create)(GN_(preVGThreadCreate));
    VG_(track_pre_thread_ll_exit)(GN_(preVGThreadExit));

    VG_(needs_client_requests)(GN_(handleClientRequest));
    //VG_(track_start_client_code)();

    VG_(track_pre_deliver_signal)(GN_(preDeliverSignal));
    VG_(track_post_deliver_signal)(GN_(postDeliverSignal));

    GN_(setCloDefaults)();
}

VG_DETERMINE_INTERFACE_VERSION(gn_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
