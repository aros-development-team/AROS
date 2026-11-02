/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc: Send a message to a port.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <exec/ports.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

/*****************************************************************************

    NAME */

        AROS_LH2(void, PutMsg,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort *, port,    A0),
        AROS_LHA(struct Message *, message, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 61, Exec)

/*  FUNCTION
        Sends a message to a given message port. Messages are not copied
        from one task to another but must lie in shared memory instead.
        Therefore the owner of the message may generally not reuse it before
        it is returned. But this depends on the two tasks sharing the message.

    INPUTS
        port    - Pointer to messageport.
        message - Pointer to message.

    RESULT

    NOTES
        It is legal to send a message from within interrupts.

        Messages may either trigger a signal at the owner of the messageport
        or raise a software interrupt, depending on port->mp_Flags&PF_ACTION.

    EXAMPLE

    BUGS

    SEE ALSO
        WaitPort(), GetMsg()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    ASSERT_VALID_PTR(message);
    ASSERT_VALID_PTR(port);

    /* NT_MESSAGE == sent message; published once the message is on the list */
    InternalPutMsgType(port, message, NT_MESSAGE, SysBase);

    AROS_LIBFUNC_EXIT
} /* PutMsg() */

/*
 * Add a message to the port's list and wake the owner.
 *
 * Messages may be sent from interrupts, so the list is protected with
 * Disable() for the local core and a spinlock on SMP. Two SMP rules
 * (heavy4c-04, 2026-11-01: input.device faulted in AddTail on a port the
 * waiter had already freed):
 *  - the node type (NT_MESSAGE / NT_REPLYMSG) is published AFTER the node
 *    is linked, under the lock. WaitIO()/CheckIO() test ln_Type without
 *    the lock; a waiter woken early (DoSyncAction shares its port's signal
 *    with the "done" signal) must not see NT_REPLYMSG while the replier is
 *    still between the store and the AddTail - it would Remove() a node
 *    that is not on any list and free the port under the replier.
 *  - everything the signalling needs is read under the lock: once the
 *    message is visible the receiver may run, consume it and free the port.
 * Type 0 leaves ln_Type alone (remtask.c / service.c rely on that).
 */
void InternalPutMsgType(struct MsgPort *port, struct Message *message, UBYTE type, struct ExecBase *SysBase)
{
    UBYTE flags;
    BYTE  sigbit;
    APTR  sigtask;

    D(bug("[EXEC] PutMsg: Port @ 0x%p, Msg @ 0x%p\n", port, message);)

    Disable();
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_LOCK(&port->mp_SpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
    flags   = port->mp_Flags;
    sigbit  = port->mp_SigBit;
    sigtask = port->mp_SigTask;
    AddTail(&port->mp_MsgList, &message->mn_Node);
    if (type)
        __atomic_store_n(&message->mn_Node.ln_Type, type, __ATOMIC_RELEASE);
    D(bug("[EXEC] PutMsg: Port MsgList->lh_TailPred =  0x%p\n", port->mp_MsgList.lh_TailPred);)
#if defined(__AROSEXEC_SMP__)
    EXEC_SPINLOCK_UNLOCK(&port->mp_SpinLock);
#endif
    Enable();

    if (sigtask)
    {
        ASSERT_VALID_PTR(sigtask);

        /* And trigger the action. */
        switch(flags & PF_ACTION)
        {
            case PA_SIGNAL:
                D(bug("[EXEC] PutMsg: PA_SIGNAL -> Task 0x%p, Signal %08x\n", sigtask, (1 << sigbit));)

                /* Send the signal */
                Signal((struct Task *)sigtask, (1 << sigbit));
                break;

            case PA_SOFTINT:
                D(bug("[EXEC] PutMsg: PA_SOFTINT -> Int %s\n", ((struct Interrupt *)sigtask)->is_Node.ln_Name);)

                /* Raise a software interrupt */
                Cause((struct Interrupt *)sigtask);
                break;

            case PA_IGNORE:
                /* Do nothing. */
                break;

            case PA_CALL:
                D(bug("[EXEC] PutMsg: PA_CALL -> Func @ 0x%p\n", sigtask, port);)

#if defined(__AROSEXEC_SMP__)
                //TODO! - the called function must be SMP safe.
#endif
                /* Call the function in mp_SigTask. */
                AROS_UFC2NR(void, sigtask,
                    AROS_UFCA(struct MsgPort *,  port,    D0),
                    AROS_UFCA(struct ExecBase *, SysBase, A6));
                break;
        }
    }
}

void InternalPutMsg(struct MsgPort *port, struct Message *message, struct ExecBase *SysBase)
{
    InternalPutMsgType(port, message, 0, SysBase);
}
