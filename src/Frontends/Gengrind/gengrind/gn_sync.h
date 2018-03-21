#ifndef GN_SYNC_INTERCEPTS_H
#define GN_SYNC_INTERCEPTS_H

#include "valgrind.h"

typedef enum _Vg_GnTClientRequest Vg_GnTClientRequest;
enum _Vg_GnTClientRequest {
    VG_USERREQ__GN_INIT = VG_USERREQ_TOOL_BASE('G', 'N'),
    VG_USERREQ__GN_DISABLE_EVENTS,
    VG_USERREQ__GN_ENABLE_EVENTS,

    //---------------------------------------------------------------------------------------------
    /** pthread client requests **/
    VG_USERREQ__GN_PTH_CREATE_PRE,
    VG_USERREQ__GN_PTH_CREATE_POST,
    VG_USERREQ__GN_PTH_JOIN_PRE,
    VG_USERREQ__GN_PTH_JOIN_POST,

    VG_USERREQ__GN_PTH_MUTEX_LOCK_PRE,
    VG_USERREQ__GN_PTH_MUTEX_LOCK_POST,

    VG_USERREQ__GN_PTH_MUTEX_UNLOCK_PRE,
    VG_USERREQ__GN_PTH_MUTEX_UNLOCK_POST,

    VG_USERREQ__GN_PTH_BARRIER_WAIT_PRE,
    VG_USERREQ__GN_PTH_BARRIER_WAIT_POST,

    VG_USERREQ__GN_PTH_COND_WAIT_PRE,
    VG_USERREQ__GN_PTH_COND_WAIT_POST,

    VG_USERREQ__GN_PTH_COND_SIGNAL_PRE,
    VG_USERREQ__GN_PTH_COND_SIGNAL_POST,

    VG_USERREQ__GN_PTH_COND_BROADCAST_PRE,
    VG_USERREQ__GN_PTH_COND_BROADCAST_POST,

    VG_USERREQ__GN_PTH_SPIN_LOCK_PRE,
    VG_USERREQ__GN_PTH_SPIN_LOCK_POST,

    VG_USERREQ__GN_PTH_SPIN_UNLOCK_PRE,
    VG_USERREQ__GN_PTH_SPIN_UNLOCK_POST,
};

/* Disable event generation.
 * This tool is only interested in generating events specific to a workload.
 * When inside specific functions, such as pthread API calls,
 * compute and memory events are specific to a synchronization implementation,
 * and not the respective workload, so disable events until the function
 * returns.*/
#define GN_DISABLE_EVENTS()                                        \
    VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_DISABLE_EVENTS, \
                                    0, 0, 0, 0, 0)
#define GN_ENABLE_EVENTS()                                         \
    VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_ENABLE_EVENTS,  \
                                    0, 0, 0, 0, 0)

#endif
