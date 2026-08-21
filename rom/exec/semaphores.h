/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Private definitions of semaphore internals
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#endif

/*
 * Every path that touches ss_QueueCount, ss_NestCount, ss_Owner or
 * ss_WaitQueue takes this lock, so they exclude each other across cores.
 */
#if defined(__AROSEXEC_SMP__)
#define SEM_LOCK(ss)    EXEC_SPINLOCK_LOCK(&(ss)->ss_MultipleLink.sr_SpinLock, \
                                           NULL, SPINLOCK_MODE_WRITE)
#define SEM_UNLOCK(ss)  EXEC_SPINLOCK_UNLOCK(&(ss)->ss_MultipleLink.sr_SpinLock)
#else
#define SEM_LOCK(ss)    do { } while (0)
#define SEM_UNLOCK(ss)  do { } while (0)
#endif

struct TraceLocation;

BOOL CheckSemaphore(struct SignalSemaphore *sigSem, struct TraceLocation *caller, struct ExecBase *SysBase);
void InternalObtainSemaphore(struct SignalSemaphore *sigSem, struct Task *owner, struct TraceLocation *caller, struct ExecBase *SysBase);
ULONG InternalAttemptSemaphore(struct SignalSemaphore *sigSem, struct Task *owner, struct TraceLocation *caller, struct ExecBase *SysBase);
