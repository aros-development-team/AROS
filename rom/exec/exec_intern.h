/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private data belonging to exec.library
    Lang:
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

#if defined(__AROSEXEC_SMP__)
#include <aros/types/spinlock_s.h>
#endif

#define __KERNEL_NOLIBBASE__
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
    struct MinList              TaskStorageSlots;               /* List of free slots, always one element with next slot        */
    struct List                 AllocatorCtxList;               /* List of allocator contexts for system mem headers            */
    struct Exec_PlatformData    PlatformData;                   /* Platform-specific stuff                                      */
    struct SupervisorAlertTask  SAT;
    char                        AlertBuffer[ALERT_BUFFER_SIZE]; /* Buffer for alert text                                        */
    void                       *ExecLogBase;
#if defined(__AROSEXEC_BROKENMEMLOCK__)
    struct SignalSemaphore      MemListSem;                     /* Memory list protection semaphore                             */
#elif defined(__AROSEXEC_SMP__)
    void                       *ExecLockBase;
    cpumask_t                   *CPUMask;                       /* bitmap of online core                                        */
    spinlock_t                  MemListSpinLock;
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
#define __kernelBase              PrivExecBase(SysBase)->KernelBase
#endif
#define DebugBase               PrivExecBase(SysBase)->DebugBase

/* IntFlags */
#define EXECF_MungWall          0x0001                          /* This flag can't be changed at runtime                        */
#define EXECF_StackSnoop        0x0002
#define EXECF_CPUAffinity       0x0004                          /* Set once the CPU affinity masks should be used               */

/* Additional private task states */
#define TS_SERVICE              128

/* Puddle size, in slots. Must be at least 1 */
#define TASKSTORAGEPUDDLE       16

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

#endif /* __EXEC_INTERN_H__ */
