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
#include <asm/riscv/csr.h>   /* rv_intr_disable/restore, not the generic helpers */
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

/* special flag to get the scheduling code to unspin tasks */
#define TS_UNSPIN 0x10

extern struct Hook Exec_TaskSpinLockFailHook;

/*
 * The run lists are changed in kernel context: a task asks with a system
 * call, a caller already in kernel context makes the move directly
 * (kernel/kernel_scheduler.c).
 */
extern void core_SysCallReschedTask(struct Task *, ULONG);
#define krnSysCallReschedTask(task, state) core_SysCallReschedTask((task), (state))

/* RemTask and the service task go through the kernel for the run lists */
#define EXEC_REMTASK_NEEDSSWITCH

/* RemTask removing the calling task: the kernel switches it away, off
 * TaskRunning and tombstoned, and returns so RemTask can finish */
extern void core_SysCallRemSwitch(void);
#define krnSysCallSwitch() core_SysCallRemSwitch()

/*
 * Interrupt masking for the run list sections in this port's own kernel
 * and exec code. The IPI is an ordinary software interrupt here (SSI, or
 * MSI in M-mode), masked by the status register's IE bit like every
 * other. The pair nests: the disable returns the previous enable state
 * for its restore.
 */
#define EXEC_IRQFIQ_DISABLE()       rv_intr_disable()
#define EXEC_IRQFIQ_RESTORE(prev)   rv_intr_restore(prev)

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
#define FLAG_SCHEDPARK_CLEAR        do { __RV_TLS()->ScheduleFlags &= ~TLSSF_Park; } while(0)
#define FLAG_SCHEDPARK_SET          do { __RV_TLS()->ScheduleFlags |= TLSSF_Park; } while(0)
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
/* Set by Wait() under Disable(), consumed by the switch it then asks for */
#define FLAG_SCHEDPARK_CLEAR        AROS_ATOMIC_AND(__RV_TLS()->ScheduleFlags, ~TLSSF_Park)
#define FLAG_SCHEDPARK_SET          AROS_ATOMIC_OR(__RV_TLS()->ScheduleFlags, TLSSF_Park)
#endif

#define IDNESTCOUNT_GET             ((LONG)__RV_TLS()->IDNestCnt)
#define IDNESTCOUNT_SET(val)        do { __RV_TLS()->IDNestCnt = (val); } while(0)
#define TDNESTCOUNT_GET             ((LONG)__RV_TLS()->TDNestCnt)
#define TDNESTCOUNT_SET(val)        do { __RV_TLS()->TDNestCnt = (val); } while(0)
#define FLAG_SCHEDQUANTUM_ISSET     ((BOOL)((__RV_TLS()->ScheduleFlags & TLSSF_Quantum) != 0))
#define FLAG_SCHEDSWITCH_ISSET      ((BOOL)((__RV_TLS()->ScheduleFlags & TLSSF_Switch) != 0))
#define FLAG_SCHEDDISPATCH_ISSET    ((BOOL)((__RV_TLS()->ScheduleFlags & TLSSF_Dispatch) != 0))
#define FLAG_SCHEDPARK_ISSET        ((BOOL)((__RV_TLS()->ScheduleFlags & TLSSF_Park) != 0))

/* TDNestCnt is per-CPU TLS, so no atomics. Blocks IRQ-exit dispatch
 * without Forbid()/Permit(); used by this port's IPI code */
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
/* The same for a caller that holds TaskRunningSpinLock (core_Dispatch) */
#define SET_THIS_TASK_LOCKED(x) \
    do { \
        TLS_SET(ThisTask,(x)); \
        AddHead(&PrivExecBase(SysBase)->TaskRunning, (struct Node *)(x)); \
    } while (0)

#endif /* __AROSEXEC_SMP__ */

#endif /* __RISCV_EXEC_PLATFORM_H */
