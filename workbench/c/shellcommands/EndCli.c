/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Why CLI command
    Lang: English
*/

/******************************************************************************


    NAME

        Why

    SYNOPSIS

    LOCATION

        Workbench:C

    FUNCTION

        Print additional information why an operation failed. Ordinarily
	when a command fails a breif message is printed that typically
	includes the name of the command that failed but provides few
	details. Why fills in details related to the failed operation.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/


#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

#include "shcommands.h"

AROS_SH0(EndCli, 41.3)
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();

    if (cli)
    {
	if (!cli->cli_Interactive)
	    Close(cli->cli_CurrentInput);

	cli->cli_Background   = TRUE;
        cli->cli_CurrentInput = NULL;
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
