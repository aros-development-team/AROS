/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
*/
#include <proto/exec.h>
#include <dos/filesystem.h>
#include <proto/dos.h>

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
        PrintFault(IoErr(), "Execute");
	return RETURN_FAIL;
    }

    if (!cli->cli_Interactive)
    {
        PutStr("Execute doesn't handle nested scripts yet\n");
        Close(from);
	return RETURN_ERROR;
    }

    cli->cli_Interactive  = FALSE;
    cli->cli_CurrentInput = from;

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
