/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.
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

/*
 * Store-store barrier for publishing a freshly built structure to readers
 * that walk it without taking a lock. x86 does not reorder stores, so
 * keeping the compiler from doing it is enough.
 */
#define EXEC_MEMORY_BARRIER()   asm volatile("" ::: "memory")

/*
 * Every one of these is a single %gs-relative access to this CPU's own
 * scheduler block (see arch/x86_64-pc/kernel/tls.h). They used to load
 * the block's address out of the TLS and then touch a field through it,
 * which a reschedule could split: a task moved to another CPU between the
 * two accesses read, or wrote, the state of the CPU it had left. One
 * instruction cannot be split by an interrupt, so the CPU that retires it
 * is the CPU the caller is on.
 *
 * The lock prefix is kept only where another CPU writes the same field.
 * The schedule flags are set for a CPU by whichever one runs the
 * heartbeat or the VBlank server; the two nest counts are touched by
 * their own CPU alone, so there the single instruction is enough.
 */
#define IDNESTCOUNT_INC                 TLS_INC_B(Sched.IDNestCnt)
#define IDNESTCOUNT_DEC                 TLS_DEC_B(Sched.IDNestCnt)
#define TDNESTCOUNT_INC                 TLS_INC_B(Sched.TDNestCnt)
#define TDNESTCOUNT_DEC                 TLS_DEC_B(Sched.TDNestCnt)
#define FLAG_SCHEDQUANTUM_CLEAR         TLS_LOCK_AND_L(Sched.ScheduleFlags, ~TLSSF_Quantum)
#define FLAG_SCHEDQUANTUM_SET           TLS_LOCK_OR_L(Sched.ScheduleFlags, TLSSF_Quantum)
#define FLAG_SCHEDSWITCH_CLEAR          TLS_LOCK_AND_L(Sched.ScheduleFlags, ~TLSSF_Switch)
#define FLAG_SCHEDSWITCH_SET            TLS_LOCK_OR_L(Sched.ScheduleFlags, TLSSF_Switch)
#define FLAG_SCHEDDISPATCH_CLEAR        TLS_LOCK_AND_L(Sched.ScheduleFlags, ~TLSSF_Dispatch)
#define FLAG_SCHEDDISPATCH_SET          TLS_LOCK_OR_L(Sched.ScheduleFlags, TLSSF_Dispatch)

/*
 * Block IRQ-exit dispatch without going through Forbid()/Permit() (Permit's
 * decrement can trigger Reschedule() which yields - exactly what we don't
 * want while holding a scheduler list spinlock). Used by the scheduler-list
 * helpers in rom/exec/exec_intern.h. Maps to the per-CPU task-dispatch nest
 * count, mirroring arm-native's EXEC_BLOCK_DISPATCH_INC/DEC.
 */
#define EXEC_BLOCK_DISPATCH_INC         TDNESTCOUNT_INC
#define EXEC_BLOCK_DISPATCH_DEC         TDNESTCOUNT_DEC

#define SCHEDQUANTUM_SET(val)           TLS_SET(Sched.Quantum, (UWORD)(val))
#define SCHEDQUANTUM_GET                TLS_GET(Sched.Quantum)
#define SCHEDELAPSED_SET(val)           TLS_SET(Sched.Elapsed, (UWORD)(val))
#define SCHEDELAPSED_GET                TLS_GET(Sched.Elapsed)
#define IDNESTCOUNT_SET(val)            TLS_SET(Sched.IDNestCnt, (BYTE)(val))
#define IDNESTCOUNT_GET                 ((LONG)TLS_GET(Sched.IDNestCnt))
#define TDNESTCOUNT_SET(val)            TLS_SET(Sched.TDNestCnt, (BYTE)(val))
#define TDNESTCOUNT_GET                 ((LONG)TLS_GET(Sched.TDNestCnt))
#define FLAG_SCHEDQUANTUM_ISSET         ((BOOL)((TLS_GET(Sched.ScheduleFlags) & TLSSF_Quantum) != 0))
#define FLAG_SCHEDSWITCH_ISSET          ((BOOL)((TLS_GET(Sched.ScheduleFlags) & TLSSF_Switch) != 0))
#define FLAG_SCHEDDISPATCH_ISSET        ((BOOL)((TLS_GET(Sched.ScheduleFlags) & TLSSF_Dispatch) != 0))
#define GET_THIS_TASK                   TLS_GET(Sched.RunningTask)
#define SET_THIS_TASK(x)         \
    ({ \
        struct Task *__newtask = (x); \
        EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_WRITE); \
        TLS_SET(Sched.RunningTask, __newtask); \
        AddHead(&PrivExecBase(SysBase)->TaskRunning, (struct Node *)__newtask); \
        EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock);  \
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
#define FLAG_SCHEDSWITCH_CLEAR          SysBase->SysFlags &= ~SFF_AttnSwitch
#define FLAG_SCHEDSWITCH_SET            SysBase->SysFlags |= SFF_AttnSwitch
#define FLAG_SCHEDDISPATCH_CLEAR        SysBase->AttnResched &= ~ARF_AttnDispatch
#define FLAG_SCHEDDISPATCH_SET          SysBase->AttnResched |= ARF_AttnDispatch
#else
#define IDNESTCOUNT_INC                 AROS_ATOMIC_INC(SysBase->IDNestCnt)
#define IDNESTCOUNT_DEC                 AROS_ATOMIC_DEC(SysBase->IDNestCnt)
#define TDNESTCOUNT_INC                 AROS_ATOMIC_INC(SysBase->TDNestCnt)
#define TDNESTCOUNT_DEC                 AROS_ATOMIC_DEC(SysBase->TDNestCnt)
#define FLAG_SCHEDQUANTUM_CLEAR         AROS_ATOMIC_AND(SysBase->SysFlags, ~SFF_QuantumOver)
#define FLAG_SCHEDQUANTUM_SET           AROS_ATOMIC_OR(SysBase->SysFlags, SFF_QuantumOver)
#define FLAG_SCHEDSWITCH_CLEAR          AROS_ATOMIC_AND(SysBase->SysFlags, ~SFF_AttnSwitch)
#define FLAG_SCHEDSWITCH_SET            AROS_ATOMIC_OR(SysBase->SysFlags, SFF_AttnSwitch)
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
#define FLAG_SCHEDSWITCH_ISSET          (SysBase->SysFlags & SFF_AttnSwitch)
#define FLAG_SCHEDDISPATCH_ISSET        (SysBase->AttnResched & ARF_AttnDispatch)

#define GET_THIS_TASK                   (SysBase->ThisTask)
#define SET_THIS_TASK(x)                (SysBase->ThisTask=(x))

#endif /* !__AROSEXEC_SMP__ */

struct Task *Exec_X86CreateIdleTask(APTR);

#include "kernel_intr.h"

#include "x86_syscalls.h"

#endif /* __EXEC_PLATFORM_H */
