/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#include <dos/filesystem.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <aros/shcommands.h>

AROS_SH1(Run, 41.2,
AROS_SHA(STRPTR, ,COMMAND,/F,NULL))
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    BPTR cis = NULL, cos = NULL, ces = NULL;
    LONG CliNum;
    LONG stacksize;

    if (cli)
    {
	BPTR toclone, olddir;

	stacksize = cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;
	if (stacksize < AROS_STACKSIZE)
	    stacksize = AROS_STACKSIZE;

	if (IsInteractive(Input()))
	    toclone = Input();
	else
	    toclone = cli->cli_StandardInput;

	olddir = CurrentDir(toclone);
	cis = Open("", FMF_READ);
	CurrentDir(olddir);

	if (IsInteractive(Output()))
	    toclone = Output();
	else
	    toclone = cli->cli_StandardOutput;

	olddir = CurrentDir(toclone);
	cos = Open("", FMF_WRITE);
	CurrentDir(olddir);

	/* This is sort of a hack, needed because the original AmigaOS shell didn't allow
	   Error() redirection, so all the scripts written so far assume that only Input() and
	   Output() require to be redirected in order to not block the parent console */
        if (Error() != cli->cli_StandardError && IsInteractive(Error()))
	{
	    toclone = Error();

	    olddir = CurrentDir(toclone);
	    ces = Open("", FMF_WRITE);
	    CurrentDir(olddir);
	}
    }

    {
        struct TagItem tags[] =
        {
	    { SYS_Input,      (IPTR)cis     },
	    { SYS_Output,     (IPTR)cos     },
	    { SYS_Error,      (IPTR)ces     },
	    { SYS_Background, TRUE          },
	    { SYS_Asynch,     TRUE          },
	    { SYS_CliNumPtr,  (IPTR)&CliNum },
	    { SYS_UserShell,  TRUE          },
	    { NP_StackSize,   stacksize     },
	    { TAG_DONE,       0             }
        };


        if (SystemTagList(SHArg(COMMAND), tags) == -1)
        {
	    PrintFault(IoErr(), "Run");
	    Close(cis);
	    Close(cos);
	    Close(ces);

	    return RETURN_FAIL;
        }
    }

    {
        IPTR data[1] = { (IPTR)CliNum };
        VPrintf("[CLI %ld]\n", data);
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}

