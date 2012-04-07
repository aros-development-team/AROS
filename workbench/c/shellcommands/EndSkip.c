/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

/******************************************************************************

    NAME

        EndSkip

    SYNOPSIS

    LOCATION

        C:

    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

        This command is quite similar to EndIf...

    HISTORY

    14.01.2000  SDuvan   implemented

******************************************************************************/

#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include "dos_commanderrors.h"

#include <aros/shcommands.h>

AROS_SH0(EndSkip,41.1)
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();


    if ((cli != NULL) && (cli->cli_CurrentInput != cli->cli_StandardInput))
    {
	return RETURN_OK;	/* Normal operation: do nothing! */
    }
    else
    {
	PrintFault(ERROR_SCRIPT_ONLY, "EndSkip");

	return RETURN_ERROR;
    }

    AROS_SHCOMMAND_EXIT
}
