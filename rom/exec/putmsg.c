/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Send a message to a port.
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <exec/ports.h>
#include <proto/exec.h>

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

    /*
	Messages may be sent from interrupts. Therefore the message list
	of the message port must be protected with Disable().
    */
    Disable();

    /* Set the node type to NT_MESSAGE == sent message. */
    message->mn_Node.ln_Type=NT_MESSAGE;

    /* Add it to the message list. */
    AddTail(&port->mp_MsgList,&message->mn_Node);

    if(port->mp_SigTask)
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
		/* Raise a software interrupt */
		Cause((struct Interrupt *)port->mp_SoftInt);
		break;

	    case PA_IGNORE:
		/* Do nothing. */
		break;
	}
    }

    /* All done. */
    Enable();
    AROS_LIBFUNC_EXIT
} /* PutMsg() */

