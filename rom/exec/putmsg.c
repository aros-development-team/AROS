/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.8  1997/02/13 23:49:14  ldp
    Added extra check: take no signalling action if mp_SigTask==0. This fixes
    streams of Enforcer hits coming from input.device (does input.device make
    assumptions about ReplyMsg()/PutMsg() internal functioning?

    Revision 1.7  1997/01/01 03:46:13  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:50  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:54  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:05  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:15  digulla
    Added standard header for all files

    Desc:
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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

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
	/* And trigger the action. */
	switch(port->mp_Flags&PF_ACTION)
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
}

