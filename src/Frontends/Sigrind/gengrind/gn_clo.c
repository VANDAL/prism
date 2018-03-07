#include "gn_clo.h"

GN_(CommandLineOptions) GN_(clo);

void GN_(setCloDefaults)(void)
{
    GN_(clo).enable_instrumentation = True;
    GN_(clo).standalone_test        = False;
    GN_(clo).ipc_dir                = NULL;
    GN_(clo).collect_func           = NULL;
    GN_(clo).start_collect_func     = NULL;
    GN_(clo).stop_collect_func      = NULL;
    GN_(clo).gen_mem                = False;
    GN_(clo).gen_comp               = False;
    GN_(clo).gen_cf                 = False;
    GN_(clo).gen_sync               = False;
    GN_(clo).gen_instr              = False;
    GN_(clo).gen_bb                 = False;
    GN_(clo).gen_fn                 = False;
    GN_(clo).gen_thr                = False;
    GN_(clo).skip_plt               = True;
    GN_(clo).bbinfo_needed          = False;
#if GN_ENABLE_DEBUG
    GN_(clo).verbose                = 0;
#endif
}

Bool GN_(processCmdLineOption)(const HChar* arg)
{
    if      VG_STR_CLO(arg,  "--ipc-dir",    GN_(clo).ipc_dir) {}
    else if VG_STR_CLO(arg,  "--at-func",    GN_(clo).collect_func) {}
    else if VG_STR_CLO(arg,  "--start-func", GN_(clo).start_collect_func) {}
    else if VG_STR_CLO(arg,  "--stop-func",  GN_(clo).stop_collect_func) {}
    else if VG_BOOL_CLO(arg, "--gen-mem",    GN_(clo).gen_mem) {}
    else if VG_BOOL_CLO(arg, "--gen-comp",   GN_(clo).gen_comp) {}
    else if VG_BOOL_CLO(arg, "--gen-sync",   GN_(clo).gen_sync) {}
    else if VG_BOOL_CLO(arg, "--gen-instr",  GN_(clo).gen_instr) {}
    else if VG_BOOL_CLO(arg, "--gen-fn",     GN_(clo).gen_fn) {}
    else if VG_BOOL_CLO(arg, "--gen-cf",     GN_(clo).gen_cf) {}
    else if VG_BOOL_CLO(arg, "--gen-bb",     GN_(clo).gen_bb) {}
    else if VG_BOOL_CLO(arg, "--enable",     GN_(clo).enable_instrumentation) {}
    else if VG_BOOL_CLO(arg, "--test",       GN_(clo).standalone_test) {}
#if GN_ENABLE_DEBUG
    else if VG_INT_CLO(arg, "--verbose",     GN_(clo).verbose) {}
#endif

    return True;
}
