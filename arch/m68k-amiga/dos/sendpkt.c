/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#define DEBUG 0
#include <aros/debug.h>
#include "dos_intern.h"
#include <dos/dosextens.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <exec/initializers.h>
#include <string.h>
#include <proto/utility.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(void, SendPkt,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, D1),
	AROS_LHA(struct MsgPort   *, port, D2),
	AROS_LHA(struct MsgPort   *, replyport, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 41, Dos)

/*  FUNCTION

    Send a packet to a handler without waiting for the result. The packet will
    be returned to 'replyport'.

    INPUTS

    packet     --  the (initialized) packet to send
    port       --  the MsgPort to send the packet to
    replyport  --  the MsgPort to which the packet will be replied

    RESULT

    This function is callable from a task.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DoPkt(), WaitPkt(), AbortPkt()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

        D(bug("[DOS] SendPkt(): pkt = $%lx, port = $%lx, replyport = $%lx\n",
		      dp, port, replyport));

        dp->dp_Port = replyport;
        dp->dp_Link->mn_ReplyPort = replyport;

        PutMsg(port, dp->dp_Link);

 
    AROS_LIBFUNC_EXIT
} /* SendPkt */

