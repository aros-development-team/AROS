/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

/******************************************************************************

    NAME

        Skip

    SYNOPSIS

        LABEL, BACK/S

    LOCATION

        Workbench:C

    FUNCTION

        Skip commands in a script file until a certain label (declared with
	Lab) or an EndSkip command is reached.

    INPUTS

        LABEL  --  The label to skip to.  

	BACK   --  Specify this if the label appears before the Skip statement
	           in the script file.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

       Lab, EndSkip

    INTERNALS

    HISTORY

    14.01.2000  SDuvan   implemented

******************************************************************************/

#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos_commanderrors.h>
#include <dos/stdio.h>

enum { ARG_LABEL = 0, ARG_BACK };


int main(int argc, char **argv)
{
    struct RDArgs *rda;
    IPTR           args[2] = { NULL, FALSE };
    struct CommandLineInterface *cli = Cli();
    int            retval = RETURN_OK;
    BOOL           labelFound = FALSE;

    if(cli == NULL || cli->cli_CurrentInput == cli->cli_StandardInput)
    {
	PrintFault(ERROR_SCRIPT_ONLY, "Skip");
	return RETURN_FAIL;
    }

    rda = ReadArgs("LABEL,BACK/S", args, NULL);

    if(rda != NULL)
    {
	char  buffer[256];
	LONG  status;
	BOOL  quit = FALSE;

	SelectInput(cli->cli_CurrentInput);

	if((BOOL)args[ARG_BACK])
	{
	    Flush(Input());
	    Seek(Input(), 0, OFFSET_BEGINNING);
	}

	while(!quit)
	{
	    status = ReadItem(buffer, sizeof(buffer), NULL);
	    
	    if(status == ITEM_ERROR)
		break;

	    if(status == ITEM_NOTHING)
	    {
		if(FGetC(Input()) == ENDSTREAMCH)
		    break;
	    }	    

	    switch(FindArg("LAB,ENDSKIP", buffer))
	    {
	    case 0:
		if(args[ARG_LABEL] != NULL)
		{
		    ReadItem(buffer, sizeof(buffer), NULL);
		    
		    if(FindArg(args[ARG_LABEL], buffer) == 0)
		    {
			quit = TRUE;
			labelFound = TRUE;
		    }
		}
		break;

	    case 1:
		quit = TRUE;
		break;
	    }

	    /* Skip to the next line */
	    {
		char a;

		do {
		    a = FGetC(Input());
		} while (a != '\n' && a != ENDSTREAMCH);
	    }
	}
    }

    if(!labelFound && args[ARG_LABEL] != NULL)
    {
	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	PrintFault(ERROR_OBJECT_NOT_FOUND, "Skip");
    }

    FreeArgs(rda);

    return retval;
}
