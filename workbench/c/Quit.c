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

int main(int argc, char **argv)
{
    struct RDArgs *rda;
    struct CommandLineInterface *cli = Cli();
    int            retval = RETURN_OK;
    IPTR           args[] = { NULL };

    rda = ReadArgs("RC/N", args, NULL);
    
    if(rda != NULL)
    {
	if(cli != NULL && cli->cli_CurrentInput != cli->cli_StandardInput)
	{
	    if(args[0] != NULL)
		retval = (int)*(LONG *)args[0];

	    /* Make sure the script reaches EOF */
	    Seek(cli->cli_CurrentInput, 0, OFFSET_END);
	}
	else
	{
	    PrintFault(ERROR_SCRIPT_ONLY, "Quit");
	    retval = RETURN_FAIL;
	}
    }

    FreeArgs(rda);

    return retval;
}
