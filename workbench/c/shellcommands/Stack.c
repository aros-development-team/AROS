/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stack CLI command
    Lang: English
*/

/**************************************************************************

    NAME
        Stack

    FORMAT
        Stack [[SIZE] (stack size)]

    SYNOPSIS
        SIZE/N

    LOCATION
	C:

    FUNCTION
        Stack sets the default stack size of the current Shell. This is the
        stack size of the commands run from the Shell. If you use Stack
	without arguments, the current stack size will be written out.

    EXAMPLE

    HISTORY

        1.1.2000  SDuvan  implemented (yes, celebration is over -- time for
	                  some hard work again -- but I might change to
			  everyday clothes first...)

**************************************************************************/

#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <proto/dos.h>

#include <aros/shcommands.h>

#define  MINIMUM_STACK_SIZE   2048     /* This is a wild guess and depends on
					  (among other things) how much stack
				          space AROS functions need. */

AROS_SH1(Stack, 41.1,
AROS_SHA(LONG *, , SIZE,/N,NULL))
{
    AROS_SHCOMMAND_INIT

    int            retval = RETURN_OK;
    struct CommandLineInterface *cli = Cli();


    /* We must be a Shell to do this operation */
    if (cli == NULL)
    {
	return RETURN_FAIL;
    }

    /* Write out current stack size */
    if (SHArg(SIZE) == NULL)
    {
	LONG currentstack = cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;
	
	VPrintf("Current stack size is %ld bytes\n",
		(IPTR *)&currentstack);
    }
    /* Set new stack size */
    else
    {
	LONG  newSize = *SHArg(SIZE);
	
	if (newSize > MINIMUM_STACK_SIZE)
	{
	    cli->cli_DefaultStack = (newSize + CLI_DEFAULTSTACK_UNIT - 1) / CLI_DEFAULTSTACK_UNIT;
	}
	else
	{
	    PutStr("Requested size is too small.\n");
	    retval = RETURN_ERROR;
	}
    }

    return retval;

    AROS_SHCOMMAND_EXIT
}
