/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Send a message to a port.
    Lang: english
*/

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
	port	- Pointer to messageport.
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

    /* FASTCALL is a special case that operates without the message list
     * locked, so we have to check it before anything */
    if (port->mp_Flags & PA_FASTCALL)
    {
        FastPutMsg(port, message, SysBase);
        return;
    }

    /*
	Messages may be sent from interrupts. Therefore the message list
	of the message port must be protected with Disable().
    */
    Disable();

    /* Set the node type to NT_MESSAGE == sent message. */
    message->mn_Node.ln_Type=NT_MESSAGE;

    InternalPutMsg(port, message, SysBase);

    /* All done. */
    Enable();
    AROS_LIBFUNC_EXIT
} /* PutMsg() */

void FastPutMsg(struct MsgPort *port, struct Message *message, struct ExecBase *SysBase)
{
    if (port->mp_SoftInt == NULL || ((struct Interrupt *) port->mp_SoftInt)->is_Code == NULL)
        return;

    ASSERT_VALID_PTR(port->mp_SoftInt);
    ASSERT_VALID_PTR(((struct Interrupt *) port->mp_SoftInt)->is_Code);

    /* call the "interrupt" with the message as an argument */
    AROS_UFC4NR(void, ((struct Interrupt *) port->mp_SoftInt)->is_Code,
             AROS_UFCA(APTR, ((struct Interrupt *) port->mp_SoftInt)->is_Data, A1),
	         AROS_UFCA(ULONG_FUNC, (ULONG_FUNC)((struct Interrupt *) port->mp_SoftInt)->is_Code, A5),
             AROS_UFCA(struct Message *,  message, D0),
             AROS_UFCA(struct ExecBase *, SysBase, A6));
}

void InternalPutMsg(struct MsgPort *port, struct Message *message, struct ExecBase *SysBase)
{
    /* Add it to the message list. */
    AddTail(&port->mp_MsgList,&message->mn_Node);

    if (port->mp_SigTask)
    {
	ASSERT_VALID_PTR(port->mp_SigTask);

	/* And trigger the action. */
	switch(port->mp_Flags & PF_ACTION)
	{
	    case PA_SIGNAL:
		/* Send the signal */
		Signal((struct Task *)port->mp_SigTask,1<<port->mp_SigBit);
		break;

	    case PA_SOFTINT:
	    	D(bug("PutMsg: PA_SOFTINT, port 0x%p, msg 0x%p, int %s\n", port, message, ((struct Interrupt *)port->mp_SoftInt)->is_Node.ln_Name));

		/* Raise a software interrupt */
		Cause((struct Interrupt *)port->mp_SoftInt);
		break;

	    case PA_IGNORE:
		/* Do nothing. */
		break;

            case PA_CALL:
                /* Call the function in mp_SigTask. */
                AROS_UFC2NR(void, port->mp_SigTask,
                    AROS_UFCA(struct MsgPort *,  port,    D0),
                    AROS_UFCA(struct ExecBase *, SysBase, A6));
                break;
	}
    }
}
