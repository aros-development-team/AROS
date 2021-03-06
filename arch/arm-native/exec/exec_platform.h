/*
    Copyright (C) 2015-2017, The AROS Development Team. All rights reserved.
*/
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

// needed to determine if this is an smp build
#include <aros/config.h>

#define SCHEDQUANTUM_VALUE      4

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

extern struct Hook Exec_TaskSpinLockFailHook;
extern void Exec_TaskSpinUnlock(spinlock_t *);

extern void Kernel_40_KrnSpinInit(spinlock_t *, void *);
#define EXEC_SPINLOCK_INIT(a) Kernel_40_KrnSpinInit((a), NULL)
extern spinlock_t *Kernel_43_KrnSpinLock(spinlock_t *, struct Hook *, ULONG, void *);
#define EXEC_SPINLOCK_LOCK(a,b) Kernel_43_KrnSpinLock((a), NULL, (b), NULL)
#define EXECTASK_SPINLOCK_LOCK(a,b) Kernel_43_KrnSpinLock((a), &Exec_TaskSpinLockFailHook, (b), NULL)
extern void Kernel_44_KrnSpinUnLock(spinlock_t *, void *);
#define EXEC_SPINLOCK_UNLOCK(a) Kernel_44_KrnSpinUnLock((a), NULL)
#define EXECTASK_SPINLOCK_UNLOCK(a) Kernel_44_KrnSpinUnLock((a), NULL); \
            Exec_TaskSpinUnlock((a))

#endif

#include "tls.h"

struct Exec_PlatformData
{
    /* No platform-specific data by default */
};

#if defined(AROS_NO_ATOMIC_OPERATIONS)
#define IDNESTCOUNT_INC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->IDNestCnt++; \
    } while(0)
#define IDNESTCOUNT_DEC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->IDNestCnt--; \
    } while(0)
#define TDNESTCOUNT_INC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->TDNestCnt++; \
    } while(0)
#define TDNESTCOUNT_DEC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->TDNestCnt--; \
    } while(0)
#define FLAG_SCHEDQUANTUM_CLEAR \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->ScheduleFlags &= ~TLSSF_Quantum; \
    } while(0)
#define FLAG_SCHEDQUANTUM_SET \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->ScheduleFlags |= TLSSF_Quantum; \
    } while(0)
#define FLAG_SCHEDSWITCH_CLEAR \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->ScheduleFlags &= ~TLSSF_Switch; \
    } while(0)
#define FLAG_SCHEDSWITCH_SET \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->ScheduleFlags |= TLSSF_Switch; \
    } while(0)
#define FLAG_SCHEDDISPATCH_CLEAR \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->ScheduleFlags &= ~TLSSF_Dispatch; \
    } while(0)
#define FLAG_SCHEDDISPATCH_SET \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->ScheduleFlags |= TLSSF_Dispatch; \
    } while(0)
#else
#define IDNESTCOUNT_INC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_INC(__tls->IDNestCnt); \
    } while(0)
#define IDNESTCOUNT_DEC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_DEC(__tls->IDNestCnt); \
    } while(0)
#define TDNESTCOUNT_INC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_INC(__tls->TDNestCnt); \
    } while(0)
#define TDNESTCOUNT_DEC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_DEC(__tls->TDNestCnt); \
    } while(0)
#define FLAG_SCHEDQUANTUM_CLEAR \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_AND(__tls->ScheduleFlags, ~TLSSF_Quantum); \
    } while(0)
#define FLAG_SCHEDQUANTUM_SET \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_OR(__tls->ScheduleFlags, TLSSF_Quantum); \
    } while(0)
#define FLAG_SCHEDSWITCH_CLEAR \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_AND(__tls->ScheduleFlags, ~TLSSF_Switch); \
    } while(0)
#define FLAG_SCHEDSWITCH_SET \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_OR(__tls->ScheduleFlags, TLSSF_Switch); \
    } while(0)
#define FLAG_SCHEDDISPATCH_CLEAR \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_AND(__tls->ScheduleFlags, ~TLSSF_Dispatch); \
    } while(0)
#define FLAG_SCHEDDISPATCH_SET \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_OR(__tls->ScheduleFlags, TLSSF_Dispatch); \
    } while(0)
#endif
#define IDNESTCOUNT_GET \
    ({ \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        LONG __ret = (__tls->IDNestCnt); \
        __ret;  \
    })
#define IDNESTCOUNT_SET(val) \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->IDNestCnt = val; \
    } while(0)
#define TDNESTCOUNT_GET \
    ({ \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        LONG __ret = (__tls->TDNestCnt); \
        __ret;  \
    })
#define TDNESTCOUNT_SET(val) \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->TDNestCnt = val; \
    } while(0)
#define FLAG_SCHEDQUANTUM_ISSET \
    ({ \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        BOOL __ret = (__tls->ScheduleFlags & TLSSF_Quantum); \
        __ret;  \
    })
#define FLAG_SCHEDSWITCH_ISSET \
    ({ \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        BOOL __ret = (__tls->ScheduleFlags & TLSSF_Switch); \
        __ret;  \
    })
#define FLAG_SCHEDDISPATCH_ISSET \
    ({ \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        BOOL __ret = (__tls->ScheduleFlags & TLSSF_Dispatch); \
        __ret;  \
    })
#define GET_THIS_TASK           TLS_GET(ThisTask)
#define SCHEDQUANTUM_SET(val)           (SysBase->Quantum=(val))
#define SCHEDQUANTUM_GET                (SysBase->Quantum)
#define SCHEDELAPSED_SET(val)           (SysBase->Elapsed=(val))
#define SCHEDELAPSED_GET                (SysBase->Elapsed)
#if !defined(__AROSEXEC_SMP__)
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x))
#else
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x)); \
    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_WRITE); \
    AddHead(&PrivExecBase(SysBase)->TaskRunning, (struct Node *)(x)); \
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock)
#endif

#endif /* __EXEC_PLATFORM_H */
