/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH2(void, AbortPkt,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort   *, port, D1),
	AROS_LHA(struct DosPacket *, pkt, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 44, Dos)

/*  FUNCTION
	Tries to abort an asynchronous packet. There is no guarantee
	that this succeeds. You must wait for the packet to return
	before you can reuse or deallocate it.

    INPUTS
	port - the message port to where the packet was sent
	pkt  - the packet to be aborted

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (NULL != (struct IORequest *)pkt->dp_Arg7) {
        if (0 == AbortIO((struct IORequest*)pkt->dp_Arg7)) 
        {
#warning Still have to decide where to finally free the IORequest structure if this goes well
            FreeMem((APTR)pkt->dp_Arg7, sizeof(struct IOFileSys));
            pkt->dp_Arg7 = NULL;
        }
    }
    AROS_LIBFUNC_EXIT
} /* AbortPkt */
