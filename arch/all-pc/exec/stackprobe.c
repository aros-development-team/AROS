/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Report a stack overrun found by the scheduler
*/

#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

void Alert_DisplayKrnAlert(struct Task * task, ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data,
        struct ExecBase *SysBase);

static struct Task *StackProbeTask;

AROS_INTH1(Exec_StackProbeInt, struct ExecBase *, SysBase)
{
    AROS_INTFUNC_INIT

    struct Task *task = __atomic_exchange_n(&StackProbeTask, NULL, __ATOMIC_ACQ_REL);

    if (task)
    {
        struct IntExecBase *IntSysBase = PrivExecBase(SysBase);

        if (IntSysBase->SAT.sat_IsAvailable)
        {
            IntSysBase->SAT.sat_Params[0] = AN_StackProbe;
            IntSysBase->SAT.sat_Params[1] = (IPTR)task;
            Signal(IntSysBase->SAT.sat_Task, SIGF_SINGLE);
        }
        else
        {
            struct IntETask *iet = GetIntETask(task);

            Alert_DisplayKrnAlert(task, AN_StackProbe, iet->iet_AlertLocation, iet->iet_AlertStack,
                    iet->iet_AlertType, ETaskAlertData(iet), SysBase);
        }
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

static struct Interrupt StackProbeInt =
{
    { NULL, NULL, NT_INTERRUPT, 0, "stack probe" }, NULL, (VOID_FUNC)Exec_StackProbeInt
};

/*
 * Called by the scheduler, which parks the task, with its tc_SpinLock held
 * on SMP. The alert is recorded in the task as for a CPU trap; a soft
 * interrupt hands it to the supervisor alert task once the scheduler is
 * done. One task is reported at a time; another one meanwhile is parked
 * only.
 */
void Exec_StackProbe(struct Task *task, struct ExecBase *SysBase)
{
    struct IntETask *iet = GetIntETask(task);
    struct ExceptionContext *ctx;
    struct Task *none = NULL;

    if (!iet || (iet->iet_AlertFlags & AF_Alert))
        return;

    ctx = iet->iet_ETask.et_RegFrame;
    iet->iet_AlertType     = AT_CPU;
    iet->iet_AlertFlags    = AF_Alert | AF_Location;
    iet->iet_AlertCode     = AN_StackProbe;
    iet->iet_AlertLocation = (APTR)ctx->PC;
    iet->iet_AlertStack    = (APTR)ctx->FP;
    CopyMem(ctx, ETaskAlertData(iet), sizeof(struct ExceptionContext));

    if (!__atomic_compare_exchange_n(&StackProbeTask, &none, task, FALSE, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        return;

    StackProbeInt.is_Data = SysBase;
    Cause(&StackProbeInt);
}
