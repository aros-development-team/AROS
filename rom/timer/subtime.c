/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SubTime - subtract two timevals from each other.
    Lang: english
*/

#include <devices/timer.h>

#include "timer_macros.h"

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

    SUBTIME(dest, src);

    AROS_LIBFUNC_EXIT
    
} /* SubTime */
