
/*
    Copyright (C) 2000 AROS - The Amiga Research OS
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

static const char version[] = "$VER: Stack 41.1 (1.1.2000)";

#define  MINIMUM_STACK_SIZE   2048     /* This is a wild guess and depends on
					  (among other things) how much stack
				          space AROS functions need. */

int main(int argc, char **argv)
{
    struct RDArgs *rda;
    IPTR           args[1] = { (IPTR)NULL };
    int            retval = RETURN_OK;
    struct CommandLineInterface *cli = Cli();

    /* We must be a Shell to do this operation */
    if(cli == NULL)
	return RETURN_FAIL;

    rda = ReadArgs("SIZE/N", args, NULL);

    if(rda != NULL)
    {
	/* Write out current stack size */
	if(args[0] == NULL)
	{
	    VPrintf("Current stack size is %ld bytes\n",
		    (IPTR *)&cli->cli_DefaultStack);
	}
        /* Set new stack size */
	else
	{
	    LONG  newSize = *(LONG *)args[0];

	    if(newSize > MINIMUM_STACK_SIZE)
		cli->cli_DefaultStack = newSize;
	    else
	    {
	        PutStr("Requested size is too small.\n");
		retval = RETURN_ERROR;
	    }
	}

	FreeArgs(rda);
    }
    else
    {
	PrintFault(IoErr(), "Stack");
	retval = RETURN_FAIL;
    }

    return retval;
}
