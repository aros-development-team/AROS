/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

        C:

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

    09.03.2009  OTigreat  41.2  Let's find Lab even after an empty line
    14.01.2000  SDuvan    41.1  Implemented

******************************************************************************/

#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include "dos_commanderrors.h"
#include <dos/stdio.h>

#include <aros/shcommands.h>

AROS_SH2(Skip, 41.2,
AROS_SHA(STRPTR, , LABEL,  , NULL),
AROS_SHA(BOOL,   , BACK, /S, FALSE))
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    BOOL                  labelFound = FALSE;


    if(cli == NULL || cli->cli_CurrentInput == cli->cli_StandardInput)
    {
	PrintFault(ERROR_SCRIPT_ONLY, "Skip");

	return RETURN_FAIL;
    }

    {
	char  buffer[256];
	int   a = 0;
	LONG  status;
	BOOL  quit = FALSE;

	SelectInput(cli->cli_CurrentInput);

	if (SHArg(BACK))
	{
	    Flush(Input());
	    Seek(Input(), 0, OFFSET_BEGINNING);
	}

	while (!quit)
	{
	    status = ReadItem(buffer, sizeof(buffer), NULL);

	    if (status == ITEM_ERROR)
	    {
		break;
	    }

	    if (status != ITEM_NOTHING) {
		switch (FindArg("LAB,ENDSKIP", buffer))
		{
		    case 0:
			if (SHArg(LABEL) != NULL)
			{
			    ReadItem(buffer, sizeof(buffer), NULL);

			    if (FindArg(SHArg(LABEL), buffer) == 0)
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
	    }

	    /* Skip to the next line */
	    do
	    {
	        a = FGetC(Input());
	    } while (a != '\n' && a != ENDSTREAMCH);
	}
    }

    if (!labelFound && SHArg(LABEL) != NULL)
    {
	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	PrintFault(ERROR_OBJECT_NOT_FOUND, "Skip");

	return RETURN_FAIL;
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
