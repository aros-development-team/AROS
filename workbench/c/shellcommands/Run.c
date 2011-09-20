/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Run

    SYNOPSIS

        QUIET/S,COMMAND/F

    LOCATION

        C:

    FUNCTION

        Run a program, that is start a program as a background process.
        That means it doesn't take over the parent shell.

    INPUTS

        QUIET    --  avoids printing of the background CLI's number 

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
#include <dos/cliinit.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <proto/alib.h>

#define DEBUG 0
#include <aros/debug.h>

#include <aros/shcommands.h>

AROS_SH2H(Run, 41.3,                "Start a program as a background process",
AROS_SHAH(BOOL  , ,QUIET  ,/S,FALSE,"\tDon't print the background CLI's number"),
AROS_SHAH(STRPTR, ,COMMAND,/F,NULL ,"The program (resp. script) to run (arguments\n"
                                    "\t\t\t\tallowed)") )
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    BPTR cis = BNULL, cos = BNULL, ces = BNULL;

    cis = Open("NIL:", MODE_OLDFILE);
    cos = OpenFromLock(DupLockFromFH(Output()));

    /* All the 'noise' goes to cli_StandardError
     */
    if (!SHArg(QUIET)) {
        if (cli)
        {
            ces = cli->cli_StandardError;
        } else {
            struct Process *me = (struct Process *)FindTask(NULL);

            ces = me->pr_CES;
        }
    }

    /* Use a duplicate of the CES lock
     */
    if (ces)
        ces = OpenFromLock(DupLockFromFH(ces));
    else
        ces = Open("NIL:", MODE_OLDFILE);

    if ( SHArg(COMMAND) )
    {
        struct TagItem tags[] =
        {
	    { SYS_Input,       (IPTR)cis     },
	    { SYS_Output,      (IPTR)cos     },
	    { SYS_Error,       (IPTR)ces     },
	    { SYS_CliType,     (IPTR)CLI_RUN },
	    { TAG_DONE,        0             }
        };

        if ( SystemTagList((CONST_STRPTR) SHArg(COMMAND)       ,
                           tags                                ) == -1 )
        {
	    PrintFault(IoErr(), "Run");
	    Close(cis);
	    Close(cos);
	    Close(ces);

	    return RETURN_FAIL;
        }
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
