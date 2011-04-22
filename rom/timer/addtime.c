/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AddTime() - add two timeval's together.
    Lang: english
*/

#include <timer_intern.h>
#include "timer_macros.h"

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

    ADDTIME(dest, src);

    AROS_LIBFUNC_EXIT
} /* AddTime */
