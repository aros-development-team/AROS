/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Run

    SYNOPSIS

        COMMAND/F

    LOCATION

        Workbench:C

    FUNCTION

        Run a program, that is start a program as a background process.
        That means it doesn't take over the parent shell.

    INPUTS

        COMMAND  --  the program to run together with its arguments

    RESULT

    NOTES

        To make it possible to close the current shell, redirect the output
        using
 
             Run >NIL: program arguments

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include "shcommands.h"

AROS_SH1(Run, 41.2,
AROS_SHA(STRPTR, ,COMMAND,/F,NULL))
{
    AROS_SHCOMMAND_INIT

    struct TagItem tags[] =
    {
	{ SYS_Input   , Open("CONSOLE:", MODE_OLDFILE)  	    	},
	{ SYS_Output  , Open("CONSOLE:", MODE_NEWFILE)  	    	},
	{ SYS_Asynch  , TRUE    	    	    	    	    	},
	{ NP_StackSize, Cli()->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT },
	{ TAG_DONE    , 0 	    	    	    	    	    	}
    };

    if (SystemTagList(SHArg(COMMAND), tags) == -1)
    {
	PrintFault(IoErr(), "Run");
	return RETURN_FAIL;
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}

