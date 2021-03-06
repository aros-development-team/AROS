/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/

/******************************************************************************

    NAME

        EndIf

    SYNOPSIS

    LOCATION

        C:

    FUNCTION

        Ends an If block. If the condition of the If command is false,
        execution will skip to the corresponding EndIf command, in case there
        isn't an Else command present.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        If, Else

    INTERNALS

    HISTORY

    10.01.2000  SDuvan   implemented

******************************************************************************/

#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include "dos_commanderrors.h"

#include <aros/shcommands.h>

AROS_SH0(EndIf,41.1)
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();


    if ((cli != NULL) && (cli->cli_CurrentInput != cli->cli_StandardInput))
    {
        return RETURN_OK;       /* Normal operation: do nothing! */
    }
    else
    {
        PrintFault(ERROR_SCRIPT_ONLY, "EndIf");

        return RETURN_ERROR;
    }

    AROS_SHCOMMAND_EXIT
}
