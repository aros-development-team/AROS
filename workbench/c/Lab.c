/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

/******************************************************************************

    NAME

        Lab

    SYNOPSIS

    LOCATION

        Workbench:C

    FUNCTION

        Declares a label in a script file. This label may be referred to in a
	Skip command.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

       If NOT EXISTS S:User-Startup
           Skip NoUserSeq
       EndIf

       FailAt 20
       Execute S:User-Startup
       Quit

       Lab NoUserSeq
       Echo "No User-Startup found"

    BUGS

    SEE ALSO

       Skip

    INTERNALS

    HISTORY

    09.03.2000  SDuvan   implemented

******************************************************************************/

#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos_commanderrors.h>

int __nocommandline;

int main(void)
{
    struct CommandLineInterface *cli = Cli();

    if((cli != NULL) && (cli->cli_CurrentInput != cli->cli_StandardInput))
    {
	return RETURN_OK;	/* Normal operation: do nothing! */
    }
    else
    {
	PrintFault(ERROR_SCRIPT_ONLY, "Lab");
	return RETURN_ERROR;
    }
}
