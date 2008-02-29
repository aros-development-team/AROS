/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

        Sys:C

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

    BPTR from;
    struct CommandLineInterface *cli = Cli();

    if (!cli)
    {
        return RETURN_ERROR;
    }

    from = Open(SHArg(NAME), FMF_READ);

    if (!from)
    {
	IPTR data[] = { (IPTR)SHArg(NAME) };
	VFPrintf(Error(), "EXECUTE: can't open %s\n", data);
	PrintFault(IoErr(), NULL);

	return RETURN_FAIL;
    }

    if (!cli->cli_Interactive)
    {
	struct DateStamp ds;
	BYTE tmpname[256];
	BPTR tmpfile = NULL;
        int count = 0;

	DateStamp(&ds);

        do {
            count++;
            __sprintf(tmpname, "T:Tmp%lu%lu%lu%lu%d",
                      ((struct Process *)FindTask(NULL))->pr_TaskNum,
                      ds.ds_Days, ds.ds_Minute, ds.ds_Tick, count);
	    tmpfile = Open(tmpname, MODE_NEWFILE);
        } while (tmpfile == NULL && IoErr() == ERROR_OBJECT_IN_USE);

	if (tmpfile)
	{
	    LONG c, len;
	    STRPTR arguments, s;

	    if (FPuts(tmpfile, ".pushis\n") != -1)
		while((c = FGetC(from)) != -1 && FPutC(tmpfile, c) != -1);

	    c = IoErr();
	    Close(from);

	    if (c)
	    {
		FPuts(Error(),
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
		FPuts(Error(), "EXECUTE: error while creating temporary file\n");
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

	    arguments = SHArg(ARGUMENTS);
	    if (arguments)
	    {
		s = AROS_BSTR_ADDR(cli->cli_CommandName);
		len = strlen(arguments);

		AROS_BSTR_setstrlen(cli->cli_CommandName, len + 1);
		CopyMem((APTR)arguments, s, len);
		s[len] = '\n';
	    }
	    else
		AROS_BSTR_setstrlen(cli->cli_CommandName, 0);

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
	    FPuts(Error(), "EXECUTE: error while creating temporary file\n");
	    PrintFault(c, NULL);
	    Close(from);

	    return RETURN_FAIL;
	}
    }
    else
    {
	STRPTR arguments = SHArg(ARGUMENTS), s;
	LONG len;

	if (arguments)
	{
kprintf("[Execute] args: %s\n", arguments);
	    s = AROS_BSTR_ADDR(cli->cli_CommandName);
	    len = strlen(arguments);

	    AROS_BSTR_setstrlen(cli->cli_CommandName, len + 1);
	    CopyMem((APTR)arguments, s, len);
	    s[len] = '\n';
	}
	else
	    AROS_BSTR_setstrlen(cli->cli_CommandName, 0);

        cli->cli_Interactive  = FALSE;
        cli->cli_CurrentInput = from;
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
