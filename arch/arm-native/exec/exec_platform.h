/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

// needed to determine if this is an smp build..
#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

extern struct Hook Exec_TaskSpinLockFailHook;
extern void Exec_TaskSpinUnlock(spinlock_t *);

extern void Kernel_40_KrnSpinInit(spinlock_t *, void *);
#define EXEC_SPINLOCK_INIT(a) Kernel_40_KrnSpinInit((a), NULL)
extern spinlock_t *Kernel_43_KrnSpinLock(spinlock_t *, struct Hook *, ULONG, void *);
#define EXEC_SPINLOCK_LOCK(a,b) Kernel_43_KrnSpinLock((a), &Exec_TaskSpinLockFailHook, (b), NULL)
extern void Kernel_44_KrnSpinUnLock(spinlock_t *, void *);
#define EXEC_SPINLOCK_UNLOCK(a) Kernel_44_KrnSpinUnLock((a), NULL); \
            Exec_TaskSpinUnlock((a))

#endif

#include "tls.h"

struct Exec_PlatformData
{
    /* No platform-specific data by default */
};

#define GET_THIS_TASK           TLS_GET(ThisTask)
#if !defined(__AROSEXEC_SMP__)
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x))
#else
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x)); \
    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_WRITE); \
    AddHead(&PrivExecBase(SysBase)->TaskRunning, (struct Node *)(x)); \
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock)
#endif

#endif /* __EXEC_PLATFORM_H */
