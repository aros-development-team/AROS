/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private definitions of semaphore internals
    Lang:
*/

struct TraceLocation;

BOOL CheckSemaphore(struct SignalSemaphore *sigSem, struct TraceLocation *caller, struct ExecBase *SysBase);
void InternalObtainSemaphore(struct SignalSemaphore *sigSem, struct Task *owner, struct TraceLocation *caller, struct ExecBase *SysBase);
ULONG InternalAttemptSemaphore(struct SignalSemaphore *sigSem, struct Task *owner, struct TraceLocation *caller, struct ExecBase *SysBase);
