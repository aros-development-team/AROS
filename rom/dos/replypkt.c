/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

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
      This function is mega-deprecated. Since all AROS drivers
      use IORequests to perform operations the driver will most
      probably not ever use or look at a packet. Also there might 
      not even be a packet at all except for if SendPkt was used.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);

    /* What should we do with res1? This function is not called by any
       AROS functions, so I think we don't have to care. */
    ((struct IOFileSys *)dp->dp_Arg7)->io_DosError = res2;

    /*
     * Just to be correct I write the res1/2 also into the packet
     */
    dp->dp_Res1 = res1;
    dp->dp_Res2 = res2;

    dp->dp_Port = &me->pr_MsgPort;
    ((struct IOFileSys *)dp->dp_Arg7)->IOFS.io_Message.mn_ReplyPort = &me->pr_MsgPort;


    /* I just love this casting... */
    ReplyMsg(&((struct IOFileSys *)dp->dp_Arg7)->IOFS.io_Message);

    AROS_LIBFUNC_EXIT
} /* ReplyPkt */
