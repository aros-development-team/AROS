/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "dos_intern.h"
#include "dos_newcliproc.h"

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

    Called to initialize CLI private data structures, when
    the User Shell is not interactive.

    EXAMPLE

    BUGS

    SEE ALSO

    CliInitNewcli()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return internal_CliInitAny(dp, DOSBase);

    AROS_LIBFUNC_EXIT
} /* CliInitRun */
