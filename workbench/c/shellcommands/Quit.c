/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

/******************************************************************************

    NAME

        Quit

    SYNOPSIS

        RC/N

    LOCATION

        Workbench:C

    FUNCTION

    INPUTS

        RC   --  the return code

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    21.01.2000  SDuvan   implemented

******************************************************************************/

#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos_commanderrors.h>

#include "shcommands.h"

AROS_SH1(Quit, 41.1,
AROS_SHA(LONG *, ,RC,/N,NULL))
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    int retval = RETURN_OK;

    if(cli && !cli->cli_Interactive)
    {

	if(SHArg(RC) != NULL)
	    retval = (int)*SHArg(RC);

	Close(cli->cli_CurrentInput);
	cli->cli_CurrentInput = NULL;
    }
    else
    {
	PrintFault(ERROR_SCRIPT_ONLY, "Quit");
	retval = RETURN_FAIL;
    }

    return retval;

    AROS_SHCOMMAND_EXIT
}
