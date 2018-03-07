#include "gn_crq.h"
#include "gn_threads.h"
#include "gn_events.h"
#include "gn_debug.h"

#define UNUSED_SYNC_DATA 0

Bool GN_(handleClientRequest)(ThreadId tid, UWord *args, UWord *ret)
{
    if (!VG_IS_TOOL_USERREQ('G', 'G', args[0]))
        return False;

    switch(args[0]) 
    {
    case VG_USERREQ__TOGGLE_COLLECT:
    case VG_USERREQ__START_INSTRUMENTATION:
    case VG_USERREQ__STOP_INSTRUMENTATION:
        /* unimplemented */
        GN_ASSERT(False);
        *ret = 0; // meaningless
        break;

    /*******************************************
     * Synchronization API intercepts 
     *******************************************/
    case VG_USERREQ__GN_PTHREAD_CREATE_ENTER:
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_PTHREAD_CREATE_LEAVE:
        /* enable and log once the thread has been CREATED and waiting */
        GN_(resetInSyncCall)(tid);
        /* sync event generated in a separate Valgrind hook that
         * captures raw thread creation */

    case VG_USERREQ__GN_PTHREAD_JOIN_ENTER:
        /* log when the thread join is ENTERED and disable */
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_JOIN, (SyncID*)&args[1], 1);
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_PTHREAD_JOIN_LEAVE:
        GN_(resetInSyncCall)(tid);
        break;

    case VG_USERREQ__GN_GOMP_LOCK_ENTER:
    case VG_USERREQ__GN_GOMP_SETLOCK_ENTER:
    case VG_USERREQ__GN_GOMP_CRITSTART_ENTER:
    case VG_USERREQ__GN_GOMP_CRITNAMESTART_ENTER:
    case VG_USERREQ__GN_GOMP_ATOMICSTART_ENTER:
    case VG_USERREQ__GN_PTHREAD_LOCK_ENTER:
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_GOMP_SETLOCK_LEAVE:
    case VG_USERREQ__GN_GOMP_LOCK_LEAVE:
    case VG_USERREQ__GN_GOMP_CRITSTART_LEAVE:
    case VG_USERREQ__GN_GOMP_CRITNAMESTART_LEAVE:
    case VG_USERREQ__GN_GOMP_ATOMICSTART_LEAVE:
    case VG_USERREQ__GN_PTHREAD_LOCK_LEAVE:
        /* enable and log once the lock has been acquired */
        GN_(resetInSyncCall)(tid);
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_LOCK, (SyncID*)&args[1], 1);
        break;

    case VG_USERREQ__GN_GOMP_UNLOCK_ENTER:
    case VG_USERREQ__GN_GOMP_UNSETLOCK_ENTER:
    case VG_USERREQ__GN_GOMP_CRITEND_ENTER:
    case VG_USERREQ__GN_GOMP_CRITNAMEEND_ENTER:
    case VG_USERREQ__GN_GOMP_ATOMICEND_ENTER:
    case VG_USERREQ__GN_PTHREAD_UNLOCK_ENTER:
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_GOMP_UNLOCK_LEAVE:
    case VG_USERREQ__GN_GOMP_UNSETLOCK_LEAVE:
    case VG_USERREQ__GN_GOMP_CRITEND_LEAVE:
    case VG_USERREQ__GN_GOMP_CRITNAMEEND_LEAVE:
    case VG_USERREQ__GN_GOMP_ATOMICEND_LEAVE:
    case VG_USERREQ__GN_PTHREAD_UNLOCK_LEAVE:
        GN_(resetInSyncCall)(tid);
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_UNLOCK, (SyncID*)&args[1], 1);
        break;

    case VG_USERREQ__GN_GOMP_BARRIER_ENTER:
    case VG_USERREQ__GN_GOMP_TEAMBARRIERWAIT_ENTER:
    case VG_USERREQ__GN_GOMP_TEAMBARRIERWAITFINAL_ENTER:
    case VG_USERREQ__GN_PTHREAD_BARRIER_ENTER:
        /* log once the barrier is ENTERED and waiting and disable */
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_BARRIER, (SyncID*)&args[1], 1);
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_GOMP_BARRIER_LEAVE:
    case VG_USERREQ__GN_GOMP_TEAMBARRIERWAIT_LEAVE:
    case VG_USERREQ__GN_GOMP_TEAMBARRIERWAITFINAL_LEAVE:
    case VG_USERREQ__GN_PTHREAD_BARRIER_LEAVE:
        GN_(resetInSyncCall)(tid);
        break;

    case VG_USERREQ__GN_PTHREAD_CONDWAIT_ENTER:
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_PTHREAD_CONDWAIT_LEAVE:
        GN_(resetInSyncCall)(tid);
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_CONDWAIT, (SyncID*)&args[1], 2);
        break;

    case VG_USERREQ__GN_PTHREAD_CONDSIG_ENTER:
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_PTHREAD_CONDSIG_LEAVE:
        GN_(resetInSyncCall)(tid);
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_CONDSIG, (SyncID*)&args[1], 1);
        break;

    case VG_USERREQ__GN_PTHREAD_CONDBROAD_ENTER:
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_PTHREAD_CONDBROAD_LEAVE:
        GN_(resetInSyncCall)(tid);
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_CONDBROAD, (SyncID*)&args[1], 1);
        break;

    case VG_USERREQ__GN_PTHREAD_SPINLOCK_ENTER:
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_PTHREAD_SPINLOCK_LEAVE:
        GN_(resetInSyncCall)(tid);
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_SPINLOCK, (SyncID*)&args[1], 1);
        break;

    case VG_USERREQ__GN_PTHREAD_SPINUNLOCK_ENTER:
        GN_(setInSyncCall)(tid);
        break;
    case VG_USERREQ__GN_PTHREAD_SPINUNLOCK_LEAVE:
        GN_(resetInSyncCall)(tid);
        if (GN_(EventGenerationEnabled))
            GN_(flush_Sync)((UChar)SGLPRIM_SYNC_SPINUNLOCK, (SyncID*)&args[1], 1);
        break;

    default:
        return False;
    }

    return True;
}
