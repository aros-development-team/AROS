/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

        C:

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
#include "dos_commanderrors.h"

#include <aros/shcommands.h>

AROS_SH1(Quit, 41.1,
AROS_SHA(LONG *, ,RC,/N,NULL))
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    int retval = RETURN_OK;


    if(cli && !cli->cli_Interactive)
    {
        struct FileHandle *fh = BADDR(cli->cli_CurrentInput);

        fh->fh_Pos = fh->fh_End + 1; /* Simulate an EOF */

	if(SHArg(RC) != NULL)
	    retval = (int)*SHArg(RC);

    }
    else
    {
	PrintFault(ERROR_SCRIPT_ONLY, "Quit");
	retval = RETURN_FAIL;
    }

    return retval;

    AROS_SHCOMMAND_EXIT
}
