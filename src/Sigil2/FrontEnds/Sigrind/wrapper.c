#include <stdio.h>
#include <pthread.h>
#include "include/pub_tool_redir.h"
//#include "valgrind.h"
#include "callgrind.h"

//WRAPPER FOR STATIC LIBRARIES

#define FN_ENTER 1
#define FN_LEAVE 0

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZucreateZa)(pthread_t *thread, const pthread_attr_t *attr, void *(*start) (void *), void *arg) {
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);

  SIGIL_PTHREAD_CREATE_ENTER( *thread );
  CALL_FN_W_WWWW(ret, fn, thread, attr, start, arg);
  SIGIL_PTHREAD_CREATE_LEAVE( *thread );

  return ret;
}

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZujoinZa)(pthread_t thread, void** value_pointer) {
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);

  SIGIL_PTHREAD_JOIN_ENTER(thread);
  CALL_FN_W_WW( ret, fn, thread, value_pointer );
  SIGIL_PTHREAD_JOIN_LEAVE(thread);

  return ret;
}

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZumutexZulockZa)(pthread_mutex_t *mutex){
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);

  SIGIL_PTHREAD_LOCK_ENTER( mutex );
  CALL_FN_W_W( ret, fn, mutex );
  SIGIL_PTHREAD_LOCK_LEAVE( mutex );

  return ret;
}

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZumutexZuunlockZa)(pthread_mutex_t *mutex){
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);

  SIGIL_PTHREAD_UNLOCK_ENTER( mutex );
  CALL_FN_W_W( ret, fn, mutex );
  SIGIL_PTHREAD_UNLOCK_LEAVE( mutex );

  return ret;
}

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZubarrierZuwaitZa)(pthread_barrier_t* bar){
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);

  SIGIL_PTHREAD_BARRIER_ENTER( bar );
  CALL_FN_W_W( ret, fn, bar );
  SIGIL_PTHREAD_BARRIER_LEAVE( bar );

  return ret;
}

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZucondZuwaitZa)(pthread_cond_t* cond, pthread_mutex_t* mutex){
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);

  SIGIL_PTHREAD_CONDWAIT_ENTER( cond );
  CALL_FN_W_WW(ret, fn, cond, mutex);
  SIGIL_PTHREAD_CONDWAIT_LEAVE( cond );

  return ret;
}

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZucondZusignalZa)(pthread_cond_t* cond){
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);

  SIGIL_PTHREAD_CONDSIG_ENTER( cond );
  CALL_FN_W_W(ret, fn, cond);
  SIGIL_PTHREAD_CONDSIG_LEAVE( cond );

  return ret;
}

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZuspinZulockZa)(pthread_spinlock_t* lock){
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);
  
  SIGIL_PTHREAD_SPINLOCK_ENTER( lock );
  CALL_FN_W_W(ret, fn, lock);
  SIGIL_PTHREAD_SPINLOCK_LEAVE( lock );

  return ret;
}

int I_WRAP_SONAME_FNNAME_ZZ(NONE,pthreadZuspinZuunlockZa)(pthread_spinlock_t* lock){
  int ret;
  OrigFn fn;
  VALGRIND_GET_ORIG_FN(fn);
  
  SIGIL_PTHREAD_SPINUNLOCK_ENTER( lock );
  CALL_FN_W_W(ret, fn, lock);
  SIGIL_PTHREAD_SPINUNLOCK_LEAVE( lock );

  return ret;
}
