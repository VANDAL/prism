#ifndef GN_CRQ_H
#define GN_CRQ_H

#include "valgrind.h"
#include "gn.h"

Bool GN_(handleClientRequest)(ThreadId tid, UWord *args, UWord *ret);

/* !! ABIWARNING !! ABIWARNING !! ABIWARNING !! ABIWARNING !!
   This enum comprises an ABI exported by Valgrind to programs
   which use client requests.  DO NOT CHANGE THE ORDER OF THESE
   ENTRIES, NOR DELETE ANY -- add new ones at the end.
 */

typedef
   enum {
      VG_USERREQ__TOGGLE_COLLECT = VG_USERREQ_TOOL_BASE('G','G'),
      VG_USERREQ__START_INSTRUMENTATION,
      VG_USERREQ__STOP_INSTRUMENTATION,

      VG_USERREQ__GN_PTHREAD_CREATE_ENTER,
      VG_USERREQ__GN_PTHREAD_CREATE_LEAVE,
      VG_USERREQ__GN_PTHREAD_JOIN_ENTER,
      VG_USERREQ__GN_PTHREAD_JOIN_LEAVE,
      VG_USERREQ__GN_PTHREAD_LOCK_ENTER,
      VG_USERREQ__GN_PTHREAD_LOCK_LEAVE,
      VG_USERREQ__GN_PTHREAD_UNLOCK_ENTER,
      VG_USERREQ__GN_PTHREAD_UNLOCK_LEAVE,
      VG_USERREQ__GN_PTHREAD_BARRIER_ENTER,
      VG_USERREQ__GN_PTHREAD_BARRIER_LEAVE,
      VG_USERREQ__GN_PTHREAD_CONDWAIT_ENTER,
      VG_USERREQ__GN_PTHREAD_CONDWAIT_LEAVE,
      VG_USERREQ__GN_PTHREAD_CONDSIG_ENTER,
      VG_USERREQ__GN_PTHREAD_CONDSIG_LEAVE,
      VG_USERREQ__GN_PTHREAD_CONDBROAD_ENTER,
      VG_USERREQ__GN_PTHREAD_CONDBROAD_LEAVE,
      VG_USERREQ__GN_PTHREAD_SPINLOCK_ENTER,
      VG_USERREQ__GN_PTHREAD_SPINLOCK_LEAVE,
      VG_USERREQ__GN_PTHREAD_SPINUNLOCK_ENTER,
      VG_USERREQ__GN_PTHREAD_SPINUNLOCK_LEAVE,

      VG_USERREQ__GN_GOMP_LOCK_ENTER,
      VG_USERREQ__GN_GOMP_LOCK_LEAVE,
      VG_USERREQ__GN_GOMP_UNLOCK_ENTER,
      VG_USERREQ__GN_GOMP_UNLOCK_LEAVE,
      VG_USERREQ__GN_GOMP_BARRIER_ENTER,
      VG_USERREQ__GN_GOMP_BARRIER_LEAVE,
      VG_USERREQ__GN_GOMP_ATOMICSTART_ENTER,
      VG_USERREQ__GN_GOMP_ATOMICSTART_LEAVE,
      VG_USERREQ__GN_GOMP_ATOMICEND_ENTER,
      VG_USERREQ__GN_GOMP_ATOMICEND_LEAVE,
      VG_USERREQ__GN_GOMP_CRITSTART_ENTER,
      VG_USERREQ__GN_GOMP_CRITSTART_LEAVE,
      VG_USERREQ__GN_GOMP_CRITEND_ENTER,
      VG_USERREQ__GN_GOMP_CRITEND_LEAVE,
      VG_USERREQ__GN_GOMP_CRITNAMESTART_ENTER,
      VG_USERREQ__GN_GOMP_CRITNAMESTART_LEAVE,
      VG_USERREQ__GN_GOMP_CRITNAMEEND_ENTER,
      VG_USERREQ__GN_GOMP_CRITNAMEEND_LEAVE,
      VG_USERREQ__GN_GOMP_SETLOCK_ENTER,
      VG_USERREQ__GN_GOMP_SETLOCK_LEAVE,
      VG_USERREQ__GN_GOMP_UNSETLOCK_ENTER,
      VG_USERREQ__GN_GOMP_UNSETLOCK_LEAVE,
      VG_USERREQ__GN_GOMP_TEAMBARRIERWAIT_ENTER,
      VG_USERREQ__GN_GOMP_TEAMBARRIERWAIT_LEAVE,
      VG_USERREQ__GN_GOMP_TEAMBARRIERWAITFINAL_ENTER,
      VG_USERREQ__GN_GOMP_TEAMBARRIERWAITFINAL_LEAVE
   } Vg_GengrindClientRequest;


/* For future use */
#define CALLGRIND_TOGGLE_COLLECT                                \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__TOGGLE_COLLECT,   \
                                  0, 0, 0, 0, 0)
/* For future use */
#define CALLGRIND_START_INSTRUMENTATION                              \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__START_INSTRUMENTATION, \
                                  0, 0, 0, 0, 0)

/* For future use */
#define CALLGRIND_STOP_INSTRUMENTATION                               \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__STOP_INSTRUMENTATION,  \
                                  0, 0, 0, 0, 0)

/*---------------------------------*/
/*---  Synchronization capture  ---*/
/*---------------------------------*/
#define GN_PTHREAD_CREATE_ENTER(thr) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_CREATE_ENTER,     \
                                  thr, 0, 0, 0, 0)
#define GN_PTHREAD_CREATE_LEAVE(thr) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_CREATE_LEAVE,     \
                                  thr, 0, 0, 0, 0)


#define GN_PTHREAD_JOIN_ENTER(thr) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_JOIN_ENTER,     \
                                  thr, 0, 0, 0, 0)
#define GN_PTHREAD_JOIN_LEAVE(thr) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_JOIN_LEAVE,     \
                                  thr, 0, 0, 0, 0)


#define GN_PTHREAD_LOCK_ENTER(mut) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_LOCK_ENTER,     \
                                  mut, 0, 0, 0, 0)
#define GN_PTHREAD_LOCK_LEAVE(mut) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_LOCK_LEAVE,     \
                                  mut, 0, 0, 0, 0)


#define GN_PTHREAD_UNLOCK_ENTER(mut) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_UNLOCK_ENTER,     \
                                  mut, 0, 0, 0, 0)
#define GN_PTHREAD_UNLOCK_LEAVE(mut) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_UNLOCK_LEAVE,     \
                                  mut, 0, 0, 0, 0)


#define GN_PTHREAD_BARRIER_ENTER(bar) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_BARRIER_ENTER,     \
                                  bar, 0, 0, 0, 0)
#define GN_PTHREAD_BARRIER_LEAVE(bar) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_BARRIER_LEAVE,     \
                                  bar, 0, 0, 0, 0)


#define GN_PTHREAD_CONDWAIT_ENTER(cond, mtx) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_CONDWAIT_ENTER,     \
                                  cond, mtx, 0, 0, 0)
#define GN_PTHREAD_CONDWAIT_LEAVE(cond, mtx) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_CONDWAIT_LEAVE,     \
                                  cond, mtx, 0, 0, 0)


#define GN_PTHREAD_CONDSIG_ENTER(cond) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_CONDSIG_ENTER,     \
                                  cond, 0, 0, 0, 0)
#define GN_PTHREAD_CONDSIG_LEAVE(cond) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_CONDSIG_LEAVE,     \
                                  cond, 0, 0, 0, 0)


#define GN_PTHREAD_CONDBROAD_ENTER(cond) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_CONDBROAD_ENTER,     \
                                  cond, 0, 0, 0, 0)
#define GN_PTHREAD_CONDBROAD_LEAVE(cond) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_CONDBROAD_LEAVE,     \
                                  cond, 0, 0, 0, 0)


#define GN_PTHREAD_SPINLOCK_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_SPINLOCK_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_PTHREAD_SPINLOCK_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_SPINLOCK_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_PTHREAD_SPINUNLOCK_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_SPINUNLOCK_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_PTHREAD_SPINUNLOCK_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_PTHREAD_SPINUNLOCK_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_LOCK_ENTER(mutex) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_LOCK_ENTER,     \
                                  mutex, 0, 0, 0, 0)
#define GN_GOMP_LOCK_LEAVE(mutex) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_LOCK_LEAVE,     \
                                  mutex, 0, 0, 0, 0)


#define GN_GOMP_UNLOCK_ENTER(mutex) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_UNLOCK_ENTER,     \
                                  mutex, 0, 0, 0, 0)
#define GN_GOMP_UNLOCK_LEAVE(mutex) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_UNLOCK_LEAVE,     \
                                  mutex, 0, 0, 0, 0)


#define GN_GOMP_BARRIER_ENTER(bar) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_BARRIER_ENTER,     \
                                  bar, 0, 0, 0, 0)
#define GN_GOMP_BARRIER_LEAVE(bar) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_BARRIER_LEAVE,     \
                                  bar, 0, 0, 0, 0)


#define GN_GOMP_ATOMICSTART_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_ATOMICSTART_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_GOMP_ATOMICSTART_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_ATOMICSTART_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_ATOMICEND_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_ATOMICEND_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_GOMP_ATOMICEND_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_ATOMICEND_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_CRITSTART_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_CRITSTART_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_GOMP_CRITSTART_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_CRITSTART_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_CRITEND_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_CRITEND_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_GOMP_CRITEND_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_CRITEND_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_CRITNAMESTART_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_CRITNAMESTART_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_GOMP_CRITNAMESTART_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_CRITNAMESTART_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_CRITNAMEEND_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_CRITNAMEEND_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_GOMP_CRITNAMEEND_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_CRITNAMEEND_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_SETLOCK_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_SETLOCK_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_GOMP_SETLOCK_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_SETLOCK_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_UNSETLOCK_ENTER(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_UNSETLOCK_ENTER,     \
                                  lock, 0, 0, 0, 0)
#define GN_GOMP_UNSETLOCK_LEAVE(lock) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_UNSETLOCK_LEAVE,     \
                                  lock, 0, 0, 0, 0)


#define GN_GOMP_TEAMBARRIERWAIT_ENTER(bar) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_TEAMBARRIERWAIT_ENTER,     \
                                  bar, 0, 0, 0, 0)
#define GN_GOMP_TEAMBARRIERWAIT_LEAVE(bar) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_TEAMBARRIERWAIT_LEAVE,     \
                                  bar, 0, 0, 0, 0)


#define GN_GOMP_TEAMBARRIERWAITFINAL_ENTER(bar) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_TEAMBARRIERWAITFINAL_ENTER,     \
                                  bar, 0, 0, 0, 0)
#define GN_GOMP_TEAMBARRIERWAITFINAL_LEAVE(bar) \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__GN_GOMP_TEAMBARRIERWAITFINAL_LEAVE,     \
                                  bar, 0, 0, 0, 0)

#endif
