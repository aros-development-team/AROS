/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#include <utility/tagitem.h>

#include <aros/shcommands.h>

AROS_SH1H(FailAt, 41.1,             "Set the return code failure limit of the current script",
AROS_SHAH(LONG *, , RCLIM,/N, NULL, "The new return code limit"))
{
    AROS_SHCOMMAND_INIT

    struct CommandLineInterface *cli = Cli();


    if (cli == NULL)
    {
	return RETURN_FAIL;
    }

    /* Write current fail level */
    if (SHArg(RCLIM) == NULL)
    {
	VPrintf("Fail limit: %ld\n", (IPTR *)&cli->cli_FailLevel);
    }
    /* Set new fail level */
    else
    {
	cli->cli_FailLevel = *SHArg(RCLIM);
    }

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
	    
