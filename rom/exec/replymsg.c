/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Reply a message
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <exec/ports.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, ReplyMsg,

/*  SYNOPSIS */
	AROS_LHA(struct Message *, message, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 63, Exec)

/*  FUNCTION
	Send a message back to where it came from. It's generally not
	wise to access the fields of a message after it has been replied.

    INPUTS
	message - a message got with GetMsg().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	WaitPort(), GetMsg(), PutMsg()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MsgPort *port;

    /* Protect the message against access by other tasks. */
    Disable();

    /* Get replyport */
    port=message->mn_ReplyPort;

    /* Not set? Only mark the message as no longer sent. */
    if(port==NULL)
	message->mn_Node.ln_Type=NT_FREEMSG;
    else
    {
	/* Mark the message as replied */
	message->mn_Node.ln_Type=NT_REPLYMSG;

	/* Add it to the replyport's list */
	AddTail(&port->mp_MsgList,&message->mn_Node);

	if(port->mp_SigTask)
	{
	    /* And trigger the arrival action. */
	    switch(port->mp_Flags&PF_ACTION)
	    {
		case PA_SIGNAL:
		    /* Send a signal */
		    Signal((struct Task *)port->mp_SigTask,1<<port->mp_SigBit);
		    break;

		case PA_SOFTINT:
		    /* Raise a software interrupt */
		    Cause((struct Interrupt *)port->mp_SoftInt);
		    break;

		case PA_IGNORE:
		    /* Do nothing */
		    break;

                case PA_CALL:
                    /* Call the function in mp_SigTask. */
                    AROS_UFC1(void, port->mp_SigTask,
                        AROS_UFCA(struct ExecBase *, SysBase, A6));
                    break;
	    }
	}
    }

    /* All done */
    Enable();
    AROS_LIBFUNC_EXIT
} /* ReplyMsg() */

