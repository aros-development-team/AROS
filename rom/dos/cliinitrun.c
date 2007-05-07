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

	AROS_LH1(IPTR, CliInitRun,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, A0),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 156, Dos)

/*  FUNCTION

    Set up a process to be a shell.

    INPUTS

    dp  --  startup arguments specified as a packet

    RESULT

    NOTES

    This function is obsolete for the same reasons as CliInitNewCli()
    and should not be used.

    EXAMPLE

    BUGS

    SEE ALSO

    CliInitNewCli()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
} /* CliInitRun */
