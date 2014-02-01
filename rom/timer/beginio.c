/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BeginIO - Start up a timer.device request, generic version for software timer emulation
    Lang: english
*/

#include <aros/libcall.h>

#include "timer_intern.h"

/*****************************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>
	AROS_LH1(void, BeginIO,

/*  SYNOPSIS */
	AROS_LHA(struct timerequest *, timereq, A1),

/*  LOCATION */
	struct TimerBase *, TimerBase, 5, Timer)

/*  FUNCTION
	BeginIO() will perform a timer.device command. It is normally
	called from within DoIO() and SendIO().

    INPUT
	timereq		- The request to process.

    RESULT
	The requested message will be processed.

    NOTES
	This function is safe to call from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	exec.library/AbortIO(), exec.library/SendIO(), exec.library/DoIO()

    INTERNALS

    HISTORY
	23-01-1998  iaint	Implemented again.

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    /*
     * common_BeginIO() is called in the middle of BeginIO() implementation
     * to provide support for time base correction.
     * If we query TBC hardware before starting doing anything here, we can
     * calculate how much time it took to do all processing and take this
     * into account when programming hardware.
     * See PowerPC native versions for working example.
     */

    common_BeginIO(timereq, TimerBase);

    /*
     * common_BeginIO() returns TRUE if it wants us to re-adjust
     * our hardware (if the request was added to the head of a queue
     * and elapsed time changes).
     * Real hardware version of timer.device needs to do this here.
     */

    AROS_LIBFUNC_EXIT
}
