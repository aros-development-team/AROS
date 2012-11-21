/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <exec/initializers.h>
#include <proto/utility.h>

#include <string.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3I(void, SendPkt,

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

    D(bug("[DOS] SendPkt(0x%p, 0x%p, 0x%p)\n", dp, port, replyport));

    internal_SendPkt(dp, port, replyport);

    AROS_LIBFUNC_EXIT
} /* SendPkt */

/* Also needed by Dos/DoPkt() */
void internal_SendPkt(struct DosPacket *dp, struct MsgPort *port, struct MsgPort *replyport)
{
    dp->dp_Port               = replyport;
    dp->dp_Link->mn_ReplyPort = replyport;

    PutMsg(port, dp->dp_Link);
}
