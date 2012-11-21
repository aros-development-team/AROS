/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <dos/notify.h>
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH0I(struct DosPacket *, WaitPkt,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct DosLibrary *, DOSBase, 42, Dos)

/*  FUNCTION

    Wait for a packet to arrive at your process' pr_MsgPort. It will call
    pr_PktWait if such a function is installed.

    INPUTS

    RESULT

    The packet we received.

    NOTES

    The packet will be released from the port.

    This function should NOT be used. It's there only for AmigaOS
    compatibility.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);

    ASSERT_VALID_PROCESS(me);

    return internal_WaitPkt(&me->pr_MsgPort);

    AROS_LIBFUNC_EXIT
} /* WaitPkt */

struct DosPacket *internal_WaitPkt(struct MsgPort *msgPort)
{
    struct Message   *msg = NULL;
    struct Process   *me = (struct Process *)FindTask(NULL);

    if (__is_process(me))
    {
        /*
         * Call the packet wait function if the user has one installed.
         * Unfortunately, in case of IOFS the user gets something completely different than
         * a packet, but we cannot do anything about that...
         */
        if (me->pr_PktWait)
        {
            msg = AROS_UFC3(struct Message *, me->pr_PktWait,
                AROS_UFCA(APTR, me->pr_PktWait, A0),
                AROS_UFCA(struct MsgPort *, msgPort, A1),
                AROS_UFCA(struct ExecBase *, SysBase, A6));
        }
    }

    if (!msg)
    {
        /* Make sure we have a packet -- we may be woken up even if there is
           not a packet for us as SIGF_DOS may be used and we may have another
           message port that waits for packets, too. */
        while ((msg = GetMsg(msgPort)) == NULL)
        {
            Wait(1 << msgPort->mp_SigBit);
        }
    }

    D(bug("[DOS] WaitPkt(): got DOS packet 0x%p in message 0x%p\n", msg->mn_Node.ln_Name, msg));
    return (struct DosPacket *)msg->mn_Node.ln_Name;
}
