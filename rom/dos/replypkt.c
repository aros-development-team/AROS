/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: replypkt.c 30792 2009-03-07 22:40:04Z neil $

    Desc:
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH3(void, ReplyPkt,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, D1),
	AROS_LHA(SIPTR             , res1, D2),
	AROS_LHA(LONG              , res2, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 43, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);

    ASSERT_VALID_PROCESS(me);

    internal_ReplyPkt(dp, &me->pr_MsgPort, res1, res2);

    AROS_LIBFUNC_EXIT
} /* ReplyPkt */

void internal_ReplyPkt(struct DosPacket *dp, struct MsgPort *replyPort, SIPTR res1, LONG res2)
{
    struct MsgPort *mp;
    struct Message *mn;

    mp = dp->dp_Port;
    mn = dp->dp_Link;
    mn->mn_Node.ln_Name = (char*)dp;
    dp->dp_Port = replyPort;
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;

    /*
     * Can't use ReplyMsg() here because mn_ReplyPort seems to be not set.
     * Tested with SFS.
     */
    PutMsg(mp, mn);
}
