/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: getsystime.c 20492 2004-01-03 18:46:50Z schulz $

    Desc: GetSysTime() - Find out what time it is.
    Lang: english
*/
#include "ticks.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/timer.h>

	AROS_LH1(void, GetSysTime,

/*  SYNOPSIS */
	AROS_LHA(struct timeval *, dest, A0),

/*  LOCATION */
	struct Device *, TimerBase, 11, Timer)

/*  FUNCTION
	GetSysTime() will fill in the supplied timeval with the current
	system time.

    INPUTS
	dest    -   A pointer to the timeval you want the time stored in.

    RESULT
	The timeval "dest" will be filled with the current system time.

    NOTES
	This function is safe to call from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
	TR_GETSYSTIME, TR_SETSYSTIME

    INTERNALS

    HISTORY
	18-02-1997  iaint   Implemented.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,TimerBase)

    struct TimerBase *timerBase = (struct TimerBase *)TimerBase;

    EClockUpdate(TimerBase);
    Disable();
    dest->tv_secs = timerBase->tb_CurrentTime.tv_secs;
    dest->tv_micro = timerBase->tb_CurrentTime.tv_micro;
    Enable();

    AROS_LIBFUNC_EXIT
} /* GetSysTime */
