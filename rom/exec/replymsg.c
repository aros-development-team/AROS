/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Reply a message
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/ports.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

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

    /* handle FASTCALL before doing locking or anything else. yes, there's a
     * potential race here if some task was to change mn_ReplyPort before/as
     * we read it. thats why we fetch it again further down, after Disable().
     * all bets are of when using FASTCALL */
    port = message->mn_ReplyPort;

    if (port != NULL && port->mp_Flags & PA_FASTCALL)
    {
        FastPutMsg(port, message, SysBase);
        return;
    }

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

	InternalPutMsg(port, message, SysBase);
    }

    /* All done */
    Enable();
    AROS_LIBFUNC_EXIT
} /* ReplyMsg() */

