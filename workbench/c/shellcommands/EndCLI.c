/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EndCLI CLI command
    Lang: English
*/

/******************************************************************************


    NAME

        EndCLI

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

AROS_SH0(EndCLI, 41.3)
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();


    if (cli)
    {
        struct FileHandle *fhin = BADDR(cli->cli_StandardInput);

        if (cli->cli_CurrentInput && cli->cli_CurrentInput != cli->cli_StandardInput) {
            Close(cli->cli_CurrentInput);
            cli->cli_CurrentInput = cli->cli_StandardInput;
        }

	cli->cli_Background = DOSTRUE;

        fhin->fh_End = 0; /* Simulate an EOF */

        if (cli->cli_Interactive)
            Printf("Task %ld ending\n", ((struct Process *)FindTask(NULL))->pr_TaskNum);
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
