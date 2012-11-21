/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
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

        AROS_LH2(void, AbortPkt,

/*  SYNOPSIS */
        AROS_LHA(struct MsgPort   *, port, D1),
        AROS_LHA(struct DosPacket *, pkt, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 44, Dos)

/*  FUNCTION
        This function does currently nothing. You can use WaitForChar()
        to poll for characters from an interactive handler.

        The planned purpose of this function is:
        Tries to abort an asynchronous packet. There is no guarantee
        that this succeeds. You must wait with WaitPkt() for the packet
        to return before you can reuse or deallocate it.

    INPUTS
        port - The message port to where the packet was sent
        pkt  - The packet to be aborted

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SendPkt(), WaitForChar(), WaitPkt()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: implement this for real packets (is it possible at all) ? */

    AROS_LIBFUNC_EXIT
} /* AbortPkt */
