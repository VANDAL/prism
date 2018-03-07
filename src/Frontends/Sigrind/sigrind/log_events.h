#ifndef SGL_LOG_EVENTS_H
#define SGL_LOG_EVENTS_H

#include "global.h"

/********************************************************************
 * Event Logging in Sigrind
 *
 * Sigrind piggy-backs off of Callgrind's event instrumentation
 * infrastructure. All Callgrind event processing (cost tracking)
 * has been gutted, and replaced with interprocess communication
 * handling. This is how dynamic application info is sent to Sigil2.
 ********************************************************************/

void SGL_(end_logging)(void);

/* 1 Instruction */
void SGL_(log_1I0D)(InstrInfo* ii);

/* 2 Instructions */
void SGL_(log_2I0D)(InstrInfo* ii1, InstrInfo* ii2);

/* 3 Instructions */
void SGL_(log_3I0D)(InstrInfo* ii1, InstrInfo* ii2, InstrInfo* ii3);

/* 1 Instruction, 1 Data Read */
void SGL_(log_1I1Dr)(InstrInfo* ii, Addr data_addr, Word data_size);

/* 1 Data Read */
void SGL_(log_0I1Dr)(InstrInfo* ii, Addr data_addr, Word data_size);

/* 1 Instruction, 1 Data Write */
void SGL_(log_1I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size);

/* 1 Data Write */
void SGL_(log_0I1Dw)(InstrInfo* ii, Addr data_addr, Word data_size);

/* 1 Compute event */
void SGL_(log_comp_event)(InstrInfo* ii, IRType op_type, IRExprTag arity);

/* Function fn entered */
void SGL_(log_fn_entry)(fn_node* fn);

/* Function fn exited */
void SGL_(log_fn_leave)(fn_node* fn);

/* Synchronization event or thread context swap
 * Some sync events have two pieces of data,
 * e.g. mutex and condition variable in a conditional wait.
 * Otherwise only the first data argument is used */
#define UNUSED_SYNC_DATA 0
void SGL_(log_sync)(UChar type, UWord data1, UWord data2);

/* unimplemented */
void SGL_(log_global_event)(InstrInfo* ii);

/* unimplemented */
void SGL_(log_cond_branch)(InstrInfo* ii, Word taken);

/* unimplemented */
void SGL_(log_ind_branch)(InstrInfo* ii, UWord actual_dst);

#endif
