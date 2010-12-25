/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Run

    SYNOPSIS

        EXECUTE/S,QUIET/S,COMMAND/F

    LOCATION

        C:

    FUNCTION

        Run a program, that is start a program as a background process.
        That means it doesn't take over the parent shell.

    INPUTS

        EXECUTE  --  allows a script to be executed in the background

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
#include <dos/filesystem.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <proto/alib.h>

#define DEBUG 0
#include <aros/debug.h>

#include <aros/shcommands.h>

static BPTR duphandle(struct DosLibrary * DOSBase, BPTR toclone, LONG mode);

AROS_SH3H(Run, 41.3,                "Start a program as a background process",
AROS_SHAH(BOOL  , ,EXECUTE,/S,FALSE,"Allows a script to be run in background"),
AROS_SHAH(BOOL  , ,QUIET  ,/S,FALSE,"\tDon't print the background CLI's number"),
AROS_SHAH(STRPTR, ,COMMAND,/F,NULL ,"The program (resp. script) to run (arguments\n"
                                    "\t\t\t\tallowed)") )
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    BPTR cis = BNULL, cos = BNULL, ces = BNULL;
    LONG CliNum;


    if (cli)
    {
	BPTR toclone;

	if (IsInteractive(Input()))
	    toclone = Input();
	else
	    toclone = cli->cli_StandardInput;

	cis = duphandle(DOSBase, toclone, FMF_READ);

	if (IsInteractive(Output()))
	    toclone = Output();
	else
	    toclone = cli->cli_StandardOutput;

	cos = duphandle(DOSBase, toclone, FMF_WRITE);

	/* This is sort of a hack, needed because the original AmigaOS shell didn't allow
	   Error() redirection, so all the scripts written so far assume that only Input() and
	   Output() require to be redirected in order to not block the parent console */
        if (Error() != cli->cli_StandardError && IsInteractive(Error()))
	{
	    toclone = Error();

	    ces = duphandle(DOSBase, toclone, FMF_WRITE);
	}
    }

    struct DateStamp  ds;
    BYTE              tmpname[256];
    BPTR              tmpfile      = BNULL;
    int               count        = 0;

    if ( (SHArg(EXECUTE)) && (SHArg(COMMAND)) )
    {
	BYTE tmpdir[4];
	BPTR tmplock;
	struct Window *win;
	struct Process *proc = (struct Process*)FindTask(0);

	DateStamp(&ds);
	
	win = proc->pr_WindowPtr;
	proc->pr_WindowPtr = (struct Window *)-1;
	tmplock = Lock("T:", SHARED_LOCK);
	proc->pr_WindowPtr = win;
	if (tmplock) {
	    strcpy(tmpdir, "T:");
	    UnLock(tmplock);
	} else {
	    strcpy(tmpdir, ":T/");
	}

        DateStamp(&ds);
        do
        {
            count++;
            __sprintf(tmpname, "%sTmp%lu%lu%lu%lu%d", tmpdir,
                      ((struct Process *)FindTask(NULL))->pr_TaskNum,
                      ds.ds_Days, ds.ds_Minute, ds.ds_Tick, count);
            tmpfile = Open(tmpname, MODE_NEWFILE);
        } while (tmpfile == BNULL && IoErr() == ERROR_OBJECT_IN_USE);

        if (tmpfile)
        {
            if ( (0 != FPuts(tmpfile, "Execute "))     ||
                 (0 != FPuts(tmpfile, SHArg(COMMAND))) ||
                 (0 != FPuts(tmpfile, "\nEndShell\n"))    )
            {
                PrintFault(IoErr(), "Run");
	        Close(cis);
	        Close(cos);
	        Close(ces);
	        Close(tmpfile);
		DeleteFile(tmpname);

		return RETURN_FAIL;
            }
            Seek(tmpfile, 0, OFFSET_BEGINNING);
        }
        else
        {
	    PrintFault(IoErr(), "Run");
	    Close(cis);
	    Close(cos);
	    Close(ces);

	    return RETURN_FAIL;
        }
    }

    {
        struct TagItem tags[] =
        {
	    { SYS_ScriptInput, (IPTR)tmpfile },
	    { SYS_Input,       (IPTR)cis     },
	    { SYS_Output,      (IPTR)cos     },
	    { SYS_Error,       (IPTR)ces     },
	    { SYS_Background,  TRUE          },
	    { SYS_Asynch,      TRUE          },
	    { SYS_CliNumPtr,   (IPTR)&CliNum },
	    { SYS_UserShell,   TRUE          },
	    { TAG_DONE,        0             }
        };

        if ( SystemTagList((SHArg(EXECUTE) && SHArg(COMMAND)) ?
                           (CONST_STRPTR) ""                  :
                           (CONST_STRPTR) SHArg(COMMAND)       ,
                           tags                                ) == -1 )
        {
	    PrintFault(IoErr(), "Run");
	    Close(cis);
	    Close(cos);
	    Close(ces);
            if (tmpfile)
	    {
	        Close(tmpfile);
	        DeleteFile(tmpname);
            }

	    return RETURN_FAIL;
        }
    }

    if ( !(SHArg(QUIET)) )
    {
        IPTR data[1] = { (IPTR)CliNum };
        VPrintf("[CLI %ld]\n", data);
    }
#if DEBUG
#else
    if ( SHArg(EXECUTE) && SHArg(COMMAND) )
        while( (0 == DeleteFile(tmpname)) && (count++ < 10) )
            Delay(10);
#endif
    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}

static BPTR duphandle(struct DosLibrary * DOSBase, BPTR toclone, LONG mode)
{
    BPTR newhandle;
#ifdef AROS_DOS_PACKETS
    struct MsgPort *old;
    old = SetConsoleTask(((struct FileHandle*)BADDR(toclone))->fh_Type);
    newhandle = Open("*", mode);
    SetConsoleTask(old);
#else
    BPTR old;
    old = CurrentDir(toclone);
    newhandle = Open("", mode);
    CurrentDir(old);
#endif
    return newhandle;
}
