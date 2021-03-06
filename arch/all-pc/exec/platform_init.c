/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/interrupts.h>
#include <exec/rawfmt.h>

#define __AROS_KERNEL__

#include "exec_intern.h"

extern void IdleTask(struct ExecBase *);
extern AROS_INTP(Exec_X86ShutdownHandler);
extern AROS_INTP(Exec_X86WarmResetHandler);
extern AROS_INTP(Exec_X86ColdResetHandler);

#if (__WORDSIZE==64)
#define EXCX_REGA    regs->rax
#define EXCX_REGB    regs->rbx
#define EXCX_REGC    regs->rcx
#else
#define EXCX_REGA    regs->eax
#define EXCX_REGB    regs->ebx
#define EXCX_REGC    regs->ecx
#endif

#define DSPIN(x)

#if defined(__AROSEXEC_SMP__)
struct Hook Exec_TaskSpinLockFailHook;
struct Hook Exec_TaskSpinLockForbidHook;
struct Hook Exec_TaskSpinLockDisableHook;

AROS_UFH3(void, Exec_TaskSpinLockFailFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(spinlock_t *, spinLock, A1),
    AROS_UFHA(void *, unused, A2))
{
    AROS_USERFUNC_INIT

    struct Task *spinTask = GET_THIS_TASK;
    struct IntETask *thisET;

    DSPIN(bug("[Exec:X86] %s()\n", __func__));
    if (spinTask)
    {
        thisET = GetIntETask(spinTask);
        if (thisET)
        {
            DSPIN(bug("[Exec:X86] %s: Setting task @ 0x%p to spinning...\n", __func__, spinTask));

            /* tell the scheduler that the task is waiting on a spinlock */
            spinTask->tc_State = TS_SPIN;

            thisET->iet_SpinLock = spinLock;

        }
    }
    DSPIN(bug("[Exec:X86] %s: Forcing Reschedule...\n", __func__));

    /* schedule us away for now .. */
    Reschedule();

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, Exec_TaskSpinLockForbidFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(spinlock_t *, spinLock, A1),
    AROS_UFHA(void *, unused, A2))
{
    AROS_USERFUNC_INIT

    DSPIN(
        struct Task *spinTask = GET_THIS_TASK;
        bug("[Exec:X86] %s(0x%p)\n", __func__, spinTask);
    )
 
    Forbid();

    DSPIN(bug("[Exec:X86] %s: done\n", __func__));

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, Exec_TaskSpinLockDisableFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(spinlock_t *, spinLock, A1),
    AROS_UFHA(void *, unused, A2))
{
    AROS_USERFUNC_INIT

    DSPIN(
        struct Task *spinTask = GET_THIS_TASK;
        bug("[Exec:X86] %s(0x%p)\n", __func__, spinTask);
    )
 
    Disable();

    DSPIN(bug("[Exec:X86] %s: done\n", __func__));

    AROS_USERFUNC_EXIT
}

void Exec_TaskSpinUnlock(spinlock_t *spinLock)
{
#if (0)
    struct Task *spinTask, *nxtTask;
    struct IntETask *thisET;
#endif
    DSPIN(bug("\n[Exec:X86] %s(0x%p)\n", __func__, spinLock));

#if (0)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskSpinningLock, NULL, SPINLOCK_MODE_WRITE);
    ForeachNodeSafe(&PrivExecBase(SysBase)->TaskSpinning, spinTask, nxtTask)
    {
        thisET = GetIntETask(spinTask);
        if ((thisET) && (thisET->iet_SpinLock == spinLock))
        {
            EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL, SPINLOCK_MODE_WRITE);
            Disable();
            Remove(&spinTask->tc_Node);
            Enqueue(&SysBase->TaskReady, &spinTask->tc_Node);
            EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskReadySpinLock);
            Enable();
        }
    }
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskSpinningLock);
#endif

    DSPIN(bug("[Exec:X86] %s: done\n\n", __func__));
}

void X86_HandleSpinLock(struct ExceptionContext *regs)
{
    struct ExecSpinSCData *spinData = (struct ExecSpinSCData *)EXCX_REGB;

    DSPIN(bug("[Exec:X86] %s(0x%p, 0x%p, %08x)\n", __func__, spinData->lock_ptr, spinData->lock_failhook, spinData->lock_mode));

    EXEC_SPINLOCK_LOCK(spinData->lock_ptr, spinData->lock_failhook, spinData->lock_mode);

    if (spinData->lock_obtainhook)
    {
        DSPIN(bug("[Exec:X86] %s: calling lock-obtained hook @ 0x%p ...\n", __func__, spinData->lock_obtainhook);)
        CALLHOOKPKT(spinData->lock_obtainhook, (APTR)spinData->lock_ptr, 0);
    }

    EXCX_REGA = (IPTR)spinData->lock_ptr;

    DSPIN(bug("[Exec:X86] %s: done\n", __func__));

    return;
}

struct syscallx86_Handler x86_SCSpinLockHandler =
{
    {
        .ln_Name = (APTR)SC_X86CPUSPINLOCK
    },
    (APTR)X86_HandleSpinLock
};

spinlock_t *ExecSpinLockCall(spinlock_t *spinLock, struct Hook *hookObtained, struct Hook *hookFailed, ULONG spinMode)
{
    struct ExecSpinSCData __spinData =
    {
        spinLock,
        hookObtained,
        hookFailed,
        spinMode
    };
    spinlock_t *__retval;
    DSPIN(bug("\n[Exec:X86] %s: attempting to lock 0x%p\n", __func__, spinLock));
    __retval = krnSysCallSpinLock(&__spinData);
    DSPIN(bug("[Exec:X86] %s: returning 0x%p\n\n", __func__, __retval));
    return __retval;
}
#endif

#if defined(EXEC_REMTASK_NEEDSSWITCH)
void X86_HandleSwitch(struct ExceptionContext *regs)
{
    D(bug("[Exec:X86] %s()\n", __func__));

    cpu_Switch(regs);

    return;
}

struct syscallx86_Handler x86_SCSwitchHandler =
{
    {
        .ln_Name = (APTR)SC_X86SWITCH
    },
    (APTR)X86_HandleSwitch
};

void X86_SetTaskState(struct Task *changeTask, ULONG newState, BOOL dolock)
{
#if defined(__AROSEXEC_SMP__)
    spinlock_t *task_listlock = NULL;
#endif
    struct List *task_list = NULL;
    D(bug("[Exec:X86] %s(0x%p,%08x)\n", __func__, changeTask, newState));

    changeTask->tc_State = newState;

    switch (newState)
    {
        case TS_RUN:
#if defined(__AROSEXEC_SMP__)
            task_listlock = &PrivExecBase(SysBase)->TaskRunningSpinLock;
            task_list = &PrivExecBase(SysBase)->TaskRunning;
#endif
            break;
        case TS_READY:
#if defined(__AROSEXEC_SMP__)
            task_listlock = &PrivExecBase(SysBase)->TaskReadySpinLock;
#endif
            task_list = &SysBase->TaskReady;
            break;
        case TS_WAIT:
#if defined(__AROSEXEC_SMP__)
            task_listlock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
#endif
            task_list = &SysBase->TaskWait;
            break;
#if defined(__AROSEXEC_SMP__)
        case TS_SPIN:
            task_listlock = &PrivExecBase(SysBase)->TaskSpinningLock;
            task_list = &PrivExecBase(SysBase)->TaskSpinning;
            break;
#endif
        default:
            break;
    }
    if (task_list)
    {
#if defined(__AROSEXEC_SMP__)
        if (dolock && task_listlock) EXEC_SPINLOCK_LOCK(task_listlock, NULL, SPINLOCK_MODE_WRITE);
#endif
        Enqueue(task_list, &changeTask->tc_Node);
#if defined(__AROSEXEC_SMP__)
        if (dolock && task_listlock) EXEC_SPINLOCK_UNLOCK(task_listlock);
#endif
    }
}

/* change a specified task's state */
void X86_HandleReschedTask(struct ExceptionContext *regs)
{
    struct Task *reschTask = (struct Task *)EXCX_REGB;
    ULONG reschState = (ULONG)EXCX_REGC;
#if defined(__AROSEXEC_SMP__)
    spinlock_t *task_listlock = NULL;
#endif

    D(bug("[Exec:X86] %s(0x%p,%08x)\n", __func__, reschTask, reschState));

    /* Move to the ready list... */
#if defined(__AROSEXEC_SMP__)
    switch (reschTask->tc_State)
    {
        case TS_RUN:
            task_listlock = &PrivExecBase(SysBase)->TaskRunningSpinLock;
            break;
        case TS_READY:
            task_listlock = &PrivExecBase(SysBase)->TaskReadySpinLock;
            break;
        case TS_WAIT:
            task_listlock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
            break;
        case TS_SPIN:
            task_listlock = &PrivExecBase(SysBase)->TaskSpinningLock;
            break;
        default:
            break;
    }
    if (task_listlock)
        EXEC_SPINLOCK_LOCK(task_listlock, NULL, SPINLOCK_MODE_WRITE);

    if ((reschTask->tc_State != TS_INVALID) && (reschTask->tc_State != TS_TOMBSTONED))
#else
    if ((reschTask->tc_State != TS_INVALID) && (reschTask->tc_State != TS_RUN))
#endif
        Remove(&reschTask->tc_Node);
#if defined(__AROSEXEC_SMP__)
    switch (reschState)
    {
        case TS_UNSPIN:
            {
#if (0)
                struct Task *spinTask, *tmpTask;
                struct IntETask *reschTaskIntET, *spinTaskIntET;
#endif
                reschState = TS_READY;
#if (0)
                reschTaskIntET = GetIntETask(reschTask);

                EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskSpinningLock, NULL, SPINLOCK_MODE_WRITE);
                ForeachNodeSafe(&PrivExecBase(SysBase)->TaskSpinning, spinTask, tmpTask)
                {
                    spinTaskIntET = GetIntETask(spinTask);
                    if ((spinTaskIntET) && (spinTaskIntET->iet_SpinLock == reschTaskIntET->iet_SpinLock))
                    {
                        bug("[Exec:X86] %s: enabling spinning task  @ 0x%p\n", __func__, spinTask);
                        spinTaskIntET->iet_SpinLock=NULL;
                        X86_SetTaskState(spinTask, TS_READY, TRUE);
                    }
                }
                EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskSpinningLock);
#endif
            }
        case TS_READY:
        case TS_SPIN:
#endif
            X86_SetTaskState(reschTask, reschState, (BOOL)(reschTask->tc_State != reschState));
#if defined(__AROSEXEC_SMP__)
            break;
        case TS_REMOVED:
                reschTask->tc_State = TS_TOMBSTONED;
            break;
    }

    if (task_listlock)
        EXEC_SPINLOCK_UNLOCK(task_listlock);
#endif

    return;
}

struct syscallx86_Handler x86_SCReschedTaskHandler =
{
    {
        .ln_Name = (APTR)SC_X86RESCHEDTASK
    },
    (APTR)X86_HandleReschedTask
};
#endif

struct Task *Exec_X86CreateIdleTask(APTR sysBase)
{
    struct ExecBase *SysBase = (struct ExecBase *)sysBase;
    struct Task *CPUIdleTask;
    char *taskName;
#if defined(__AROSEXEC_SMP__)
    struct KernelBase *KernelBase = __kernelBase;
    int cpuNo = KrnGetCPUNumber();
    IPTR idleNameArg[] =
    {
        cpuNo
    };
    struct MemList *ml;
    void *cpuMask;

    cpuMask = KrnAllocCPUMask();
    
    if ((ml = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Exec:X86.%03u] FATAL : Failed to allocate memory for idle task name info", cpuNo);
        return NULL;
    }

    ml->ml_NumEntries      = 1;

    ml->ml_ME[0].me_Length = 15;
    if ((ml->ml_ME[0].me_Addr = AllocMem(15, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Exec:X86.%03u] FATAL : Failed to allocate memory for task idle name", cpuNo);
        FreeMem(ml, sizeof(struct MemList));
        return NULL;
    }
    taskName = ml->ml_ME[0].me_Addr;
    RawDoFmt("CPU #%03u Idle", (RAWARG)idleNameArg, RAWFMTFUNC_STRING, taskName);
    
    KrnGetCPUMask(cpuNo, cpuMask);

#else
    taskName = "CPU Idle";
#endif
    CPUIdleTask = NewCreateTask(TASKTAG_NAME   , taskName,
#if defined(__AROSEXEC_SMP__)
                                TASKTAG_AFFINITY   , cpuMask,
#endif
                                TASKTAG_PRI        , -128,
                                TASKTAG_PC         , IdleTask,
                                TASKTAG_ARG1       , SysBase,
                                TAG_DONE);

    if (CPUIdleTask)
    {
        D(
            bug("[Exec:X86] %s: '%s' Task created @ 0x%p\n", __func__, CPUIdleTask->tc_Node.ln_Name, CPUIdleTask);
#if defined(__AROSEXEC_SMP__)
            bug("[Exec:X86] %s:      CPU Affinity : %08x\n", __func__, IntETask(CPUIdleTask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
#endif
        )
#if defined(__AROSEXEC_SMP__)
        AddTail(&CPUIdleTask->tc_MemEntry, &ml->ml_Node);
#endif
    }
#if defined(__AROSEXEC_SMP__)
    else
    {
        FreeMem(ml->ml_ME[0].me_Addr, 15);
        FreeMem(ml, sizeof(struct MemList));
    }
#endif
    return CPUIdleTask;
}

int Exec_X86Init(struct ExecBase *SysBase)
{
    struct IntExecBase *sysBase = (struct IntExecBase *)SysBase;
#if defined(__AROSEXEC_SMP__) || defined(EXEC_REMTASK_NEEDSSWITCH)
    struct KernelBase *KernelBase = __kernelBase;
#endif

    D(bug("[Exec:X86] %s()\n", __func__));
    D(bug("[Exec:X86] %s: KernelBase @ 0x%p\n", __func__, __kernelBase));
    D(bug("[Exec:X86] %s: PlatformData @ 0x%p\n", __func__, &sysBase->PlatformData));

    /* Install The default Power Management handlers */
    sysBase->ColdResetHandler.is_Node.ln_Pri = -64;
    sysBase->ColdResetHandler.is_Node.ln_Name = "Keyboard Controller Reset";
    sysBase->ColdResetHandler.is_Code = (VOID_FUNC)Exec_X86ColdResetHandler;
    sysBase->ColdResetHandler.is_Data = &sysBase->ColdResetHandler;
    AddResetCallback(&sysBase->ColdResetHandler);

    sysBase->WarmResetHandler.is_Node.ln_Pri = -64;
    sysBase->WarmResetHandler.is_Node.ln_Name = "System Reset";
    sysBase->WarmResetHandler.is_Code = (VOID_FUNC)Exec_X86WarmResetHandler;
    sysBase->WarmResetHandler.is_Data = &sysBase->WarmResetHandler;
    AddResetCallback(&sysBase->WarmResetHandler);

    sysBase->ShutdownHandler.is_Node.ln_Pri = -128;
    sysBase->ShutdownHandler.is_Node.ln_Name = "System Shutdown";
    sysBase->ShutdownHandler.is_Code = (VOID_FUNC)Exec_X86ShutdownHandler;
    sysBase->ShutdownHandler.is_Data = &sysBase->ShutdownHandler;
    AddResetCallback(&sysBase->ShutdownHandler);

    D(bug("[Exec:X86] %s: Default Handlers Registered\n", __func__));

#if defined(EXEC_REMTASK_NEEDSSWITCH)
    krnAddSysCallHandler(KernelBase->kb_PlatformData, &x86_SCSwitchHandler, TRUE, TRUE);
    krnAddSysCallHandler(KernelBase->kb_PlatformData, &x86_SCReschedTaskHandler, TRUE, TRUE);
#endif
#if defined(__AROSEXEC_SMP__)
    /* register the task spinlock syscall */
    krnAddSysCallHandler(KernelBase->kb_PlatformData, &x86_SCSpinLockHandler, TRUE, TRUE);
    sysBase->PlatformData.SpinLockCall = ExecSpinLockCall;

    /* set up the task spinning hooks */
    Exec_TaskSpinLockForbidHook.h_Entry = (HOOKFUNC)Exec_TaskSpinLockForbidFunc;
    Exec_TaskSpinLockDisableHook.h_Entry = (HOOKFUNC)Exec_TaskSpinLockDisableFunc;
    Exec_TaskSpinLockFailHook.h_Entry = (HOOKFUNC)Exec_TaskSpinLockFailFunc;
    D(bug("[Exec:X86] %s: SpinLock Fail Hook @ 0x%p, Handler @ 0x%p\n", __func__, &Exec_TaskSpinLockFailHook, Exec_TaskSpinLockFailFunc));
    D(bug("[Exec:X86] %s: SpinLock Forbid Hook @ 0x%p, Handler @ 0x%p\n", __func__, &Exec_TaskSpinLockForbidHook, Exec_TaskSpinLockForbidFunc));
#endif

    Exec_X86CreateIdleTask(SysBase);

    return TRUE;
}

ADD2INITLIB(Exec_X86Init, -127)
