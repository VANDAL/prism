#ifndef SGL_LOG_EVENTS_H
#define SGL_LOG_EVENTS_H

#include "global.h"

/**
 * Piggy-backing off of callgrind's event infrastructure.
 * 
 * Callgrind Events are detailed in callgrind's specific
 * event header and source files. For future devs, in brief:
 *
 * TODO
 * Document this
 * Events are categorized as yada yada
 * Event Groups combine these as something because soemthing
 * Event Sets are comprised of Event Groups...I think?
 *
 */

//TODO gcc attributes were causes errors in YCM plugin, re-enable for production

//void log_global_event(InstrInfo* ii) VG_REGPARM(1);
void log_global_event(InstrInfo* ii);

//void log_cond_branch(InstrInfo* ii, Word taken) VG_REGPARM(2);
void log_cond_branch(InstrInfo* ii, Word taken);

//void log_ind_branch(InstrInfo* ii, UWord actual_dst) VG_REGPARM(2);
void log_ind_branch(InstrInfo* ii, UWord actual_dst);

/* 1 Instruction */
//void log_1I0D(InstrInfo* ii) VG_REGPARM(1);
void log_1I0D(InstrInfo* ii);

/* 2 Instructions */
//void log_2I0D(InstrInfo* ii1, InstrInfo* ii2) VG_REGPARM(2);
void log_2I0D(InstrInfo* ii1, InstrInfo* ii2);

/* 3 Instructions */
//void log_3I0D(InstrInfo* ii1, InstrInfo* ii2, InstrInfo* ii3) VG_REGPARM(3);
void log_3I0D(InstrInfo* ii1, InstrInfo* ii2, InstrInfo* ii3);

/* 1 Instruction, 1 Data Read */
//void log_1I1Dr(InstrInfo* ii, Addr data_addr, Word data_size) VG_REGPARM(3);
void log_1I1Dr(InstrInfo* ii, Addr data_addr, Word data_size);

/* 1 Data Read */
//void log_0I1Dr(InstrInfo* ii, Addr data_addr, Word data_size) VG_REGPARM(3);
void log_0I1Dr(InstrInfo* ii, Addr data_addr, Word data_size);

/* 1 Instruction, 1 Data Write */
//void log_1I1Dw(InstrInfo* ii, Addr data_addr, Word data_size) VG_REGPARM(3);
void log_1I1Dw(InstrInfo* ii, Addr data_addr, Word data_size);

/* 1 Data Write */
//void log_0I1Dw(InstrInfo* ii, Addr data_addr, Word data_size) VG_REGPARM(3);
void log_0I1Dw(InstrInfo* ii, Addr data_addr, Word data_size);

void log_comp_event(InstrInfo* ii, IRType op_type, IRExprTag arity);

/* Function fn entered */
void log_fn_entry(fn_node* fn);

/* Function fn exited */
void log_fn_leave(fn_node* fn);

void log_sync(UChar type, UWord data);

#endif
