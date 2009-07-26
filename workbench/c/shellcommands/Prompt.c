/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

        C:

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

        dos.library/SetPrompt()

    INTERNALS

    HISTORY

        16-12-99   SDuvan   Implemented

******************************************************************************/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

#include <aros/shcommands.h>

AROS_SH1(Prompt,41.1,
AROS_SHA(STRPTR, ,PROMPT, , "%N.%S> "))
{
    AROS_SHCOMMAND_INIT


    if (SetPrompt(SHArg(PROMPT)) == DOSFALSE)
    {
	PrintFault(IoErr(), "Prompt");

	return RETURN_ERROR;
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}

