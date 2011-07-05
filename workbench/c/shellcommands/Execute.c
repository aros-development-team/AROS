/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang:
*/

/******************************************************************************

    NAME

        Execute <script> [{<arguments>}]

    SYNOPSIS

        FILE/A

    LOCATION

        C:

    FUNCTION

        Executes a script with DOS commands.

    INPUTS

        FILE -- file to execute

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/exec.h>
#include <dos/filesystem.h>
#include <dos/dos.h>
#include <dos/bptr.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <string.h>

#define SH_GLOBAL_SYSBASE 1

#define DEBUG 0
#include <aros/debug.h>
#include <aros/shcommands.h>

AROS_SH2(Execute, 41.1,
AROS_SHA(STRPTR, ,NAME     , /A, NULL),
AROS_SHA(STRPTR, ,ARGUMENTS, /F, NULL))
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();
    STRPTR arguments = SHArg(ARGUMENTS), s;
    BPTR from;
    struct Process *me = (struct Process *)FindTask(NULL);

    if (!cli)
        return RETURN_ERROR;

    from = Open(SHArg(NAME), MODE_OLDFILE);

    if (!from)
    {
	IPTR data[] = { (IPTR)SHArg(NAME) };
	VFPrintf(me->pr_CES, "EXECUTE: can't open %s\n", data);
	PrintFault(IoErr(), NULL);
	return RETURN_FAIL;
    }

    if (cli->cli_Interactive)
    {
        cli->cli_Interactive  = FALSE;
        cli->cli_CurrentInput = from;
    }
    else
    {
	struct DateStamp ds;
	BYTE tmpname[256];
	BPTR tmpfile = BNULL;
	int count = 0;
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

        do {
            count++;
            __sprintf(tmpname, "%sTmp%lu%lu%lu%lu%d", tmpdir,
                      ((struct Process *)FindTask(NULL))->pr_TaskNum,
                      ds.ds_Days, ds.ds_Minute, ds.ds_Tick, count);
	    tmpfile = Open(tmpname, MODE_NEWFILE);
        } while (tmpfile == BNULL && IoErr() == ERROR_OBJECT_IN_USE);

	if (tmpfile)
	{
	    LONG c;

	    if (FPuts(tmpfile, ".pushis\n") != -1)
		while((c = FGetC(from)) != -1 && FPutC(tmpfile, c) != -1);

	    c = IoErr();
	    Close(from);

	    if (c)
	    {
		FPuts(me->pr_CES,
		      "EXECUTE: error while creating temporary file\n");
		PrintFault(c, NULL);
		Close(tmpfile);
		DeleteFile(tmpname);

		return RETURN_FAIL;
	    }

	    c = '\n';
	    FPutC(tmpfile, c);

	    FPuts(tmpfile, ".popis\n");

	    while((c = FGetC(cli->cli_CurrentInput)) != -1 && FPutC(tmpfile, c) != -1);

	    c = IoErr();

	    if (c)
	    {
		FPuts(me->pr_CES, "EXECUTE: error while creating temporary file\n");
		PrintFault(c, NULL);
		Close(tmpfile);
		DeleteFile(tmpname);

		return RETURN_FAIL;
	    }

	    Close(cli->cli_CurrentInput);
	    if (AROS_BSTR_strlen(cli->cli_CommandFile))
	        DeleteFile(AROS_BSTR_ADDR(cli->cli_CommandFile));

	    {
	        LONG len = strlen(tmpname);
	        CopyMem(tmpname, AROS_BSTR_ADDR(cli->cli_CommandFile), len);
	        AROS_BSTR_setstrlen(cli->cli_CommandFile, len);
	    }

	    cli->cli_CurrentInput = tmpfile;
	    Seek(tmpfile, 0, OFFSET_BEGINNING);
	}
	else
	{
	    /*
	      we should try to open ":T", but since ":"
	      is not handled correctly yet, we just give up
	    */
	    LONG c = IoErr();
	    FPuts(me->pr_CES, "EXECUTE: error while creating temporary file\n");
	    PrintFault(c, NULL);
	    Close(from);

	    return RETURN_FAIL;
	}
    }

    if (arguments)
    {
	LONG len = strlen(arguments);

	D(bug("[Execute] args (len: %d): %s\n", len, arguments));
	if (len >= 255)
	    return ERROR_LINE_TOO_LONG;

	s = AROS_BSTR_ADDR(cli->cli_CommandName);

	AROS_BSTR_setstrlen(cli->cli_CommandName, len);
	CopyMem(arguments, s, len);
	s[len] = '\0';
    }
    else
	AROS_BSTR_setstrlen(cli->cli_CommandName, 0);

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
