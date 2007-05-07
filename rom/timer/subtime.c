/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SubTime - subtract two timevals from each other.
    Lang: english
*/
#include "timer_intern.h"

/*****************************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>

	AROS_LH2(void, SubTime,

/*  SYNOPSIS */
	AROS_LHA(struct timeval *, dest, A0),
	AROS_LHA(struct timeval *, src, A1),

/*  LOCATION */
	struct Device *, TimerBase, 8, Timer)

/*  FUNCTION
	SubTime() will subtract the src timeval from the destination
	timeval, ie "dest - src --> dest".

    INPUTS
	dest    -   Destination timeval
	src     -   Source timeval

    RESULT
	The timeval dest will contain the sum (dest - src).

    NOTES
	This function is safe to call from interrupts.

    EXAMPLE

    BUGS
	May not preserve registers.

    SEE ALSO
	AddTime(), CmpTime()

    INTERNALS

    HISTORY
	18-02-1997  iaint   Implemented.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Normalize the terms */
    while(src->tv_micro > 999999)
    {
	src->tv_secs++;
	src->tv_micro -= 1000000;
    }
    while(dest->tv_micro > 999999)
    {
	dest->tv_secs++;
	dest->tv_micro -= 1000000;
    }

    /* Check if wrap around will happen, when subtracting src->tv_micro
       from dest->tv_micro. If yes, then normalize, by adding 1 sec to
       micros and subtracting 1 sec from secs. Note: this check must be
       done here, ie. before subtracting src timeval from dest timeval! */
       
    if(dest->tv_micro < src->tv_micro)
    {
	dest->tv_micro += 1000000;
	dest->tv_secs--;
    }

    dest->tv_micro -= src->tv_micro;
    dest->tv_secs -= src->tv_secs;

    AROS_LIBFUNC_EXIT
    
} /* SubTime */
