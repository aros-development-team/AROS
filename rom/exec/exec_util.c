/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exec utility functions.
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>

#include <proto/exec.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "taskstorage.h"

/****************************************************************************

    NAME */
#include "exec_util.h"

	struct ETask * Exec_FindChild(

/*  SYNOPSIS */
	ULONG	    id,
	struct ExecBase *SysBase)

/*  FUNCTION
	Scan through the current tasks children list searching for the task
	whose et_UniqueID field matches.

    INPUTS
	id	-   The task ID to match.

    RESULT
	Address of the ETask structure that matches, or
	NULL otherwise.

    NOTES
        This is an internal exec.library function not exported from the
        library.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct Task *ThisTask = GET_THIS_TASK;
    struct ETask *et, *retVal = NULL;
    struct ETask *thisET;

    thisET = GetETask(ThisTask);
    if (thisET != NULL)
    {
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_LOCK(&IntETask(thisET)->iet_TaskLock, NULL, SPINLOCK_MODE_READ);
#endif
	ForeachNode (&thisET->et_Children, et)
	{
	    if (et->et_UniqueID == id)
            {
		retVal = et;
                break;
            }
	}
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(&IntETask(thisET)->iet_TaskLock);
#endif
        if (!retVal)
        {
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_LOCK(&thisET->et_TaskMsgPort.mp_SpinLock, NULL, SPINLOCK_MODE_READ);
#endif
            ForeachNode(&thisET->et_TaskMsgPort.mp_MsgList, et)
            {
                if (et->et_UniqueID == id)
                {
                    retVal = et;
                    break;
                }
            }
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(&thisET->et_TaskMsgPort.mp_SpinLock);
#endif
        }
    }
    return retVal;
}

BOOL
Exec_InitETask(struct Task *task, struct Task *parent, struct ExecBase *SysBase)
{
    /*
     *  We don't add this to the task memory, it isn't free'd by
     *  RemTask(), rather by somebody else calling ChildFree().
     *  Alternatively, an orphaned task will free its own ETask.
     */
#if defined(__AROSEXEC_SMP__)
    int cpunum;
#endif
    struct ETask *et =
        AllocMem(sizeof(struct IntETask), MEMF_PUBLIC | MEMF_CLEAR);

    D(
        bug("[EXEC:ETask] Init: Allocated ETask for Task '%s' @ %p\n",
        task->tc_Node.ln_Name, task);
        bug("[EXEC:ETask] Init:            ETask @ 0x%p, %d bytes\n",
        et, sizeof(struct IntETask));
    )

    task->tc_UnionETask.tc_ETask = et;
    if (!et)
        return FALSE;
    task->tc_Flags |= TF_ETASK;

#if defined(__AROSEXEC_SMP__)
    cpunum = KrnGetCPUNumber();
    EXEC_SPINLOCK_INIT(&IntETask(et)->iet_TaskLock);
    if (PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity)
    {
        IntETask(et)->iet_CpuAffinity = KrnAllocCPUMask();
        KrnGetCPUMask(cpunum, IntETask(et)->iet_CpuAffinity);

        D(bug("[EXEC:ETask] Init: CPU #%d, mask %08x\n", cpunum, IntETask(et)->iet_CpuAffinity);)
    }
#endif

    et->et_Parent = parent;
    NEWLIST(&et->et_Children);

    D(bug("[EXEC:ETask] Init: Parent @ 0x%p\n", et->et_Parent);)

    /* Initialise the message list */
    InitMsgPort(&et->et_TaskMsgPort);
    et->et_TaskMsgPort.mp_SigTask = task;
    et->et_TaskMsgPort.mp_SigBit = SIGB_CHILD;

    /* Initialise the trap fields */
    et->et_TrapAlloc = SysBase->TaskTrapAlloc;
    et->et_TrapAble = 0;

#ifdef DEBUG_ETASK
    {
	int len = strlen(task->tc_Node.ln_Name) + 1;
	IntETask(et)->iet_Me = AllocVec(len, MEMF_CLEAR|MEMF_PUBLIC);
	if (IntETask(et)->iet_Me != NULL)
	    CopyMem(task->tc_Node.ln_Name, IntETask(et)->iet_Me, len);
    }
#endif

    D(bug("[EXEC:ETask] Init: Generating Unique ID...\n");)

    /* Get a unique identifier for this task */
    Forbid();
    while(et->et_UniqueID == 0)
    {
        //TODO: Replace with UUID!

	/*
	 *	Add some fuzz on wrapping. It's likely that the early numbers
	 *	where taken by somebody else.
	 */
	if(++SysBase->ex_TaskID == 0)
	    SysBase->ex_TaskID = 1024;

	Disable();
        D(bug("[EXEC:ETask] Init: Checking for existing ID...\n");)
	if (FindTaskByPID(SysBase->ex_TaskID) == NULL)
	    et->et_UniqueID = SysBase->ex_TaskID;
        D(bug("[EXEC:ETask] Init: done\n");)
	Enable();
    }
   
    D(bug("[EXEC:ETask] Init:     Task ID : %08x\n", et->et_UniqueID);)

    /* Finally if the parent task is an ETask, add myself as its child */
    if(et->et_Parent && ((struct Task*) et->et_Parent)->tc_Flags & TF_ETASK)
    {
        struct ETask * parentEtask = GetETask(et->et_Parent);
        D(bug("[EXEC:ETask] Init: Registering with Parent ETask\n");)
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_LOCK(&IntETask(parentEtask)->iet_TaskLock, NULL, SPINLOCK_MODE_WRITE);
#endif
	ADDHEAD(&parentEtask->et_Children, et);
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(&IntETask(parentEtask)->iet_TaskLock);
#endif
    }
    Permit();

    D(bug("[EXEC:ETask] Init: Initialized\n");)

    return TRUE;
}

void
Exec_CleanupETask(struct Task *task, struct ExecBase *SysBase)
{
    struct ETask *et, *child, *nextchild, *parent;
    struct Node *tmpNode;
    BOOL expunge = TRUE;

    if(!(task->tc_Flags & TF_ETASK) ||
        ((et = task->tc_UnionETask.tc_ETask) == NULL))
	return;

    D(bug("[EXEC:ETask] Cleanup: Task @ 0x%p, ETask @ 0x%p\n", task, et);)

    Forbid();

#if defined(__AROSEXEC_SMP__)
    if ((PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) && (IntETask(et)->iet_CpuAffinity))
    {
        if ((IPTR)IntETask(et)->iet_CpuAffinity != TASKAFFINITY_ANY)
            KrnFreeCPUMask(IntETask(et)->iet_CpuAffinity);
    }
#endif

#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&et->et_TaskMsgPort.mp_SpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
    /* Clean up after all the children that the task didn't do itself. */
    ForeachNodeSafe(&et->et_TaskMsgPort.mp_MsgList, child, tmpNode)
    {
        ExpungeETask(child);
    }
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&et->et_TaskMsgPort.mp_SpinLock);
#endif
    /* If we have an ETask parent, tell it we have exited. */
    if(et->et_Parent != NULL)
    {
        parent = GetETask(et->et_Parent);

#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_LOCK(&IntETask(et)->iet_TaskLock, NULL, SPINLOCK_MODE_WRITE);
#endif
        /* Link children to our parent. */
        ForeachNodeSafe(&et->et_Children, child, nextchild)
        {
            child->et_Parent = et->et_Parent;
            //Forbid();
            if (parent)
            {
#if defined(__AROSEXEC_SMP__)
                EXEC_SPINLOCK_LOCK(&IntETask(parent)->iet_TaskLock, NULL, SPINLOCK_MODE_WRITE);
#endif                
                ADDTAIL(&parent->et_Children, child);
#if defined(__AROSEXEC_SMP__)
                EXEC_SPINLOCK_UNLOCK(&IntETask(parent)->iet_TaskLock);
#endif
            }
            //Permit();
        }
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(&IntETask(et)->iet_TaskLock);
#endif

        /* Notify parent only if child was created with NP_NotifyOnDeath set 
           to TRUE */
        if(parent != NULL)
        {
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_LOCK(&IntETask(parent)->iet_TaskLock, NULL, SPINLOCK_MODE_WRITE);
#endif   
            REMOVE(et);
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(&IntETask(parent)->iet_TaskLock);
#endif
            if(
               (((struct Task *)task)->tc_Node.ln_Type == NT_PROCESS) && 
               (((struct Process*) task)->pr_Flags & PRF_NOTIFYONDEATH)
            )
            {
                PutMsg(&parent->et_TaskMsgPort, (struct Message *)et);
                expunge = FALSE;
            }
        }
    }
    else
    {
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_LOCK(&IntETask(et)->iet_TaskLock, NULL, SPINLOCK_MODE_WRITE);
#endif
        /* Orphan all our remaining children. */
        ForeachNode(&et->et_Children, child)
            child->et_Parent = NULL;
#if defined(__AROSEXEC_SMP__)
        EXEC_SPINLOCK_UNLOCK(&IntETask(et)->iet_TaskLock);
#endif
    }

    if(expunge)
        ExpungeETask(et);

    Permit();
}

void
Exec_ExpungeETask(struct ETask *et, struct ExecBase *SysBase)
{
    IPTR *ts = et->et_TaskStorage;

    FreeVec(et->et_Result2);

#ifdef DEBUG_ETASK
    FreeVec(IntETask(et)->iet_Me);
#endif
    D(bug("[EXEC:ETask] Expunge: Freeing ETask @ 0x%p, TS @ 0x%p, size=%d\n",
          et, ts, ts ? (ULONG)ts[__TS_FIRSTSLOT] : 0
    );)
    FreeMem(et, sizeof(struct IntETask));
    if (ts)
        FreeMem(ts, ts[__TS_FIRSTSLOT] * sizeof(ts[0]));
}

BOOL Exec_CheckTask(struct Task *task, struct ExecBase *SysBase)
{
    struct Task *t;

    if (!task)
	return FALSE;

    Forbid();
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_READ);
    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, t)
    {
        if (task == t)
        {
            EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock);
            Permit();
            return TRUE;
        }
    }
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskRunningSpinLock);
    Permit();
    Forbid();
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskSpinningLock, NULL, SPINLOCK_MODE_READ);
    ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, t)
    {
        if (task == t)
        {
            EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskSpinningLock);
            Permit();
            return TRUE;
        }
    }
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskSpinningLock);
    Permit();
    Forbid();
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL, SPINLOCK_MODE_READ);
#else
    if (task == GET_THIS_TASK)
    {
    	Permit();
    	return TRUE;
    }
#endif

    ForeachNode(&SysBase->TaskReady, t)
    {
    	if (task == t)
    	{
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskReadySpinLock);
#endif
            Permit();
    	    return TRUE;
    	}
    }
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskReadySpinLock);
    Permit();
    Forbid();
    EXEC_SPINLOCK_LOCK(&PrivExecBase(SysBase)->TaskWaitSpinLock, NULL, SPINLOCK_MODE_READ);
#endif
    ForeachNode(&SysBase->TaskWait, t)
    {
    	if (task == t)
    	{
#if defined(__AROSEXEC_SMP__)
            EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskWaitSpinLock);
#endif
            Permit();
    	    return TRUE;
    	}
    }
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&PrivExecBase(SysBase)->TaskWaitSpinLock);
#endif
    Permit();

    return FALSE;
}
