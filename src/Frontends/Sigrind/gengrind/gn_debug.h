#ifndef GN_DEBUG_H
#define GN_DEBUG_H

#include "gn.h"
#include "gn_clo.h"

void GN_(printTabs)(UInt tabs);
void GN_(printBBno)(void);
void GN_(sprint_SyncType)(HChar *str, SyncType type);

//-------------------------------------------------------------------------------------------------
/** Debug macros from Callgrind **/

#if GN_ENABLE_DEBUG

#define GN_DEBUGIF(x) \
  if (UNLIKELY( (GN_(clo).verbose > x)))

#define GN_DEBUG(x,format,args...)   \
    GN_DEBUGIF(x) {                  \
      GN_(printBBno)();	      \
      VG_(printf)(format,##args);     \
    }

#define GN_ASSERT(cond)              \
    if (UNLIKELY(!(cond))) {          \
      GN_(printBBno)();	      \
      tl_assert(cond);                \
     }

#else
#define GN_DEBUGIF(x) if (0)
#define GN_DEBUG(x...) {}
#define GN_ASSERT(cond) tl_assert(cond);
#endif



#endif
