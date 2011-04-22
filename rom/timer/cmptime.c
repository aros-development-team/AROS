/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CmpTime() - compare two time values.
    Lang: english
*/

#include <timer_intern.h>
#include "timer_macros.h"

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

    return CMPTIME(dest, src);

    AROS_LIBFUNC_EXIT
} /* CmpTime */
