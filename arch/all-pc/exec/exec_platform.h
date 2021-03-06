/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/
#ifndef __EXEC_PLATFORM_H
#define __EXEC_PLATFORM_H

// needed to determine if this is an smp build
#include <aros/config.h>
#include <aros/atomic.h>

#ifndef __KERNEL_NOLIBBASE__
#define __KERNEL_NOLIBBASE__
#endif /* !__KERNEL_NOLIBBASE__ */
#include <proto/kernel.h>

#if (__WORDSIZE==64)
#define EXEC_REMTASK_NEEDSSWITCH
#endif
#if defined (__AROSEXEC_SMP__)
#define SCHEDQUANTUM_VALUE      10
#define SCHEDGRAN_VALUE         1
#else
#define SCHEDQUANTUM_VALUE      4
#define SCHEDGRAN_VALUE         1
#endif

#include "kernel_base.h"

#if defined(__AROSEXEC_SMP__)
#include "kernel_intern.h"
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

#include "tls.h"
#include "etask.h"

/* special flag to get the scheduling code to unspin tasks */
#define TS_UNSPIN 0x10

extern struct Hook Exec_TaskSpinLockFailHook;
extern struct Hook Exec_TaskSpinLockForbidHook;
extern struct Hook Exec_TaskSpinLockDisableHook;
extern void Exec_TaskSpinUnlock(spinlock_t *);

struct ExecSpinSCData
{
    spinlock_t *lock_ptr;
    struct Hook *lock_obtainhook;
    struct Hook *lock_failhook;
    ULONG lock_mode;
};

struct Exec_PlatformData
{
    spinlock_t *(*SpinLockCall)(spinlock_t *, struct Hook *, struct Hook *, ULONG);
};

#ifndef __KERNEL_NO_SPINLOCK_PROTOS__
extern void Kernel_49_KrnSpinInit(spinlock_t *, void *);
extern spinlock_t *Kernel_52_KrnSpinLock(spinlock_t *, struct Hook *, ULONG, void *);
extern void Kernel_53_KrnSpinUnLock(spinlock_t *, void *);
#endif

#define EXEC_SPINLOCK_INIT(a) Kernel_49_KrnSpinInit((a), NULL)
#define EXEC_SPINLOCK_LOCK(a,b,c) Kernel_52_KrnSpinLock((a), (b), (c), NULL)
#define EXEC_SPINLOCK_UNLOCK(a) Kernel_53_KrnSpinUnLock((a), NULL)

#if defined(AROS_NO_ATOMIC_OPERATIONS)
#define IDNESTCOUNT_INC \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->IDNestCnt++; \
    } while(0)
#define IDNESTCOUNT_DEC \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->IDNestCnt--; \
    } while(0)
#define TDNESTCOUNT_INC \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->TDNestCnt++; \
    } while(0)
#define TDNESTCOUNT_DEC \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->TDNestCnt--; \
    } while(0)
#define FLAG_SCHEDQUANTUM_CLEAR \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->ScheduleFlags &= ~TLSSF_Quantum; \
    } while(0)
#define FLAG_SCHEDQUANTUM_SET \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->ScheduleFlags |= TLSSF_Quantum; \
    } while(0)
#define FLAG_SCHEDSWITCH_CLEAR \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->ScheduleFlags &= ~TLSSF_Switch; \
    } while(0)
#define FLAG_SCHEDSWITCH_SET \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->ScheduleFlags |= TLSSF_Switch; \
    } while(0)
#define FLAG_SCHEDDISPATCH_CLEAR \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->ScheduleFlags &= ~TLSSF_Dispatch; \
    } while(0)
#define FLAG_SCHEDDISPATCH_SET \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->ScheduleFlags |= TLSSF_Dispatch; \
    } while(0)
#else /* !AROS_NO_ATOMIC_OPERATIONS */
#define IDNESTCOUNT_INC \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_INC_B(__schd->IDNestCnt); \
    } while(0)
#define IDNESTCOUNT_DEC \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_DEC_B(__schd->IDNestCnt); \
    } while(0)
#define TDNESTCOUNT_INC \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_INC_B(__schd->TDNestCnt); \
    } while(0)
#define TDNESTCOUNT_DEC \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_DEC_B(__schd->TDNestCnt); \
    } while(0)
#define FLAG_SCHEDQUANTUM_CLEAR \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_AND_L(__schd->ScheduleFlags, ~TLSSF_Quantum); \
    } while(0)
#define FLAG_SCHEDQUANTUM_SET \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_OR_L(__schd->ScheduleFlags, TLSSF_Quantum); \
    } while(0)
#define FLAG_SCHEDSWITCH_CLEAR \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_AND_L(__schd->ScheduleFlags, ~TLSSF_Switch); \
    } while(0)
#define FLAG_SCHEDSWITCH_SET \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_OR_L(__schd->ScheduleFlags, TLSSF_Switch); \
    } while(0)
#define FLAG_SCHEDDISPATCH_CLEAR \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_AND_L(__schd->ScheduleFlags, ~TLSSF_Dispatch); \
    } while(0)
#define FLAG_SCHEDDISPATCH_SET \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __AROS_ATOMIC_OR_L(__schd->ScheduleFlags, TLSSF_Dispatch); \
    } while(0)
#endif /* !AROS_NO_ATOMIC_OPERATIONS */
#define SCHEDQUANTUM_SET(val) \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->Quantum = val; \
    } while(0)
#define SCHEDQUANTUM_GET \
    ({ \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        UWORD __ret = 0; \
        if (__schd) \
            __ret = (__schd->Quantum); \
         __ret;  \
   })
#define SCHEDELAPSED_SET(val) \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->Elapsed = val; \
    } while(0)
#define SCHEDELAPSED_GET \
    ({ \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        UWORD __ret = 0; \
        if (__schd) \
            __ret = (__schd->Elapsed); \
         __ret;  \
   })
#define IDNESTCOUNT_GET \
    ({ \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        LONG __ret = 0; \
        if (__schd) \
            __ret = (__schd->IDNestCnt); \
         __ret;  \
   })
#define IDNESTCOUNT_SET(val) \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->IDNestCnt = val; \
    } while(0)
#define TDNESTCOUNT_GET \
    ({ \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        LONG __ret = 0; \
        if (__schd) \
            __ret = (__schd->TDNestCnt); \
        __ret;  \
    })
#define TDNESTCOUNT_SET(val) \
    do { \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
            __schd->TDNestCnt = val; \
    } while(0)
#define FLAG_SCHEDQUANTUM_ISSET \
    ({ \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        BOOL __ret = FALSE; \
        if (__schd) \
            __ret = (__schd->ScheduleFlags & TLSSF_Quantum); \
        __ret;  \
    })
#define FLAG_SCHEDSWITCH_ISSET \
    ({ \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        BOOL __ret = FALSE; \
        if (__schd) \
            __ret = (__schd->ScheduleFlags & TLSSF_Switch); \
        __ret;  \
    })
#define FLAG_SCHEDDISPATCH_ISSET \
    ({ \
        tls_t *__tls = TLS_PTR_GET(); \
        BOOL __ret = FALSE; \
        if (__schd) \
            __ret = (__schd->ScheduleFlags & TLSSF_Dispatch); \
        __ret;  \
    })
#define GET_THIS_TASK \
    ({ \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        struct Task *__ret = NULL; \
        if (__schd) \
            __ret = __schd->RunningTask; \
        __ret;  \
    })
#define SET_THIS_TASK(x)         \
    ({ \
        struct X86SchedulerPrivate  *__schd = TLS_GET(ScheduleData); \
        if (__schd) \
        { \
            EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_WRITE); \
            __schd->RunningTask = (x); \
            AddHead(&PrivExecBase(SysBase)->TaskRunning, (struct Node *)(x)); \
            EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock);  \
        } \
    })

#else /* !__AROSEXEC_SMP__ */

struct Exec_PlatformData
{
    /* No platform-specific data on plain x86 builds */
};

#ifdef AROS_NO_ATOMIC_OPERATIONS
#define IDNESTCOUNT_INC                 SysBase->IDNestCnt++
#define IDNESTCOUNT_DEC                 SysBase->IDNestCnt--
#define TDNESTCOUNT_INC                 SysBase->TDNestCnt++
#define TDNESTCOUNT_DEC                 SysBase->TDNestCnt--
#define FLAG_SCHEDQUANTUM_CLEAR         SysBase->SysFlags &= ~SFF_QuantumOver
#define FLAG_SCHEDQUANTUM_SET           SysBase->SysFlags |= SFF_QuantumOver
#define FLAG_SCHEDSWITCH_CLEAR          SysBase->AttnResched &= ~ARF_AttnSwitch
#define FLAG_SCHEDSWITCH_SET            SysBase->AttnResched |= ARF_AttnSwitch
#define FLAG_SCHEDDISPATCH_CLEAR        SysBase->AttnResched &= ~ARF_AttnDispatch
#define FLAG_SCHEDDISPATCH_SET          SysBase->AttnResched |= ARF_AttnDispatch
#else
#define IDNESTCOUNT_INC                 AROS_ATOMIC_INC(SysBase->IDNestCnt)
#define IDNESTCOUNT_DEC                 AROS_ATOMIC_DEC(SysBase->IDNestCnt)
#define TDNESTCOUNT_INC                 AROS_ATOMIC_INC(SysBase->TDNestCnt)
#define TDNESTCOUNT_DEC                 AROS_ATOMIC_DEC(SysBase->TDNestCnt)
#define FLAG_SCHEDQUANTUM_CLEAR         AROS_ATOMIC_AND(SysBase->SysFlags, ~SFF_QuantumOver)
#define FLAG_SCHEDQUANTUM_SET           AROS_ATOMIC_OR(SysBase->SysFlags, SFF_QuantumOver)
#define FLAG_SCHEDSWITCH_CLEAR          AROS_ATOMIC_AND(SysBase->AttnResched, ~ARF_AttnSwitch)
#define FLAG_SCHEDSWITCH_SET            AROS_ATOMIC_OR(SysBase->AttnResched, ARF_AttnSwitch)
#define FLAG_SCHEDDISPATCH_CLEAR        AROS_ATOMIC_AND(SysBase->AttnResched, ~ARF_AttnDispatch)
#define FLAG_SCHEDDISPATCH_SET          AROS_ATOMIC_OR(SysBase->AttnResched, ARF_AttnDispatch)
#endif
#define SCHEDQUANTUM_SET(val)           (SysBase->Quantum=(val))
#define SCHEDQUANTUM_GET                (SysBase->Quantum)
#define SCHEDELAPSED_SET(val)           (SysBase->Elapsed=(val))
#define SCHEDELAPSED_GET                (SysBase->Elapsed)
#define IDNESTCOUNT_GET                 (SysBase->IDNestCnt)
#define IDNESTCOUNT_SET(val)            (SysBase->IDNestCnt=(val))
#define TDNESTCOUNT_GET                 (SysBase->TDNestCnt)
#define TDNESTCOUNT_SET(val)            (SysBase->TDNestCnt=(val))
#define FLAG_SCHEDQUANTUM_ISSET         (SysBase->SysFlags & SFF_QuantumOver)
#define FLAG_SCHEDSWITCH_ISSET          (SysBase->AttnResched & ARF_AttnSwitch)
#define FLAG_SCHEDDISPATCH_ISSET        (SysBase->AttnResched & ARF_AttnDispatch)

#define GET_THIS_TASK                   (SysBase->ThisTask)
#define SET_THIS_TASK(x)                (SysBase->ThisTask=(x))

#endif /* !__AROSEXEC_SMP__ */

struct Task *Exec_X86CreateIdleTask(APTR);

#include "kernel_intr.h"

#include "x86_syscalls.h"

#endif /* __EXEC_PLATFORM_H */
