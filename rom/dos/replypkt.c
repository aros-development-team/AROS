/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id: replypkt.c 30792 2009-03-07 22:40:04Z neil $

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH3(void, ReplyPkt,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, D1),
	AROS_LHA(LONG              , res1, D2),
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

    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;
    dp->dp_Port = &me->pr_MsgPort;
    ReplyMsg(dp->dp_Link);

    AROS_LIBFUNC_EXIT
} /* ReplyPkt */
