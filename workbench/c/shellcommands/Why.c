/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Why CLI command
    Lang: English
*/

/******************************************************************************


    NAME

        Why

    SYNOPSIS

    LOCATION

        C:

    FUNCTION

        Print additional information why an operation failed. Ordinarily
	when a command fails a brief message is printed that typically
	includes the name of the command that failed but provides few
	details. Why fills in details related to the failed operation.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/


#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <aros/shcommands.h>

AROS_SH0(Why, 41.3)
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli;
    LONG  lasterror;
    int   error = RETURN_OK;
    

    if ((cli = Cli()) != NULL)
    {
        lasterror = cli->cli_Result2;

        if (cli->cli_ReturnCode == 0 || lasterror == 0)
	{
            VPrintf("The last command did not set a return-value\n", NULL);
	}
        else
        {
            PrintFault(lasterror, "The last command failed, reason");
            SetIoErr(0);
        }
    }
    else
    {
        error = RETURN_FAIL;
    }

    return error;

    AROS_SHCOMMAND_EXIT
}
