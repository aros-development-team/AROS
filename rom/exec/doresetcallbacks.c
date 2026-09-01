/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Execute installed reset handlers.
*/

#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <devices/timer.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "exec_debug.h"

/* How long one reset handler may take before the chain moves on without
   it (seconds). Generous - a handler doing real work (unloading device
   firmware, flushing) may legitimately sleep - but bounded, so a crashed
   or hung handler can never leave the machine sitting half shut down. */
#define RESETHANDLER_TIMEOUT    10

/* Serialization state for the non-supervisor path lives in ExecBase:
   the launcher waits there until the running handler's task reports
   completion. A kickstart module may not have .bss, so this cannot be
   file-scope state. */

static void ResetCallbackHandler(struct ExecBase *SysBase, struct Interrupt *callback)
{
    DSHUTDOWN("Calling handler: %d '%s'", callback->is_Node.ln_Pri, callback->is_Node.ln_Name);
    AROS_INTC1(callback->is_Code, callback->is_Data);
    /* Wake the launcher - nothing may proceed, the reset performer
       included, until this handler is done */
    if (PrivExecBase(SysBase)->ResetCallbackWaiter)
        Signal(PrivExecBase(SysBase)->ResetCallbackWaiter,
               PrivExecBase(SysBase)->ResetCallbackSignal);
}

/*
   This function executes all installed reset handlers in sequence.
   It stores the supplied shutdown action type (SD_ACTION_#?) in the
   ln_Type field of each reset interrupt structure before invoking it.
   Typically, this information is used by system-level reset handlers
   (EFI, ACPI, etc.), but not by peripheral-device handlers (USB HCs,
   NICs, etc.). The ln_Type field also encodes whether the code is
   executing in supervisor mode.

   For improved safety, all callbacks are executed in a Disable()d state.
   This function itself does not need to call Enable().

   NOTE: This function can fail if any installed reset handler crashes
   or hangs. In such cases, the shutdown sequence may not complete,
   leaving the system in an undefined or partially reset state. This
   behavior needs to be corrected so that failure in one handler cannot
   prevent the rest of the shutdown process from completing.
*/

void Exec_DoResetCallbacks(struct IntExecBase *IntSysBase, UBYTE action)
{
    struct Task *shutdownTask = FindTask(NULL);
    struct Interrupt *i, *tmp;
    int issuper;
    BYTE sigBit = -1;
    struct MsgPort *tport = NULL;
    struct timerequest *treq = NULL;
    BOOL timedout = FALSE;

    DSHUTDOWN("Executing Reset Callbacks");

    BYTE prio = SetTaskPri(shutdownTask, 120);
    issuper = KrnIsSuper();
    if (issuper)
        Disable();
    else
    {
        /* The handlers run from support tasks; each launch below waits
           for the task to finish so the chain stays strictly in
           sequence even when a handler sleeps. AllocSignal() returns a
           bit number - Wait() needs the mask. */
        IntSysBase->ResetCallbackWaiter = shutdownTask;
        sigBit = AllocSignal(-1);
        IntSysBase->ResetCallbackSignal = (sigBit == -1) ? SIGF_SINGLE : (1UL << sigBit);

        /* A timeout source, so no handler can stall the chain forever -
           it must reach a reset performer no matter what. Without
           timer.device the waits below fall back to unbounded. */
        if ((tport = CreateMsgPort()) != NULL)
        {
            if ((treq = (struct timerequest *)CreateIORequest(tport, sizeof(struct timerequest))) != NULL)
            {
                if (OpenDevice("timer.device", UNIT_VBLANK, &treq->tr_node, 0) != 0)
                {
                    DeleteIORequest(&treq->tr_node);
                    treq = NULL;
                }
            }
            if (treq == NULL)
            {
                DeleteMsgPort(tport);
                tport = NULL;
            }
        }
    }

    ForeachNodeSafe(&IntSysBase->ResetHandlers, i, tmp) {
        i->is_Node.ln_Type = action;
        if (issuper) {
            i->is_Node.ln_Type |= 0x80; /* Set the "supervisor" flag */
            ResetCallbackHandler(SysBase, i);
        } else {
            struct Task *handlerTask;

            /* perform the operation from a support task,
             * so that crashes are trapped and dont stop the process
             */
            SetSignal(0, IntSysBase->ResetCallbackSignal);
            handlerTask = NewCreateTask(TASKTAG_NAME    , "ResetCallbackHandler",
                       TASKTAG_PRI        , 127,
                       TASKTAG_PC         , ResetCallbackHandler,
                       TASKTAG_ARG1       , SysBase,
                       TASKTAG_ARG2       , i,
                       TAG_DONE);
            /* Strictly one handler at a time: one that sleeps must
             * finish before the next is launched, or before the chain
             * is declared over. But never forever - a handler that
             * crashes or hangs is abandoned after the timeout so the
             * chain still reaches a reset performer.
             */
            if (handlerTask)
            {
                if (treq)
                {
                    ULONG tsig = 1UL << tport->mp_SigBit;
                    ULONG sigs;

                    treq->tr_node.io_Command = TR_ADDREQUEST;
                    treq->tr_time.tv_secs = RESETHANDLER_TIMEOUT;
                    treq->tr_time.tv_micro = 0;
                    SendIO(&treq->tr_node);
                    sigs = Wait(IntSysBase->ResetCallbackSignal | tsig);
                    if (sigs & IntSysBase->ResetCallbackSignal)
                    {
                        if (!CheckIO(&treq->tr_node))
                            AbortIO(&treq->tr_node);
                        WaitIO(&treq->tr_node);
                        SetSignal(0, tsig);
                    }
                    else
                    {
                        WaitIO(&treq->tr_node);
                        timedout = TRUE;
                        bug("[exec] reset callback '%s' did not complete - moving on\n",
                            i->is_Node.ln_Name ? i->is_Node.ln_Name : "(unnamed)");
                    }
                }
                else
                    Wait(IntSysBase->ResetCallbackSignal);
            }
        }
    }

    if (!issuper)
    {
        if (treq)
        {
            CloseDevice(&treq->tr_node);
            DeleteIORequest(&treq->tr_node);
            DeleteMsgPort(tport);
        }
        /* A timed-out handler may still signal one day - keep the bit
           and the waiter valid rather than have that land on a
           recycled signal */
        if (!timedout)
        {
            IntSysBase->ResetCallbackWaiter = NULL;
            if (sigBit != -1)
                FreeSignal(sigBit);
        }
    }

    /* We shouldnt reach here ..
     * but just incase restore the tasks priority.
     */
    SetTaskPri(shutdownTask, prio);
}
