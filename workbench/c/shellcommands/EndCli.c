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


#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include "shcommands.h"

AROS_SH0(EndCli, 41.3)
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();

    if (cli)
    {
        struct FileHandle *fh = BADDR(cli->cli_CurrentInput);
	cli->cli_Background   = TRUE;

        fh->fh_Pos = fh->fh_End + 1; /* Simulate an EOF */
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
