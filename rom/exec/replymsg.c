/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:16  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <exec/ports.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, ReplyMsg,

/*  SYNOPSIS */
	__AROS_LA(struct Message *, message, A1),

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
    __AROS_FUNC_INIT

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
    __AROS_FUNC_EXIT
}

