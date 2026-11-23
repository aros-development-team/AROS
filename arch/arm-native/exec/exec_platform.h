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

extern void Kernel_49_KrnSpinInit(spinlock_t *, void *);
#define EXEC_SPINLOCK_INIT(a) Kernel_49_KrnSpinInit((a), NULL)
extern spinlock_t *Kernel_52_KrnSpinLock(spinlock_t *, struct Hook *, ULONG, void *);
#define EXEC_SPINLOCK_LOCK(a,b,c) Kernel_52_KrnSpinLock((a), (b), (c), NULL)
extern void Kernel_53_KrnSpinUnLock(spinlock_t *, void *);
#define EXEC_SPINLOCK_UNLOCK(a) Kernel_53_KrnSpinUnLock((a), NULL)

/*
 * Store-store barrier for publishing a freshly built structure to readers
 * that walk it without taking a lock. DMB is available unprivileged on
 * ARMv7-A, so this is usable from task context in rom/exec.
 */
#define EXEC_MEMORY_BARRIER()   asm volatile("dmb" ::: "memory")

/* special flag to get the scheduling code to unspin tasks */
#define TS_UNSPIN 0x10

extern struct Hook Exec_TaskSpinLockFailHook;

struct Task;

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
#define FLAG_SCHEDPARK_CLEAR \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->ScheduleFlags &= ~TLSSF_Park; \
    } while(0)
#define FLAG_SCHEDPARK_SET \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->ScheduleFlags |= TLSSF_Park; \
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
/* Set by Wait() under Disable(), consumed by the switch it then asks for */
#define FLAG_SCHEDPARK_CLEAR \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_AND(__tls->ScheduleFlags, ~TLSSF_Park); \
    } while(0)
#define FLAG_SCHEDPARK_SET \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        AROS_ATOMIC_OR(__tls->ScheduleFlags, TLSSF_Park); \
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
#define FLAG_SCHEDPARK_ISSET \
    ({ \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        BOOL __ret = ((__tls->ScheduleFlags & TLSSF_Park) != 0); \
        __ret;  \
    })
/*
 * Plain (non-atomic) TDNestCnt bumps: TDNestCnt is in the per-CPU TLS.
 * They keep the interrupt exit path from switching tasks without going
 * through Forbid()/Permit(). Used by this port's IPI code.
 */
#define EXEC_BLOCK_DISPATCH_INC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->TDNestCnt++; \
    } while(0)

#define EXEC_BLOCK_DISPATCH_DEC \
    do { \
        tls_t *__tls; \
        asm volatile("mrc p15, 0, %0, c13, c0, 3":"=r"(__tls)); \
        __tls->TDNestCnt--; \
    } while(0)

/*
 * Interrupt masking for the run list sections in this port's own kernel
 * code: IRQ and FIQ both, because the inter-CPU IPI arrives as an FIQ
 * and its handler takes the run list locks too. The pair nests: the
 * disable returns the previous I and F bits, and the restore only opens
 * what was open before.
 *
 * This works in a privileged mode only. In USER mode cpsid does nothing;
 * a task masks interrupts with Disable(), which masks IRQ and FIQ through
 * a system call (SC_CLI).
 */
#define EXEC_IRQFIQ_DISABLE() \
    ({ unsigned int __cpsr; \
       asm volatile("mrs %0, cpsr\n\tcpsid if" : "=r"(__cpsr) :: "memory"); \
       (__cpsr & 0xc0); })

#define EXEC_IRQFIQ_RESTORE(prev) \
    do { \
        if (!((prev) & 0x80)) asm volatile("cpsie i" ::: "memory"); \
        if (!((prev) & 0x40)) asm volatile("cpsie f" ::: "memory"); \
    } while(0)

#define GET_THIS_TASK           TLS_GET(ThisTask)
#define SCHEDQUANTUM_SET(val)           TLS_SET(Quantum,(val))
#define SCHEDQUANTUM_GET                TLS_GET(Quantum)
#define SCHEDELAPSED_SET(val)           TLS_SET(Elapsed,(val))
#define SCHEDELAPSED_GET                TLS_GET(Elapsed)
#if !defined(__AROSEXEC_SMP__)
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x))
#else
/*
 * SET_THIS_TASK takes TaskRunningSpinLock, so it is used in kernel
 * context with IRQ and FIQ masked, like every other run list section
 * (core_Dispatch, cpu_BootStrap, exec's init on the boot CPU).
 */
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
#endif

#endif /* __EXEC_PLATFORM_H */
