/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: FailAt - Set the failure level of a process
    Lang: English
*/

/**************************************************************************

    NAME
	FailAt

    FORMAT
	FailAt <limit>

    SYNOPSIS
	RCLIM/N

    LOCATION
	C:

    FUNCTION
	FailAt sets the return code limit of the current shell script. If
	any command returns with a failure code of this value or higher
	the script shall abort.

	Common failure codes are:
	    0	- No error
	    5	- Warning
	    10  - Error
	    20	- Failure

	The normal value for the return code limit is 10.

    EXAMPLE
	If we have a script with the commands

	    Copy RAM:SomeFile DF0:
	    Echo "Done!"

	and the file RAM:SomeFile does not exist, the Copy command will
	return with:

	    Copy: object not found
	    Copy: returned with error code 20

	and the script will abort. However if you include the command

	    FailAt 21

	then the script will complete since the return code from Copy is
	less than the return code limit.

**************************************************************************/

#include <exec/types.h>
#include <exec/tasks.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define ARG_TEMPLATE	"RCLIM/N"
#define ARG_RCLIM	0
#define TOTAL_ARGS	1

static const char version[] = "$VER: FailAt 41.1 (3.1.1998)";
static const char exthelp[] =
    "FailAt : Set the return code failure limit of the current script\n"
    "\tRCLIM/N   The new return code limit\n";


int main(int argc, char **argv)
{
    struct RDArgs *rd, *rda;
    IPTR           args[TOTAL_ARGS] = { (IPTR)NULL };
    int            error = 0;
    struct CommandLineInterface *cli = Cli();

    if(cli == NULL)
	return RETURN_FAIL;

    rda = AllocDosObject(DOS_RDARGS, NULL);
    if(rda != NULL)
    {
	rda->RDA_ExtHelp = (STRPTR)exthelp;

	rd = ReadArgs(ARG_TEMPLATE, args, rda);
	if(rd != NULL)
	{
	    /* Write current fail level */
	    if(args[ARG_RCLIM] == NULL)
	    {
		VPrintf("Fail limit: %ld\n", (IPTR *)&cli->cli_FailLevel);
	    }
	    /* Set new fail level */
	    else
	    {
		cli->cli_FailLevel = *(LONG *)args[ARG_RCLIM];
	    }

	    FreeArgs(rd);
	}
	else
	    error = IoErr();

	FreeDosObject(DOS_RDARGS, rda);
    }
    else
	error = IoErr();
    
    if(error != 0)
    {
	PrintFault(error, "FailAt");
	return RETURN_FAIL;
    }

    return RETURN_OK;
}
	    
