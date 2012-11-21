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

        AROS_LH1(IPTR, CliInitNewcli,

/*  SYNOPSIS */
        AROS_LHA(struct DosPacket *, dp, A0),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 155, Dos)

/*  FUNCTION

    Set up a process to be a shell using a startup packet.

    INPUTS

    packet  --  startup arguments that were passed to the shell
                If NULL, defaults will be used

    RESULT

    NOTES

    Called to initialize CLI private data structures, when
    the User Shell is in interactive mode.

    EXAMPLE

    BUGS

    SEE ALSO

    CliInitRun()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return internal_CliInitAny(dp, DOSBase);

    AROS_LIBFUNC_EXIT
} /* CliInitNewcli */
