/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id: waitpkt.c 31121 2009-04-17 08:38:45Z sonic $

    Desc:
    Lang: English
*/

#define  DEBUG 0
#include <aros/debug.h>

#include "dos_intern.h"
#include <dos/filesystem.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH0(struct DosPacket *, WaitPkt,

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

    struct Process   *me = (struct Process *)FindTask(NULL);
    return internal_WaitPkt(&me->pr_MsgPort, DOSBase);

    AROS_LIBFUNC_EXIT
} /* WaitPkt */

struct DosPacket *internal_WaitPkt(struct MsgPort *msgPort,
				   struct DosLibrary *DOSBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct Message *msg = NULL;
    struct MsgPort *mp = NULL;
    struct DosPacket *dp = NULL;

    if (__is_process(me))
    {
        mp = &me->pr_MsgPort;
	/* Call the packet wait function if the user has one installed. */
	if (me->pr_PktWait)
	{
	    msg = AROS_UFC3(struct Message *, me->pr_PktWait,
		AROS_UFCA(APTR, me->pr_PktWait, A0),
		AROS_UFCA(struct MsgPort *, mp, A1),
		AROS_UFCA(struct ExecBase *, SysBase, A6));
	}
    }

    if (!msg && mp) {
        /* Make sure we have a packet -- we may be woken up even if there is
           not a packet for us as SIGF_DOS may be used and we may have another
           message port that waits for packets, too. */
        while ((msg = GetMsg(mp)) == NULL)
        {
	    Wait(1 << mp->mp_SigBit);
        }
    }
    
    dp = (struct DosPacket*)msg->mn_Node.ln_Name;

    return dp;

}

