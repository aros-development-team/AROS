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

/*
 * arm-native does not need a syscall for reschedule: the work is just
 * task-list mutation under spinlocks, no privilege change required.
 * Exec_ReschedTask is implemented in arch/arm-native/exec/platform_init.c.
 */
extern void Exec_ReschedTask(struct Task *, ULONG);
#define krnSysCallReschedTask(task, state) Exec_ReschedTask((task), (state))

/*
 * Route RemTask/ServiceTask through the locked reschedule path. Without
 * this, rom/exec/remtask.c and service.c take their #if !defined branch
 * and mutate the scheduler lists (Remove/Enqueue) with no spinlock held
 * - safe single-threaded, but it corrupts the lists once tasks run on
 * more than one core. all-pc defines this for the same reason.
 */
#define EXEC_REMTASK_NEEDSSWITCH

/*
 * Switch-only primitive for RemTask's self-removal (suicide) path. The
 * task has already set TS_REMOVED; detach it from TaskRunning under the
 * list lock and tombstone it for the service task, then RETURN so
 * RemTask can finish its own teardown and send itself to the service
 * task before the final KrnDispatch. (KrnSwitch would also dispatch and
 * never return, which would strand the half-torn-down task.)
 */
extern void Exec_SuicideSwitch(void);
#define krnSysCallSwitch() Exec_SuicideSwitch()

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
/*
 * Plain (non-atomic) TDNestCnt bumps. TDNestCnt lives in per-CPU TLS,
 * so concurrent updates from other cores aren't possible. Used by the
 * scheduler-list helpers in rom/exec/exec_intern.h to block IRQ-exit
 * dispatch without going through Forbid()/Permit() (Permit's decrement
 * can trigger Reschedule() which yields - exactly what we don't want
 * while holding a scheduler list spinlock).
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
 * Mask FIQ across scheduler-list / tc_SpinLock critical sections. On this
 * platform the ONLY FIQ source is the inter-core mailbox (the IPI). An IPI
 * that lands while this CPU holds a scheduler lock re-enters that lock via
 * bcm2708_fiq_process -> handle_ipi -> signal_hook -> Signal ->
 * Exec_ReschedTask on the SAME CPU and self-deadlocks (TaskReadySpinLock
 * read held by core_Schedule vs the IPI's write acquire). EXEC_BLOCK_DISPATCH
 * stops the dispatcher but not FIQ, so it does not cover this.
 *
 * EXEC_FIQ_DISABLE returns the prior F bit and disables FIQ; EXEC_FIQ_RESTORE
 * re-enables FIQ only if it was enabled on entry, so the pair nests safely
 * (an already-in-FIQ caller stays masked). The mailbox SET register latches
 * the pending IPI; bcm2708_fiq_process drains it the moment FIQ is unmasked,
 * so nothing is lost - delivery is just deferred to the end of the section.
 */
#define EXEC_FIQ_DISABLE() \
    ({ unsigned int __cpsr; \
       asm volatile("mrs %0, cpsr\n\tcpsid f" : "=r"(__cpsr) :: "memory"); \
       (__cpsr & 0x40); })

#define EXEC_FIQ_RESTORE(prevF) \
    do { if (!(prevF)) asm volatile("cpsie f" ::: "memory"); } while(0)

/*
 * The cross-CPU Signal IPI is delivered as an FIQ on this platform, so its
 * hook (signal_hook in rom/exec/signal.c) runs in restricted interrupt
 * context: it must NOT call the full Signal() (Disable()/Enable() issue svc
 * syscalls that nest on the FIQ SVC stack, and Reschedule() would switch
 * tasks from inside the handler). signal_hook therefore delivers inline.
 * Arches whose Signal IPI arrives as an ordinary interrupt leave this
 * undefined and signal_hook just calls Signal().
 */
#define __AROSEXEC_IPI_RESTRICTED_CTX__

/*
 * Raw IRQ+FIQ mask/restore (privileged only). Used by scheduler primitives
 * that are reachable from the FIQ/IPI handler (Exec_ReschedTask) where the
 * exec Disable()/Enable() path is unusable: Disable() always issues a
 * KrnCli svc, and that svc nests an exception on the FIQ-handler SVC stack.
 * Raw cpsid avoids the syscall. Saves and restores the prior I|F bits so it
 * nests and leaves an already-masked caller masked. In USER mode cpsid is a
 * no-op, but every such caller already holds interrupts off (Signal Disable's
 * first, the IPI handler runs FIQ/IRQ-masked), so it is safe there too.
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
 * SET_THIS_TASK is invoked from core_Dispatch which runs with IRQs
 * already masked (IRQ-exit path). The TaskRunningSpinLock acquire here
 * therefore inherits IRQ-disabled context; no explicit Disable() needed.
 * Callers from non-dispatch context must Disable() themselves before
 * invoking SET_THIS_TASK to avoid the scheduler-list self-deadlock.
 */
#define SET_THIS_TASK(x)        TLS_SET(ThisTask,(x)); \
    KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_WRITE); \
    AddHead(&PrivExecBase(SysBase)->TaskRunning, (struct Node *)(x)); \
    KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock)
#endif

#endif /* __EXEC_PLATFORM_H */
