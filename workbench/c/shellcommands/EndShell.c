/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EndShell CLI command
    Lang: English
*/

/******************************************************************************


    NAME

        EndShell

    SYNOPSIS

    LOCATION

        C:

    FUNCTION

        Exits a Shell/CLI.

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

AROS_SH0(EndShell, 41.3)
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();


    if (cli)
    {
        struct FileHandle *fhin = BADDR(cli->cli_CurrentInput);
        struct FileHandle *fhout = BADDR(cli->cli_StandardOutput);

	cli->cli_Background = TRUE;

        fhin->fh_Pos  = fhin->fh_End + 1; /* Simulate an EOF */
        fhout->fh_Pos = fhout->fh_Buf; /* don't flush cli's standard output on close*/
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
