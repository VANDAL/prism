
//-------------------------------------------------------------------------------------------------
/** Using Helgrind as an example **/

#include "pub_tool_basics.h"
#include "pub_tool_redir.h"
#include "pub_tool_clreq.h"
#include "gn_sync.h"

#include <pthread.h>
#include <stdio.h>

#if defined(VGO_linux)
#define PTH_FUNC(ret_ty, f, args...) \
   ret_ty I_WRAP_SONAME_FNNAME_ZZ(VG_Z_LIBPTHREAD_SONAME,f)(args); \
   ret_ty I_WRAP_SONAME_FNNAME_ZZ(VG_Z_LIBPTHREAD_SONAME,f)(args)
#endif


#define TRACE_PTH_FNS 0

//-------------------------------------------------------------------------------------------------
/** pthread_create **/
/* ensure this has its own frame, so as to make it more distinguishable
   in suppressions */
    __attribute__((noinline))
static int pthread_create_WRK(pthread_t *thread, const pthread_attr_t *attr,
                              void *(*start) (void *), void *arg)
{
    int    ret;
    OrigFn fn;

    VALGRIND_GET_ORIG_FN(fn);

    GN_DISABLE_EVENTS();
    VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTH_CREATE_PRE,
                                    0, 0, 0, 0, 0);
    CALL_FN_W_WWWW(ret, fn, thread, attr, start, arg);
    VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTH_CREATE_POST,
                                    0, 0, 0, 0, 0);
    GN_ENABLE_EVENTS();

    if (TRACE_PTH_FNS) {
        fprintf(stderr, " :: pth_create -> %d >>\n", ret);
    }
    return ret;
}
#if defined(VGO_linux)
   PTH_FUNC(int, pthreadZucreateZAZa, // pthread_create@*
                 pthread_t *thread, const pthread_attr_t *attr,
                 void *(*start) (void *), void *arg) {
      return pthread_create_WRK(thread, attr, start, arg);
   }
#endif

//-------------------------------------------------------------------
/** pthread_join **/
