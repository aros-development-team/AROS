/*
 * AmigaOS thread syncronization primitives.
 */

#include "dbus_intern.h"

#include <exec/memory.h>
#include <exec/semaphores.h>

#include <clib/alib_protos.h>
#include <proto/dbus.h>
#include <proto/exec.h>

#define DEBUG 1
#include <aros/debug.h>

extern struct ExecBase* DBUS_SysBase;
#define SysBase DBUS_SysBase

struct CondVar {
    struct MinList Waiters;
};

static DBusMutex* _mutex_new(void);
static void _mutex_free(DBusMutex*);
static dbus_bool_t _mutex_lock(DBusMutex*);
static dbus_bool_t _mutex_unlock(DBusMutex*);

static DBusCondVar* _condvar_new(void);
static void _condvar_free(DBusCondVar*);
static void _condvar_wait(DBusCondVar*, DBusMutex*);
static dbus_bool_t _condvar_wait_timeout (DBusCondVar*, DBusMutex*, int);
static void _condvar_wake_one (DBusCondVar*);
static void _condvar_wake_all (DBusCondVar*);

static const DBusThreadFunctions amiga_functions = {
  (DBUS_THREAD_FUNCTIONS_MUTEX_NEW_MASK |
   DBUS_THREAD_FUNCTIONS_MUTEX_FREE_MASK |
   DBUS_THREAD_FUNCTIONS_MUTEX_LOCK_MASK |
   DBUS_THREAD_FUNCTIONS_MUTEX_UNLOCK_MASK |
   DBUS_THREAD_FUNCTIONS_CONDVAR_NEW_MASK |
   DBUS_THREAD_FUNCTIONS_CONDVAR_FREE_MASK |
   DBUS_THREAD_FUNCTIONS_CONDVAR_WAIT_MASK |
   DBUS_THREAD_FUNCTIONS_CONDVAR_WAIT_TIMEOUT_MASK |
   DBUS_THREAD_FUNCTIONS_CONDVAR_WAKE_ONE_MASK |
   DBUS_THREAD_FUNCTIONS_CONDVAR_WAKE_ALL_MASK),
  _mutex_new,
  _mutex_free,
  _mutex_lock,
  _mutex_unlock,
  _condvar_new,
  _condvar_free,
  _condvar_wait,
  _condvar_wait_timeout,
  _condvar_wake_one,
  _condvar_wake_all,
};

void InitThreads(struct DBUSBase* DBUSBase) {
  dbus_threads_init(&amiga_functions);
}

static DBusMutex* _mutex_new(void) {
  struct SignalSemaphore* mutex = AllocVec(sizeof(struct SignalSemaphore),
					   MEMF_ANY|MEMF_CLEAR);

  if (mutex != NULL) {
    mutex->ss_Link.ln_Type=NT_SIGNALSEM;
    InitSemaphore(mutex);
  }

//  kprintf("_mutex_new returns %08lx\n", mutex);
  return (DBusMutex*) mutex;
}

static void _mutex_free(DBusMutex* m) {
  struct SignalSemaphore* mutex = (struct SignalSemaphore*) m;

//  kprintf("_mutex_free %08lx\n", mutex);
  FreeVec(mutex);
}

static dbus_bool_t _mutex_lock(DBusMutex* m) {
  struct SignalSemaphore* mutex = (struct SignalSemaphore*) m;

//  kprintf("_mutex_lock %08lx\n", mutex);
  ObtainSemaphore(mutex);
  return TRUE;
}

static dbus_bool_t _mutex_unlock(DBusMutex* m) {
  struct SignalSemaphore* mutex = (struct SignalSemaphore*) m;

//  kprintf("_mutex_unlock %08lx\n", mutex);
  ReleaseSemaphore(mutex);
  return TRUE;
}


static DBusCondVar* _condvar_new(void) {
  struct CondVar* cond = AllocVec(sizeof(struct CondVar),
				  MEMF_ANY|MEMF_CLEAR);

  if (cond != NULL) {
    NewList((struct List*) &cond->Waiters);
  }

  return (DBusCondVar*) cond;
}

static void _condvar_free(DBusCondVar* c) {
  struct CondVar* cond = (struct CondVar*) c;

  FreeVec(cond);
}


static void _condvar_wait(DBusCondVar* c, DBusMutex* m) {
  struct CondVar* cond = (struct CondVar*) c;

  struct SemaphoreRequest sr = {
    { NULL, NULL },
    FindTask(NULL)
  };
  
  AddTail((struct List*) &cond->Waiters, (struct Node*) &sr);

  Forbid();
  _mutex_unlock(m);
  SetSignal(0, SIGF_SINGLE);
  Permit();

  Wait(SIGF_SINGLE);

  _mutex_lock(m);
  Remove((struct Node*) &sr);
}

static dbus_bool_t _condvar_wait_timeout(DBusCondVar* c, DBusMutex* m, int msec) {
  /* TODO */
  _condvar_wait(c, m);

  return TRUE;
}

static void _condvar_wake_one (DBusCondVar* c) {
  struct CondVar* cond = (struct CondVar*) c;

  /* Assume the mutex is locked */
  if (!IsListEmpty((struct List*) &cond->Waiters)) {
    Signal(((struct SemaphoreRequest*) cond->Waiters.mlh_Head)->sr_Waiter, SIGF_SINGLE);
  }
}

static void _condvar_wake_all (DBusCondVar* c) {
  struct CondVar* cond = (struct CondVar*) c;
  struct SemaphoreRequest* sr;

  /* Assume the mutex is locked */
  for (sr = (struct SemaphoreRequest*) cond->Waiters.mlh_Head;
       sr->sr_Link.mln_Succ != NULL;
       sr = (struct SemaphoreRequest*) sr->sr_Link.mln_Succ) {
    Signal(sr->sr_Waiter, SIGF_SINGLE);
  }
}
