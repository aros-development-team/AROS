/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native RISC-V exec platform definitions.

    Uniprocessor builds keep the per-CPU scheduler state in ExecBase,
    exactly as rom/exec/exec_platform.h does. SMP builds keep it in the
    per-hart TLS that tp points to (see kernel/tls.h), following the
    aarch64-native port.
*/
#ifndef __RISCV_EXEC_PLATFORM_H
#define __RISCV_EXEC_PLATFORM_H

#include <aros/config.h>

#if !defined(__AROSEXEC_SMP__)

/* Uniprocessor: the generic definitions (they guard on __EXEC_PLATFORM_H) */
#include "../../../rom/exec/exec_platform.h"

#else /* __AROSEXEC_SMP__ */

#define __EXEC_PLATFORM_H

#define SCHEDQUANTUM_VALUE      4

#include <aros/atomic.h>
#include <aros/types/spinlock_s.h>
#include <utility/hooks.h>

extern void Kernel_49_KrnSpinInit(spinlock_t *, void *);
#define EXEC_SPINLOCK_INIT(a) Kernel_49_KrnSpinInit((a), NULL)
extern spinlock_t *Kernel_52_KrnSpinLock(spinlock_t *, struct Hook *, ULONG, void *);
/* (lock, failhook, mode) - rom/exec passes all three */
#define EXEC_SPINLOCK_LOCK(a,b,c) Kernel_52_KrnSpinLock((a), (b), (c), NULL)
extern void Kernel_53_KrnSpinUnLock(spinlock_t *, void *);
#define EXEC_SPINLOCK_UNLOCK(a) Kernel_53_KrnSpinUnLock((a), NULL)

/*
 * Store-store barrier for publishing a freshly built structure to
 * readers that walk it without taking a lock.
 */
#define EXEC_MEMORY_BARRIER()   asm volatile("fence w, w" ::: "memory")

/* No syscall needed: just list mutation under spinlocks */
extern void Exec_ReschedTask(struct Task *, ULONG);
#define krnSysCallReschedTask(task, state) Exec_ReschedTask((task), (state))

/* Without this RemTask/ServiceTask mutate the scheduler lists unlocked */
#define EXEC_REMTASK_NEEDSSWITCH

/* RemTask's suicide path: detach and tombstone, then RETURN so RemTask
 * can finish its teardown (KrnSwitch would never come back) */
extern void Exec_SuicideSwitch(void);
#define krnSysCallSwitch() Exec_SuicideSwitch()

/*
 * Interrupt masking around tc_SpinLock / scheduler-list sections. The
 * IPI is an ordinary supervisor interrupt here (SSI), masked by
 * sstatus.SIE like every other, so the "FIQ" and "IRQ+FIQ" pairs that
 * rom/exec and the scheduler use are the same operation. They nest:
 * each returns the previous enable state for its restore.
 *
 * Because the IPI is an ordinary interrupt, signal_hook may call the
 * full Signal(): __AROSEXEC_IPI_RESTRICTED_CTX__ is not defined.
 */
#define EXEC_FIQ_DISABLE() \
    ({ unsigned long __ss; \
       asm volatile("csrrci %0, sstatus, 2" : "=r"(__ss) :: "memory"); \
       (unsigned int)(__ss & 2); })

#define EXEC_FIQ_RESTORE(prev) \
    do { if (prev) asm volatile("csrsi sstatus, 2" ::: "memory"); } while(0)

#define EXEC_IRQFIQ_DISABLE()       EXEC_FIQ_DISABLE()
#define EXEC_IRQFIQ_RESTORE(prev)   EXEC_FIQ_RESTORE(prev)

#include "tls.h"

struct Exec_PlatformData
{
    /* No platform-specific data by default */
};

#define __RV_TLS() \
    ({ tls_t *__tls; asm volatile("mv %0, tp" : "=r"(__tls)); __tls; })

#if defined(AROS_NO_ATOMIC_OPERATIONS)
#define IDNESTCOUNT_INC             do { __RV_TLS()->IDNestCnt++; } while(0)
#define IDNESTCOUNT_DEC             do { __RV_TLS()->IDNestCnt--; } while(0)
#define TDNESTCOUNT_INC             do { __RV_TLS()->TDNestCnt++; } while(0)
#define TDNESTCOUNT_DEC             do { __RV_TLS()->TDNestCnt--; } while(0)
#define FLAG_SCHEDQUANTUM_CLEAR     do { __RV_TLS()->ScheduleFlags &= ~TLSSF_Quantum; } while(0)
#define FLAG_SCHEDQUANTUM_SET       do { __RV_TLS()->ScheduleFlags |= TLSSF_Quantum; } while(0)
#define FLAG_SCHEDSWITCH_CLEAR      do { __RV_TLS()->ScheduleFlags &= ~TLSSF_Switch; } while(0)
#define FLAG_SCHEDSWITCH_SET        do { __RV_TLS()->ScheduleFlags |= TLSSF_Switch; } while(0)
#define FLAG_SCHEDDISPATCH_CLEAR    do { __RV_TLS()->ScheduleFlags &= ~TLSSF_Dispatch; } while(0)
#define FLAG_SCHEDDISPATCH_SET      do { __RV_TLS()->ScheduleFlags |= TLSSF_Dispatch; } while(0)
#else
#define IDNESTCOUNT_INC             AROS_ATOMIC_INC(__RV_TLS()->IDNestCnt)
#define IDNESTCOUNT_DEC             AROS_ATOMIC_DEC(__RV_TLS()->IDNestCnt)
#define TDNESTCOUNT_INC             AROS_ATOMIC_INC(__RV_TLS()->TDNestCnt)
#define TDNESTCOUNT_DEC             AROS_ATOMIC_DEC(__RV_TLS()->TDNestCnt)
#define FLAG_SCHEDQUANTUM_CLEAR     AROS_ATOMIC_AND(__RV_TLS()->ScheduleFlags, ~TLSSF_Quantum)
#define FLAG_SCHEDQUANTUM_SET       AROS_ATOMIC_OR(__RV_TLS()->ScheduleFlags, TLSSF_Quantum)
#define FLAG_SCHEDSWITCH_CLEAR      AROS_ATOMIC_AND(__RV_TLS()->ScheduleFlags, ~TLSSF_Switch)
#define FLAG_SCHEDSWITCH_SET        AROS_ATOMIC_OR(__RV_TLS()->ScheduleFlags, TLSSF_Switch)
#define FLAG_SCHEDDISPATCH_CLEAR    AROS_ATOMIC_AND(__RV_TLS()->ScheduleFlags, ~TLSSF_Dispatch)
#define FLAG_SCHEDDISPATCH_SET      AROS_ATOMIC_OR(__RV_TLS()->ScheduleFlags, TLSSF_Dispatch)
#endif

#define IDNESTCOUNT_GET             ((LONG)__RV_TLS()->IDNestCnt)
#define IDNESTCOUNT_SET(val)        do { __RV_TLS()->IDNestCnt = (val); } while(0)
#define TDNESTCOUNT_GET             ((LONG)__RV_TLS()->TDNestCnt)
#define TDNESTCOUNT_SET(val)        do { __RV_TLS()->TDNestCnt = (val); } while(0)
#define FLAG_SCHEDQUANTUM_ISSET     ((BOOL)((__RV_TLS()->ScheduleFlags & TLSSF_Quantum) != 0))
#define FLAG_SCHEDSWITCH_ISSET      ((BOOL)((__RV_TLS()->ScheduleFlags & TLSSF_Switch) != 0))
#define FLAG_SCHEDDISPATCH_ISSET    ((BOOL)((__RV_TLS()->ScheduleFlags & TLSSF_Dispatch) != 0))

/* TDNestCnt is per-CPU TLS, so no atomics. Blocks IRQ-exit dispatch
 * without Forbid()/Permit() */
#define EXEC_BLOCK_DISPATCH_INC     do { __RV_TLS()->TDNestCnt++; } while(0)
#define EXEC_BLOCK_DISPATCH_DEC     do { __RV_TLS()->TDNestCnt--; } while(0)

#define GET_THIS_TASK                   TLS_GET(ThisTask)
#define SCHEDQUANTUM_SET(val)           TLS_SET(Quantum,(val))
#define SCHEDQUANTUM_GET                TLS_GET(Quantum)
#define SCHEDELAPSED_SET(val)           TLS_SET(Elapsed,(val))
#define SCHEDELAPSED_GET                TLS_GET(Elapsed)
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x)); \
    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_WRITE); \
    AddHead(&PrivExecBase(SysBase)->TaskRunning, (struct Node *)(x)); \
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock)

#endif /* __AROSEXEC_SMP__ */

#endif /* __RISCV_EXEC_PLATFORM_H */
