/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:15  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:53  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:57  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:08  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:18  digulla
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

    HISTORY

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
	}
    }

    /* All done */
    Enable();
    AROS_LIBFUNC_EXIT
}

