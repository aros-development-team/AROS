/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Delay CLI command. 
    Lang: english
*/

/******************************************************************************


    NAME

        Delay (n) [TICK | TICKS]

    SYNOPSIS

        TIME/N,TICK=TICKS/S

    LOCATION

       Sys:C

    FUNCTION

        Wait a certain amount of ticks clock (1/50 of a second).

    INPUTS

        TICK=TICKS  --  time unit in ticks to wait for.
    RESULT

    NOTES
	An arbitrary limit is set to 15000 ticks to wait for.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

int __nocommandline;

int main(void)
{
    IPTR args[1] = { 0 };
    struct RDArgs *rda = ReadArgs("TIME/N,TICK=TICKS/S", args, NULL);

    if (rda)
    {
	LONG ticks = *((LONG *)args[0]);

	if (ticks >= 0 && ticks <= TICKS_PER_SECOND * 300)
	{
	    Delay(ticks);
	    FreeArgs(rda);
	    return RETURN_OK;
	}
	FreeArgs(rda);
    }
    return RETURN_FAIL;
}

