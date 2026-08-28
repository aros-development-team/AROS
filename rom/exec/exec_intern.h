/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Desc: Private data belonging to exec.library
*/
#ifndef __EXEC_INTERN_H__
#define __EXEC_INTERN_H__

/* This is a short file that contains a few things every Exec function needs */

#include <aros/debug.h> /* Needed for aros_print_not_implemented macro */
#include <aros/system.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <exec_platform.h>

/*
 * Mask the IPI's FIQ around tc_SpinLock / scheduler-list critical sections.
 * arm-native defines real versions in exec_platform.h (the IPI that delivers
 * cross-CPU Signal arrives as an FIQ and would re-enter these locks on the
 * same CPU via signal_hook). Arches without an FIQ-delivered IPI get no-ops.
 */
#ifndef EXEC_FIQ_DISABLE
#define EXEC_FIQ_DISABLE()      (0)
#define EXEC_FIQ_RESTORE(x)     do { (void)(x); } while (0)
#endif

/*
 * Store-store barrier, for publishing a freshly built structure to a reader
 * that walks it lock-free. SMP arches define a real one in exec_platform.h;
 * on single-CPU builds there is no other observer, so a no-op is correct.
 */
#ifndef EXEC_MEMORY_BARRIER
#define EXEC_MEMORY_BARRIER()   do { } while (0)
#endif

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#endif

#ifndef __KERNEL_NOLIBBASE__
#define __KERNEL_NOLIBBASE__
#endif
#include <proto/kernel.h>

#define ALERT_BUFFER_SIZE 2048

#if !defined(HAVE_ExecDoInitResident)
#define HAVE_ExecDoInitResident
#define ExecDoInitResident(a,b,c) \
({ \
        a = AROS_UFC3(struct Library *, (b), \
        AROS_UFCA(struct Library *,  0L, D0), \
        AROS_UFCA(BPTR,              (c), A0), \
        AROS_UFCA(struct ExecBase *, SysBase, A6) \
    ); \
})
#endif


/*
   Internals of this structure are host-specific, we don't know them here
 */
struct HostInterface;

struct SupervisorAlertTask
{
    struct Task                 *sat_Task;                      /* Task that tries to display supervisor-level alerts           */
    BOOL                        sat_IsAvailable;
    IPTR                        sat_Params[2];
};

/*
   AROS specific private portion of ExecBase
 */
struct IntExecBase
{
    struct ExecBase             pub;
    struct List                 ResetHandlers;                  /* Reset handlers list                                          */
    struct Interrupt            ColdResetHandler;               /* Reset handler that causes cold reboot                        */
    struct Interrupt            WarmResetHandler;               /* Reset handler that causes warm reboot                        */
    struct Interrupt            ShutdownHandler;                /* Reset handler that halts CPU                                 */
    struct MinList              AllocMemList;                   /* Mungwall allocations list                                    */
    struct SignalSemaphore      LowMemSem;                      /* Lock for single-threading low memory handlers                */
    APTR                        KernelBase;                     /* kernel.resource base                                         */
    struct Library              *DebugBase;                     /* debug.library base                                           */
    ULONG                       PageSize;                       /* Memory page size                                             */
    ULONG                       IntFlags;                       /* Internal flags, see below                                    */
    struct MsgPort              *ServicePort;                   /* Message port for service task                                */
    struct List                 AllocatorCtxList;               /* List of allocator contexts for system mem headers            */
    struct Exec_PlatformData    PlatformData;                   /* Platform-specific stuff                                      */
    struct SupervisorAlertTask  SAT;
    ULONG                       SupervisorDeadEndCnt;           /* Counter of reaching AT_DeadEnd under Supervisor mode         */
    char                        AlertBuffer[ALERT_BUFFER_SIZE]; /* Buffer for alert text                                        */
    void                       *ExecLogBase;
#if defined(__AROSEXEC_BROKENMEMLOCK__)
    struct SignalSemaphore      MemListSem;                     /* Memory list protection semaphore                             */
#elif defined(__AROSEXEC_SMP__)
    void                       *ExecLockBase;
    cpumask_t                   *CPUMask;                       /* bitmap of online core                                        */
    spinlock_t                  MemListSpinLock;
    spinlock_t                  AllocMemListSpinLock;           /* mungwall AllocMemList (SMP-safe replacement for Forbid)       */
    spinlock_t                  AllocatorCtxListSpinLock;       /* AllocatorCtxList lazy-create serialization                   */
    /* First the locks for arbitration of public resources ... */
    spinlock_t                  ResourceListSpinLock;
    spinlock_t                  DeviceListSpinLock;
    spinlock_t                  IntrListSpinLock;
    spinlock_t                  LibListSpinLock;
    spinlock_t                  PortListSpinLock;
    spinlock_t                  SemListSpinLock;

    /* .. and then scheduling related locks ... */
    spinlock_t                  TaskRunningSpinLock;
    struct List                 TaskRunning;                    /* Tasks that are currently running on available CPUs           */
    spinlock_t                  TaskSpinningLock;
    struct List                 TaskSpinning;                   /* Tasks that are spinning waiting for a lock                   */
    spinlock_t                  TaskReadySpinLock;
    spinlock_t                  TaskWaitSpinLock;
#endif
};

#define PrivExecBase(base)      ((struct IntExecBase *)(base))
#define PD(base)                PrivExecBase(base)->PlatformData
#ifndef __AROS_KERNEL__
#define KernelBase              PrivExecBase(SysBase)->KernelBase
#else
#define __kernelBase            PrivExecBase(SysBase)->KernelBase
#endif
#if !defined(DEBUG_NOPRIVATEINLINE)
#define DebugBase               PrivExecBase(SysBase)->DebugBase
#endif

/* IntFlags */
#define EXECB_MungWall          0                                /* This flag can't be changed at runtime                        */
#define EXECF_MungWall          (1 << EXECB_MungWall)
#define EXECB_StackSnoop        1
#define EXECF_StackSnoop        (1 << EXECB_StackSnoop)
#define EXECB_CPUAffinity       2                                /* Set once the CPU affinity masks should be used               */
#define EXECF_CPUAffinity       (1 << EXECB_CPUAffinity)

/* Additional private task states */
#define TS_SERVICE              128

#if UseLVOs
extern void __AROS_InitExecBase (void);
#endif

struct ExecBase *PrepareExecBase(struct MemHeader *mh, struct TagItem *tags);
void InitExecBase(struct ExecBase *SysBase, ULONG negsize, struct TagItem *msg);
struct ExecBase *PrepareExecBaseMove(struct ExecBase *oldSysBase);
BOOL Exec_PreparePlatform(struct Exec_PlatformData *pdata, struct TagItem *tags);

void InitKickTags(struct ExecBase *SysBase);
UWORD GetSysBaseChkSum(struct ExecBase *sysbase);
void SetSysBaseChkSum(void);
BOOL IsSysBaseValid(struct ExecBase *sysbase);

IPTR cpu_SuperState();

void ServiceTask(struct ExecBase *SysBase);

#if defined(__AROSEXEC_SMP__)

/*
 * Task list migration helpers. Single source of truth for "atomically
 * move task on or off a SysBase task list under the matching spinlock."
 * Caller is responsible for tc_State semantics and (if needed) the
 * per-task tc_SpinLock around the (state, list-membership) tuple.
 *
 * Used by rom/exec/wait.c and by core_Switch on both x86_64 and
 * arm-native.
 */

/*
 * Scheduler-list spinlocks must be held with IRQ/FIQ-driven dispatch
 * blocked: arm-native's core_ExitInterrupt runs core_Dispatch which
 * itself acquires TaskReadySpinLock - if a dispatch fires while we
 * hold the lock, the dispatcher self-deadlocks (same CPU) or leaks
 * the lock to whoever runs next.
 *
 * EXEC_BLOCK_DISPATCH_INC/DEC bump TDNestCnt directly (per-CPU TLS,
 * no atomics needed) without going through Forbid()/Permit(). Permit's
 * decrement-to-fully-permitted path triggers Reschedule() if
 * FLAG_SCHEDSWITCH is set, yielding the CPU - the LAST thing we want
 * while holding a scheduler lock. The macros are defined per-arch in
 * exec_platform.h.
 */
static inline void exec_TaskRemoveRunning(struct Task *task)
{
    unsigned int __fiq = EXEC_FIQ_DISABLE();
    EXEC_BLOCK_DISPATCH_INC;
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL,
                       SPINLOCK_MODE_WRITE);
    Remove(&task->tc_Node);
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock);
    EXEC_BLOCK_DISPATCH_DEC;
    EXEC_FIQ_RESTORE(__fiq);
}

static inline void exec_TaskEnqueueReady(struct Task *task)
{
    unsigned int __fiq = EXEC_FIQ_DISABLE();
    EXEC_BLOCK_DISPATCH_INC;
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL,
                       SPINLOCK_MODE_WRITE);
    Enqueue(&SysBase->TaskReady, &task->tc_Node);
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskReadySpinLock);
    EXEC_BLOCK_DISPATCH_DEC;
    EXEC_FIQ_RESTORE(__fiq);
}

static inline void exec_TaskEnqueueWait(struct Task *task)
{
    unsigned int __fiq = EXEC_FIQ_DISABLE();
    EXEC_BLOCK_DISPATCH_INC;
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskWaitSpinLock, NULL,
                       SPINLOCK_MODE_WRITE);
    Enqueue(&SysBase->TaskWait, &task->tc_Node);
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskWaitSpinLock);
    EXEC_BLOCK_DISPATCH_DEC;
    EXEC_FIQ_RESTORE(__fiq);
}

static inline void exec_TaskEnqueueSpinning(struct Task *task)
{
    unsigned int __fiq = EXEC_FIQ_DISABLE();
    EXEC_BLOCK_DISPATCH_INC;
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskSpinningLock, NULL,
                       SPINLOCK_MODE_WRITE);
    Enqueue(&PrivExecBase(SysBase)->TaskSpinning, &task->tc_Node);
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskSpinningLock);
    EXEC_BLOCK_DISPATCH_DEC;
    EXEC_FIQ_RESTORE(__fiq);
}

#endif /* __AROSEXEC_SMP__ */

#endif /* __EXEC_INTERN_H__ */
