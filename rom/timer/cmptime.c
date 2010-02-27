/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CmpTime() - compare two time values.
    Lang: english
*/
#include "timer_intern.h"

/*****************************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>

	AROS_LH2(LONG, CmpTime,

/*  SYNOPSIS */
	AROS_LHA(struct timeval *, dest, A0),
	AROS_LHA(struct timeval *, src, A1),

/*  LOCATION */
	struct Device *, TimerBase, 9, Timer)

/*  FUNCTION
	CmpTime() will compare two timeval's for magnitude, and return
	which is the larger.

    INPUTS
	dest    -   Destination timeval
	src     -   Source timeval

    RESULT
	-1 if dest has more time than src (i.e. dest > src)
	 0 if dest and src are the same (i.e. dest == src)
	+1 if dest has less time than src (i.e. dest < src)

    NOTES
	This function is safe to call from interrupts.

    EXAMPLE

    BUGS
	The registers A0 and A1 may not be preserved.

    SEE ALSO
	AddTime(), SubTime()

    INTERNALS

    HISTORY
	18-02-1997  iaint   Implemented.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG diff;

    if(dest->tv_secs == src->tv_secs)
	diff = src->tv_micro - dest->tv_micro;
    else
	diff = src->tv_secs - dest->tv_secs;

    if (diff < 0)
	return -1;
    else if (diff > 0)
	return 1;
    else
	return 0;

    AROS_LIBFUNC_EXIT
} /* CmpTime */
