/*
    Copyright � 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#ifdef DEBUG
#undef DEBUG
#endif
#define  DEBUG 0
#include <aros/debug.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH7(SIPTR, DoPkt,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port, D1),
	AROS_LHA(LONG            , action, D2),
	AROS_LHA(SIPTR           , arg1, D3),
	AROS_LHA(SIPTR           , arg2, D4),
	AROS_LHA(SIPTR           , arg3, D5),
	AROS_LHA(SIPTR           , arg4, D6),
	AROS_LHA(SIPTR           , arg5, D7),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 40, Dos)

/*  FUNCTION

    Send a dos packet to a filesystem and wait for the action to complete.

    INPUTS

    RESULT

    NOTES

    Callable from a task.

    This function should NOT be used; it's only here for AmigaOS compatibility.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * First I create a regular dos packet and then let 
     * SendPkt rewrite it.
     */

    SIPTR res;
    struct Process   *me = (struct Process *)FindTask(NULL);
    struct DosPacket *dp = (struct DosPacket *)AllocDosObject(DOS_STDPKT,
							      NULL);
    struct MsgPort   *replyPort;
    
    BOOL i_am_process = TRUE;
    
    if (NULL == dp)
    {
	return FALSE;
    }
    
    D(bug("Allocated packet %p for action %ld\n", dp, action));

    if (__is_process(me))
    {
	replyPort = &me->pr_MsgPort;
    }
    else
    {
	/* Make sure that tasks can use DoPkt(). */
	replyPort = CreateMsgPort();

	if (NULL == replyPort)
	{
	    FreeDosObject(DOS_STDPKT, dp);
	    return FALSE;
	}

	i_am_process = FALSE;
    }
    
    dp->dp_Type = action;
    dp->dp_Arg1 = arg1;
    dp->dp_Arg2 = arg2;
    dp->dp_Arg3 = arg3;
    dp->dp_Arg4 = arg4;
    dp->dp_Arg5 = arg5;
    
    SendPkt(dp, port, replyPort);
    
    internal_WaitPkt(replyPort, DOSBase);
    
    SetIoErr(dp->dp_Res2);
    res = dp->dp_Res1;
    
    if (FALSE == i_am_process)
    {
	DeleteMsgPort(replyPort);
    }
    
    FreeDosObject(DOS_STDPKT, dp);
    
    return res;

    AROS_LIBFUNC_EXIT
} /* DoPkt */


