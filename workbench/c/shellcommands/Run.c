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
#include <dos/stdio.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <proto/alib.h>

#define DEBUG 0
#include <aros/debug.h>

#include <aros/shcommands.h>

#include <string.h>

AROS_SH2H(Run, 41.3,                  "Start a program as a background process\n",
AROS_SHAH(BOOL  , ,QUIET  ,/S,FALSE,"\tDon't print the background CLI's number"),
AROS_SHAH(STRPTR, ,COMMAND,/F,NULL ,  "The program (resp. script) to run (arguments allowed)\n") )
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR cis = BNULL, cos = BNULL, ces = BNULL;
    struct FileHandle *fh;
    LONG argsize;
    CONST_STRPTR argbuff;
    STRPTR command = (STRPTR)SHArg(COMMAND);
    LONG cmdsize = 0;

    if (!command)
        command = "";

    cis = Open("NIL:", MODE_OLDFILE);
    
    /* To support '+' style continuation, we're
     * going to need to be a little tricky. We
     * want to append *only* the buffered input
     * left in Input() after the implicit ReadArgs
     * to our command.
     *
     * First, let's see how much we have.
     */
    fh = BADDR(Input());
    argsize = (fh->fh_End > 0 && fh->fh_Pos > 0) ? (fh->fh_End - fh->fh_Pos) : 0;

    /* Good, there's some buffered input for us.
     * Append it to the command.
     */
    if (argsize > 0) {
        STRPTR tmp;
        cmdsize = strlen(command);

        argbuff = BADDR(fh->fh_Buf) + fh->fh_Pos;

        tmp = AllocMem(cmdsize+1+argsize+1, MEMF_ANY);
        if (tmp) {
            command = tmp;
            CopyMem(SHArg(COMMAND), command, cmdsize);
            command[cmdsize++]='\n';
            CopyMem(argbuff, &command[cmdsize], argsize);
            cmdsize += argsize;
            command[cmdsize++] = 0;
        } else {
            cmdsize = 0;
        }
    }

    cos = OpenFromLock(DupLockFromFH(Output()));

    /* All the 'noise' goes to cli_StandardError
     */
    if (!SHArg(QUIET)) {
        if (cli)
        {
            ces = cli->cli_StandardError;
        } else {
            ces = me->pr_CES;
        }
    }

    /* Use a duplicate of the CES lock
     */
    if (ces)
        ces = OpenFromLock(DupLockFromFH(ces));
    else
        ces = Open("NIL:", MODE_OLDFILE);

    if ( command[0] != 0)
    {
        struct TagItem tags[] =
        {
	    { SYS_ScriptInput, (IPTR)cis     },
	    { SYS_Input,       (IPTR)cis     },
	    { SYS_Output,      (IPTR)cos     },
	    { SYS_Error,       (IPTR)ces     },
	    { SYS_CliType,     (IPTR)CLI_RUN },
	    { TAG_DONE,        0             }
        };

        if ( SystemTagList((CONST_STRPTR)command,
                           tags                                ) == -1 )
        {
	    PrintFault(IoErr(), "Run");
	    Close(cis);
	    Close(cos);
	    Close(ces);
	    if (cmdsize > 0)
	        FreeMem(command, cmdsize);

	    return RETURN_FAIL;
        }
    }

    if (cmdsize > 0)
        FreeMem(command, cmdsize);

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
