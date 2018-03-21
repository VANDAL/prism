#include "gn_debug.h"
#include "gn_bb.h"

static ULong bbWritten = 0;

void GN_(printBBno)(void)
{
    if (bbWritten != GN_(bbExecutions)) {
        /* only write BB once, if multiple debug calls are made */
        bbWritten = GN_(bbExecutions);
        VG_(printf)("BB# %llu\n", GN_(bbExecutions));
    }
}


void GN_(printTabs)(UInt tabs)
{
    for (UInt i=0; i<tabs; ++i)
        VG_(printf)("\t");
}


void GN_(printBBInfo)(BBInfo *bb)
{
    VG_(printf)("BBInfo %p\n", bb);
    GN_(printTabs)(1);
    VG_(printf)("Total Events: %d\n", bb->eventsTotal);
}


void GN_(sprint_SyncType)(HChar *str, SyncType type)
{
    switch(type) {
    case PRISM_SYNC_UNDEF:
        VG_(sprintf)(str, "Undefined");
        break;
    case PRISM_SYNC_CREATE:
        VG_(sprintf)(str, "Spawn");
        break;
    case PRISM_SYNC_JOIN:
        VG_(sprintf)(str, "Join");
        break;
    case PRISM_SYNC_BARRIER:
        VG_(sprintf)(str, "BarrieR");
        break;
    case PRISM_SYNC_SYNC:
        VG_(sprintf)(str, "Sync (generalized)");
        break;
    case PRISM_SYNC_SWAP:
        VG_(sprintf)(str, "Swap");
        break;
    case PRISM_SYNC_LOCK:
        VG_(sprintf)(str, "Lock");
        break;
    case PRISM_SYNC_UNLOCK:
        VG_(sprintf)(str, "Unlock");
        break;
    case PRISM_SYNC_CONDWAIT:
        VG_(sprintf)(str, "Conditional Wait");
        break;
    case PRISM_SYNC_CONDSIG:
        VG_(sprintf)(str, "Conditional Signal");
        break;
    case PRISM_SYNC_CONDBROAD:
        VG_(sprintf)(str, "Conditional Broadcast");
        break;
    case PRISM_SYNC_SPINLOCK:
        VG_(sprintf)(str, "SpinLock");
        break;
    case PRISM_SYNC_SPINUNLOCK:
        VG_(sprintf)(str, "SpinUnlock");
        break;
    default:
        VG_(sprintf)(str, "INVALID");
        tl_assert(0);
        break;
    }
}
