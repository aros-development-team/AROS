/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: FailAt - Set the failure level of a process
    Lang: english
*/

/**************************************************************************

    NAME
	FailAt

    FORMAT
	FailAt <limit>

    SYNOPSIS
	RCLIM/A/N

    LOCATION
	INTERNAL

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

#define ARG_TEMPLATE	"RCLIM/A/N"
#define ARG_RCLIM	0
#define TOTAL_ARGS	1

static const char version[] = "$VER: FailAt 41.1 (3.1.1998)";
static const char exthelp[] =
    "FailAt : Set the return code failure limit of the current script\n"
    "\tRCLIM/A/N   The new return code limit\n";

int main(int argc, char **argv)
{
    struct RDArgs *rd, *rda;
    IPTR args[TOTAL_ARGS] = { 0 };
    int error = 0;

    rda = AllocDosObject(DOS_RDARGS, NULL);
    if( rda != NULL )
    {
	rda->RDA_ExtHelp = (STRPTR)exthelp;

	rd = ReadArgs( ARG_TEMPLATE, args, rda );
	if( rd != NULL )
	{
	    struct CommandLineInterface *cli;

	    /* Nice and easy this command... */
	    cli = Cli();

	    if( cli != NULL )
	    {
		cli->cli_FailLevel = args[ARG_RCLIM];
	    }
	    else
		error = ERROR_OBJECT_WRONG_TYPE;

	    FreeArgs(rd);
	}
	else
	    error = IoErr();

	FreeDosObject(DOS_RDARGS, rda);
    }
    else
	error = IoErr();

    if( error != 0 )
    {
	PrintFault(error, "FailAt");
	return RETURN_FAIL;
    }
    SetIoErr(0);
    return 0;
}
	    
