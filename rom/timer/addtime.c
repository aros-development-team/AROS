/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AddTime() - add two timeval's together.
    Lang: english
*/
#include "timer_intern.h"

/*****************************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>

	AROS_LH2(void, AddTime,

/*  SYNOPSIS */
	AROS_LHA(struct timeval *, dest, A0),
	AROS_LHA(struct timeval *, src, A1),

/*  LOCATION */
	struct Device *, TimerBase, 7, Timer)

/*  FUNCTION
	Add two timeval's together. The result will be the sum
	dest + src --> dest.

	The values of A0 and A1 will not be changed.

    INPUTS
	dest    -   Destination timeval.
	src     -   Source timeval.

    RESULT
	dest will contain (src + dest).

    NOTES
	This function can be called from Interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	SubTime(), CmpTime()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    timer_lib.fd and clib/timer_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    dest->tv_micro += src->tv_micro;
    dest->tv_secs += src->tv_secs;
    
    /* Normalize the result. */
    while(dest->tv_micro > 999999)
    {
	dest->tv_secs++;
	dest->tv_micro -= 1000000;
    }

    AROS_LIBFUNC_EXIT
} /* AddTime */
