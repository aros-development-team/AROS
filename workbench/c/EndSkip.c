/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

/******************************************************************************

    NAME

        EndSkip

    SYNOPSIS

    LOCATION

        Workbench:C

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
#include <dos_commanderrors.h>


int __nocommandline = 1;

int main(void)
{
    struct CommandLineInterface *cli = Cli();

    if((cli != NULL) && (cli->cli_CurrentInput != cli->cli_StandardInput))
    {
	return RETURN_OK;	/* Normal operation: do nothing! */
    }
    else
    {
	PrintFault(ERROR_SCRIPT_ONLY, "EndSkip");
	return RETURN_ERROR;
    }
}
