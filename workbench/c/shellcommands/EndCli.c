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

#include <aros/shcommands.h>

AROS_SH0(EndCli, 41.3)
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();

    if (cli)
    {
        struct FileHandle *fhin = BADDR(cli->cli_CurrentInput);
        struct FileHandle *fhout = BADDR(cli->cli_StandardOutput);
	cli->cli_Background   = TRUE;

        fhin->fh_Pos  = fhin->fh_End + 1; /* Simulate an EOF */
        fhout->fh_Pos = fhout->fh_Buf; /* don't flush cli's standard output on close*/
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
