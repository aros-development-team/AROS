/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: Prompt CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        Prompt

    SYNOPSIS

        PROMPT

    LOCATION

        Workbench:c

    FUNCTION

        Specify the prompt for the current shell.

    INPUTS

        PROMPT  --  The prompt to set as a string. The following commands
	            may be used in a printf kind of style.

		    N  --  cli number
		    S  --  name of the current directory
		    R  --  return code of the last operation

		    If no prompt is specified "%N.%S> " is used as default.

    RESULT

        Standard DOS return codes.

    NOTES

    EXAMPLE

        Prompt "Oepir Risti.%N> " gives:

	Oepir Risti.10>      (if the CLI number was 10)

    BUGS

    SEE ALSO

        SetPrompt()

    INTERNALS

    HISTORY

        16-12-99   SDuvan   Implemented

******************************************************************************/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

int main(int argc, char *argv[])
{
    struct RDArgs *rda;
    ULONG          retval = RETURN_OK;
    STRPTR         promptString = "%N.%S> ";
    IPTR           args[1] = { NULL };
    
    rda = ReadArgs("PROMPT", args, NULL);
    
    if(rda == NULL)
	return RETURN_ERROR;
    
    if(args[0] != NULL)
	promptString = (STRPTR)args[0];
    
    if(SetPrompt(promptString) == DOSFALSE)
    {
	PrintFault(IoErr(), "Prompt");
	retval = RETURN_ERROR;
    }

    FreeArgs(rda);
    
    return retval;
}

