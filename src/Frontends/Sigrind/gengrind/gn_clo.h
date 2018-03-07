#ifndef GN_CLO_H
#define GN_CLO_H

#include "gn.h"

typedef struct {
  const HChar* ipc_dir;
  const HChar* collect_func;
  const HChar* start_collect_func;
  const HChar* stop_collect_func;
  Bool enable_instrumentation;
  Bool standalone_test;
  Bool gen_mem;
  Bool gen_comp;
  Bool gen_cf;
  Bool gen_sync;
  Bool gen_instr;
  Bool gen_bb;
  Bool gen_fn;
  Bool gen_thr;

  Bool skip_plt;

  Bool bbinfo_needed;

#if GN_ENABLE_DEBUG
  Int verbose;
#endif

} GN_(CommandLineOptions);

extern GN_(CommandLineOptions) GN_(clo);

//-------------------------------------------------------------------------------------------------
void GN_(setCloDefaults)(void);
Bool GN_(processCmdLineOption)(const HChar* arg);

#endif
