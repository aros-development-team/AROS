/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
*/
#include <proto/exec.h>
#include <dos/filesystem.h>
#include <dos/dos.h>
#include <dos/bptr.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <string.h>

#define SH_GLOBAL_SYSBASE 1

#include "shcommands.h"



AROS_SH1(Execute, 41.1,
AROS_SHA(STRPTR, ,NAME,/A,NULL))
{
    AROS_SHCOMMAND_INIT

    BPTR from;
    struct CommandLineInterface *cli = Cli();

    if (!cli)
        return RETURN_ERROR;

    from = Open(SHArg(NAME), FMF_READ);
    if (!from)
    {
        PrintFault(IoErr(), "Execute: couldn't open the script file");
	return RETURN_FAIL;
    }

    if (!cli->cli_Interactive)
    {
	struct DateStamp ds;
	BYTE tmpname[2+3+10+10+2+2+1];
	BPTR tmpfile;
	DateStamp(&ds);

	__sprintf(tmpname, "T:Tmp%lu%lu%lu%lu",
	          ((struct Process *)FindTask(NULL))->pr_TaskNum,
		  ds.ds_Days, ds.ds_Minute, ds.ds_Tick);


	tmpfile = Open(tmpname, FMF_LOCK|FMF_WRITE|FMF_READ|FMF_CREATE|FMF_CLEAR);
	if (tmpfile)
	{
	    LONG c;

	    while((c = FGetC(from)) != -1 && FPutC(tmpfile, c) != -1);

	    c = IoErr();

	    Close(from);

	    if (c)
	    {
	        PrintFault(c, "Execute: error while creating temporary file");
		Close(tmpfile);
		DeleteFile(tmpname);
		return RETURN_FAIL;
	    }

	    c = '\n';
	    FPutC(tmpfile, c);

	    while((c = FGetC(cli->cli_CurrentInput)) != -1 && FPutC(tmpfile, c) != -1);

	    c = IoErr();

	    if (c)
	    {
	        PrintFault(c, "Execute: error while creating temporary file");
 		Close(tmpfile);
		DeleteFile(tmpname);
		return RETURN_FAIL;
	    }


	    Close(cli->cli_CurrentInput);
	    if (AROS_BSTR_strlen(cli->cli_CommandFile))
	        DeleteFile(BADDR(cli->cli_CommandFile));

	    {
	        LONG len = strlen(tmpname);
	        CopyMem(tmpname, BADDR(cli->cli_CommandFile), len);
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
	    PrintFault(IoErr(), "Execute: couldn't create temporary file");
	    Close(from);
	    return RETURN_FAIL;
	}
    }
    else
    {
        cli->cli_Interactive  = FALSE;
        cli->cli_CurrentInput = from;
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
