#include <stdio.h>
#include <pthread.h>
#include "libgomp.h"
#include "include/pub_tool_redir.h"
#include "sigrind/callgrind.h"

////////////////////////////////////////////
// PTHREAD CREATE
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZucreateZa)(pthread_t *thread,
                                                     const pthread_attr_t *attr,
                                                     void *(*start)(void *),
                                                     void *arg)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_CREATE_ENTER(*thread);
    CALL_FN_W_WWWW(ret, fn, thread, attr, start, arg);
    SIGIL_PTHREAD_CREATE_LEAVE(*thread);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZucreateZa)(pthread_t *thread,
                                                                  const pthread_attr_t *attr,
                                                                  void *(*start)(void *),
                                                                  void *arg)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_CREATE_ENTER(*thread);
    CALL_FN_W_WWWW(ret, fn, thread, attr, start, arg);
    SIGIL_PTHREAD_CREATE_LEAVE(*thread);

    return ret;
}


////////////////////////////////////////////
// PTHREAD JOIN
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZujoinZa)(pthread_t thread, void **value_pointer)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_JOIN_ENTER(thread);
    CALL_FN_W_WW(ret, fn, thread, value_pointer);
    SIGIL_PTHREAD_JOIN_LEAVE(thread);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZujoinZa)(pthread_t thread,
                                                                void **value_pointer)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_JOIN_ENTER(thread);
    CALL_FN_W_WW(ret, fn, thread, value_pointer);
    SIGIL_PTHREAD_JOIN_LEAVE(thread);

    return ret;
}


////////////////////////////////////////////
// PTHREAD MUTEX LOCK
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZumutexZulockZa)(pthread_mutex_t *mutex)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_LOCK_ENTER(mutex);
    CALL_FN_W_W(ret, fn, mutex);
    SIGIL_PTHREAD_LOCK_LEAVE(mutex);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZumutexZulockZa)(pthread_mutex_t *mutex)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_LOCK_ENTER(mutex);
    CALL_FN_W_W(ret, fn, mutex);
    SIGIL_PTHREAD_LOCK_LEAVE(mutex);

    return ret;
}


////////////////////////////////////////////
// PTHREAD MUTEX UNLOCK
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZumutexZuunlockZa)(pthread_mutex_t *mutex)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_UNLOCK_ENTER(mutex);
    CALL_FN_W_W(ret, fn, mutex);
    SIGIL_PTHREAD_UNLOCK_LEAVE(mutex);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZumutexZuunlockZa)(pthread_mutex_t *mutex)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_UNLOCK_ENTER(mutex);
    CALL_FN_W_W(ret, fn, mutex);
    SIGIL_PTHREAD_UNLOCK_LEAVE(mutex);

    return ret;
}


////////////////////////////////////////////
// PTHREAD BARRIER
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZubarrierZuwaitZa)(pthread_barrier_t *bar)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_BARRIER_ENTER(bar);
    CALL_FN_W_W(ret, fn, bar);
    SIGIL_PTHREAD_BARRIER_LEAVE(bar);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZubarrierZuwaitZa)(pthread_barrier_t *bar)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_BARRIER_ENTER(bar);
    CALL_FN_W_W(ret, fn, bar);
    SIGIL_PTHREAD_BARRIER_LEAVE(bar);

    return ret;
}


////////////////////////////////////////////
// PTHREAD CONDITIONAL WAIT
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZucondZuwaitZa)(pthread_cond_t *cond,
                                                         pthread_mutex_t *mutex)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_CONDWAIT_ENTER(cond);
    CALL_FN_W_WW(ret, fn, cond, mutex);
    SIGIL_PTHREAD_CONDWAIT_LEAVE(cond);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZucondZuwaitZa)(pthread_cond_t *cond,
                                                                      pthread_mutex_t *mutex)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_CONDWAIT_ENTER(cond);
    CALL_FN_W_WW(ret, fn, cond, mutex);
    SIGIL_PTHREAD_CONDWAIT_LEAVE(cond);

    return ret;
}


////////////////////////////////////////////
// PTHREAD CONDITIONAL SIGNAL
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZucondZusignalZa)(pthread_cond_t *cond)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_CONDSIG_ENTER(cond);
    CALL_FN_W_W(ret, fn, cond);
    SIGIL_PTHREAD_CONDSIG_LEAVE(cond);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZucondZusignalZa)(pthread_cond_t *cond)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_CONDSIG_ENTER(cond);
    CALL_FN_W_W(ret, fn, cond);
    SIGIL_PTHREAD_CONDSIG_LEAVE(cond);

    return ret;
}


////////////////////////////////////////////
// PTHREAD CONDITIONAL Broadcast
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZucondZubroadcast)(pthread_cond_t *cond)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_CONDBROAD_ENTER( cond );
    CALL_FN_W_W(ret, fn, cond);
    SIGIL_PTHREAD_CONDBROAD_LEAVE( cond );

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0,pthreadZucondZubroadcast)(pthread_cond_t *cond)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_CONDBROAD_ENTER( cond );
    CALL_FN_W_W(ret, fn, cond);
    SIGIL_PTHREAD_CONDBROAD_LEAVE( cond );

    return ret;
}


////////////////////////////////////////////
// PTHREAD SPIN LOCK
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZuspinZulockZa)(pthread_spinlock_t *lock)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_SPINLOCK_ENTER(lock);
    CALL_FN_W_W(ret, fn, lock);
    SIGIL_PTHREAD_SPINLOCK_LEAVE(lock);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZuspinZulockZa)(pthread_spinlock_t *lock)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_SPINLOCK_ENTER(lock);
    CALL_FN_W_W(ret, fn, lock);
    SIGIL_PTHREAD_SPINLOCK_LEAVE(lock);

    return ret;
}


////////////////////////////////////////////
// PTHREAD SPIN UNLOCK
////////////////////////////////////////////
int I_WRAP_SONAME_FNNAME_ZZ(NONE, pthreadZuspinZuunlockZa)(pthread_spinlock_t *lock)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_SPINUNLOCK_ENTER(lock);
    CALL_FN_W_W(ret, fn, lock);
    SIGIL_PTHREAD_SPINUNLOCK_LEAVE(lock);

    return ret;
}
int I_WRAP_SONAME_FNNAME_ZZ(libpthreadZdsoZd0, pthreadZuspinZuunlockZa)(pthread_spinlock_t *lock)
{
    int ret;
    OrigFn fn;
    VALGRIND_GET_ORIG_FN(fn);

    SIGIL_PTHREAD_SPINUNLOCK_ENTER(lock);
    CALL_FN_W_W(ret, fn, lock);
    SIGIL_PTHREAD_SPINUNLOCK_LEAVE(lock);

    return ret;
}


////////////////////////////////////////////
// GOMP_PARALLEL
// Loop kick-off calls are detected, but pthread creates are used to
// initiate threads. OMP Barriers are used to "end" loops.
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, GOMPZuparallel)(void (*fn)(void *), void *data,
                                                   unsigned num_threads, unsigned int flags)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_WWWW(func, fn, data, num_threads, flags);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZuparallel)(void (*fn)(void *), void *data,
                                                             unsigned num_threads, unsigned int flags)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_WWWW(func, fn, data, num_threads, flags);
}


////////////////////////////////////////////
// GOMP_PARALLEL_START
// Do nothing - this is action is caught during pthread creates
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, GOMPZuparallelZustart)(void (*fn)(void *), void *data,
                                                          unsigned num_threads)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_WWW(func, fn, data, num_threads);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZuparallelZustart)(void (*fn)(void *), void *data,
                                                                    unsigned num_threads)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_WWW(func, fn, data, num_threads);
}


////////////////////////////////////////////
// GOMP_PARALLEL_END
// Do nothing - this action is caught during pthread joins
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, GOMPZuparallelZuend)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_v(func);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZuparallelZuend)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_v(func);
}


////////////////////////////////////////////
// GOMP_MUTEX_LOCK
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZumutexZulock)(gomp_mutex_t *mutex)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_LOCK_ENTER(mutex);
    CALL_FN_v_W(func, mutex);
    SIGIL_GOMP_LOCK_LEAVE(mutex);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZumutexZulock)(gomp_mutex_t *mutex)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_LOCK_ENTER(mutex);
    CALL_FN_v_W(func, mutex);
    SIGIL_GOMP_LOCK_LEAVE(mutex);
}


////////////////////////////////////////////
// GOMP_MUTEX_UNLOCK
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZumutexZuunlock)(gomp_mutex_t *mutex)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_UNLOCK_ENTER(mutex);
    CALL_FN_v_W(func, mutex);
    SIGIL_GOMP_UNLOCK_LEAVE(mutex);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZumutexZuunlock)(gomp_mutex_t *mutex)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_UNLOCK_ENTER(mutex);
    CALL_FN_v_W(func, mutex);
    SIGIL_GOMP_UNLOCK_LEAVE(mutex);
}


////////////////////////////////////////////
// GOMP_BARRIER_WAIT
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZubarrierZuwait)(gomp_barrier_t *bar)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_BARRIER_ENTER(bar);
    CALL_FN_v_W(func, bar);
    SIGIL_GOMP_BARRIER_LEAVE(bar);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZubarrierZuwait)(gomp_barrier_t *bar)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_BARRIER_ENTER(bar);
    CALL_FN_v_W(func, bar);
    SIGIL_GOMP_BARRIER_LEAVE(bar);
}


////////////////////////////////////////////
// KS: With nested loops, GOMP_atomic is called.
//
// HACK FOR SYNCHROTRACE:
// KS & ML: Inside, gomp_mutex_lock is called from $LIBGOMP_LIB/config/linux/mutex.h.
// FIXME How to get the static gomp_mutex_t object for this call?
// SynchroTraceSim just needs a valid address in the address space for these locks.
static gomp_mutex_t atomic_lock;
static gomp_mutex_t default_lock;
//
// GOMP_ATOMIC_START
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, GOMPZuatomicZustart)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_ATOMICSTART_ENTER(&atomic_lock);
    CALL_FN_v_v(func);
    SIGIL_GOMP_ATOMICSTART_LEAVE(&atomic_lock);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZuatomicZustart)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_ATOMICSTART_ENTER(&atomic_lock);
    CALL_FN_v_v(func);
    SIGIL_GOMP_ATOMICSTART_LEAVE(&atomic_lock);
}


////////////////////////////////////////////
// GOMP_ATOMIC_END
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, GOMPZuatomicZuend)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_ATOMICEND_ENTER(&atomic_lock);
    CALL_FN_v_v(func);
    SIGIL_GOMP_ATOMICEND_LEAVE(&atomic_lock);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZuatomicZuend)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_ATOMICEND_ENTER(&atomic_lock);
    CALL_FN_v_v(func);
    SIGIL_GOMP_ATOMICEND_LEAVE(&atomic_lock);
}


////////////////////////////////////////////
// GOMP_CRITICAL_START
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZucriticalZustart)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_CRITSTART_ENTER(&default_lock);
    CALL_FN_v_v(func);
    SIGIL_GOMP_CRITSTART_LEAVE(&default_lock);
}


////////////////////////////////////////////
// GOMP_CRITICAL_END
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZucriticalZuend)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_CRITEND_ENTER(&default_lock);
    CALL_FN_v_v(func);
    SIGIL_GOMP_CRITEND_LEAVE(&default_lock);
}


////////////////////////////////////////////
// GOMP_CRITICAL_NAME_START
// FIXME what is 'plock' and why is it being passed to the original function?
// Originally added by KS
// KS & ML: keep until further investigation
////////////////////////////////////////////
gomp_mutex_t *plock;

void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZucriticalZunameZustart)(void **pptr)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_CRITNAMESTART_ENTER(plock);
    CALL_FN_v_W(func, plock);
    SIGIL_GOMP_CRITNAMESTART_LEAVE(plock);
}


////////////////////////////////////////////
// GOMP_CRITICAL_NAME_END
// FIXME what is 'plock' and why is it being passed to the original function?
// KS & ML: keep until further investigation
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, GOMPZucriticalZunameZuend)(void **pptr)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_CRITNAMEEND_ENTER(plock);
    CALL_FN_v_W(func, plock);
    SIGIL_GOMP_CRITNAMEEND_LEAVE(plock);
}


////////////////////////////////////////////
// OMP_SET_LOCK
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, ompZusetZulock)(omp_lock_t *lock)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_SETLOCK_ENTER(lock);
    CALL_FN_v_W(func, lock);
    SIGIL_GOMP_SETLOCK_LEAVE(lock);
}


////////////////////////////////////////////
// OMP_UNSET_LOCK
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, ompZuunsetZulock)(omp_lock_t *lock)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_UNSETLOCK_ENTER(lock);
    CALL_FN_v_W(func, lock);
    SIGIL_GOMP_UNSETLOCK_LEAVE(lock);
}


////////////////////////////////////////////
// XXX TESTING THREAD TEAM START
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZuteamZustart)(void (*fn)(void *), void *data,
                                                      unsigned nthreads, unsigned flags, struct gomp_team *team)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_5W(func, fn, data, nthreads, flags, team);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZuteamZustart)(void (*fn)(void *), void *data,
                                                                unsigned nthreads, unsigned flags, struct gomp_team *team)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_5W(func, fn, data, nthreads, flags, team);
}


////////////////////////////////////////////
// XXX TESTING THREAD TEAM END
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZuteamZuend)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_v(func);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZuteamZuend)()
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_v(func);
}


////////////////////////////////////////////
// XXX TESTING THREAD START
////////////////////////////////////////////
void *I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZuthreadZustart)(void *xdata)
{
    void *ret;
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_W_W(ret, func, xdata);
    return ret;
}
void *I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZuthreadZustart)(void *xdata)
{
    void *ret;
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_W_W(ret, func, xdata);
    return ret;
}


////////////////////////////////////////////
// XXX TESTING FREE THREAD
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZufreeZuthread)(void *arg __attribute__((unused)))
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_W(func, arg);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZufreeZuthread)(void *arg __attribute__((unused)))
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_W(func, arg);
}


////////////////////////////////////////////
// XXX GOMP_TEAM_BARRIER_WAIT - NOT FOUND
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZuteamZubarrierZuwait)(gomp_barrier_t *bar)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_TEAMBARRIERWAIT_ENTER(bar);
    CALL_FN_v_W(func, bar);
    SIGIL_GOMP_TEAMBARRIERWAIT_LEAVE(bar);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZuteamZubarrierZuwait)(gomp_barrier_t *bar)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_TEAMBARRIERWAIT_ENTER(bar);
    CALL_FN_v_W(func, bar);
    SIGIL_GOMP_TEAMBARRIERWAIT_LEAVE(bar);
}


////////////////////////////////////////////
// GOMP_BARRIER_DESTROY
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZubarrierZudestroy)(gomp_barrier_t *bar)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_W(func, bar);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZubarrierZudestroy)(gomp_barrier_t *bar)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_W(func, bar);
}


////////////////////////////////////////////
// GOMP_TEAM_BARRIER_WAIT_FINAL
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZuteamZubarrierZuwaitZufinal)(gomp_barrier_t *bar)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_TEAMBARRIERWAITFINAL_ENTER(bar);
    CALL_FN_v_W(func, bar);
    SIGIL_GOMP_TEAMBARRIERWAITFINAL_LEAVE(bar);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZuteamZubarrierZuwaitZufinal)(gomp_barrier_t *bar)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);

    SIGIL_GOMP_TEAMBARRIERWAITFINAL_ENTER(bar);
    CALL_FN_v_W(func, bar);
    SIGIL_GOMP_TEAMBARRIERWAITFINAL_LEAVE(bar);
}


////////////////////////////////////////////
// GOMP_BARRIER_WAIT_END
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, gompZubarrierZuwaitZuend)(gomp_barrier_t *bar,
                                                             gomp_barrier_state_t state)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_WW(func, bar, state);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, gompZubarrierZuwaitZuend)(gomp_barrier_t *bar,
                                                                       gomp_barrier_state_t state)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_WW(func, bar, state);
}


////////////////////////////////////////////
// FREE_TEAM
////////////////////////////////////////////
void I_WRAP_SONAME_FNNAME_ZZ(NONE, freeZuteam)(struct gomp_team *team)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_W(func, team);
}
void I_WRAP_SONAME_FNNAME_ZZ(libgompZdsoZd1, freeZuteam)(struct gomp_team *team)
{
    OrigFn func;
    VALGRIND_GET_ORIG_FN(func);
    CALL_FN_v_W(func, team);
}
