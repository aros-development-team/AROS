/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Why CLI command
    Lang: English
*/

/******************************************************************************


    NAME

        Why

    SYNOPSIS

    LOCATION

        Workbench:C

    FUNCTION

        Print additional information why an operation failed. Ordinarily
	when a command fails a breif message is printed that typically
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

static const char version[] = "$VER: why 41.3 (26.8.98)\n";


int main()
{
    struct RDArgs *rda;
    IPTR args[1]; /* we actually need 0 arguments */
    struct CommandLineInterface *cli;
    LONG lasterror;
    int error = RETURN_OK;
    
    rda = ReadArgs("", args, NULL);
    
    if (rda != NULL)
    {
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
	
	FreeArgs(rda);
    }
    else
    {
        PrintFault(IoErr(), "Why");
        error = RETURN_FAIL;
    }

    return error;
}
