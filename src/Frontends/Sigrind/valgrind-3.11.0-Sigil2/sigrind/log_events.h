#ifndef SGL_LOG_EVENTS_H
#define SGL_LOG_EVENTS_H

#include "global.h"
#include "coregrind/pub_core_libcprint.h"

/********************************************************************
 * Event Logging in Sigrind
 *
 * Sigrind piggy-backs off of Callgrind's event instrumentation
 * infrastructure. All Callgrind event processing (cost tracking)
 * has been gutted, and replaced with interprocess communication
 * handling. This is how dynamic application info is sent to Sigil2.
 ********************************************************************/

/*
 * Initializes interprocess communication with Sigil2.
 * THIS MUST BE RUN BEFORE ANY LOGGING FUNCTIONS ARE INVOKED 
 */
void SGL_(init_IPC)(void);
void SGL_(finish_IPC)(void);

/* 1 Instruction */
void SGL_(log_1I0D)(InstrInfo* ii) VG_REGPARM(1);

/* 2 Instructions */
void SGL_(log_2I0D)(InstrInfo* ii1, InstrInfo* ii2) VG_REGPARM(2);

/* 3 Instructions */
void SGL_(log_3I0D)(InstrInfo* ii1, InstrInfo* ii2, InstrInfo* ii3) VG_REGPARM(3);

/* 1 Instruction, 1 Data Read */
void SGL_(log_1I1Dr)(InstrInfo* ii, Addr data_addr, Word data_size) VG_REGPARM(3);

/* 1 Data Read */
void SGL_(log_0I1Dr)(InstrInfo* ii, Addr data_addr, Word data_size) VG_REGPARM(3);

/* 1 Instruction, 1 Data Write */
void SGL_(log_1I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size) VG_REGPARM(3);

/* 1 Data Write */
void SGL_(log_0I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size) VG_REGPARM(3);

/* 1 Compute event */
void SGL_(log_comp_event)(InstrInfo* ii, IRType op_type, IRExprTag arity);

/* Function fn entered */
void SGL_(log_fn_entry)(fn_node* fn);

/* Function fn exited */
void SGL_(log_fn_leave)(fn_node* fn);

/* Synchronization event or thread context swap */
void SGL_(log_sync)(UChar type, UWord data);

/* unimplemented */
void SGL_(log_global_event)(InstrInfo* ii) VG_REGPARM(1);

/* unimplemented */
void SGL_(log_cond_branch)(InstrInfo* ii, Word taken) VG_REGPARM(2);

/* unimplemented */
void SGL_(log_ind_branch)(InstrInfo* ii, UWord actual_dst) VG_REGPARM(2);

#endif
