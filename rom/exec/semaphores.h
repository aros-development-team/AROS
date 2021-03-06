/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Private definitions of semaphore internals
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#endif

struct TraceLocation;

BOOL CheckSemaphore(struct SignalSemaphore *sigSem, struct TraceLocation *caller, struct ExecBase *SysBase);
void InternalObtainSemaphore(struct SignalSemaphore *sigSem, struct Task *owner, struct TraceLocation *caller, struct ExecBase *SysBase);
ULONG InternalAttemptSemaphore(struct SignalSemaphore *sigSem, struct Task *owner, struct TraceLocation *caller, struct ExecBase *SysBase);
