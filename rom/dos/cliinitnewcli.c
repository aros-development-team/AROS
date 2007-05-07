/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH1(IPTR, CliInitNewcli,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, A0),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 155, Dos)

/*  FUNCTION

    Set up a process to be a shell using a startup packet.

    INPUTS

    packet  --  startup arguments passed as a packet

    RESULT

    NOTES

    This function is obsolete as AROS don't use packets. There is no need
    for this function as functionality is added in other places to deal
    with things taken care of by this function. Furthermore, the Amiga
    startup packet interface was a pile of crap.

    EXAMPLE

    BUGS

    SEE ALSO

    CliInitRun()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
} /* CliInitNewcli */
