/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.

    Desc: ResetBattClock()
    Lang: english
*/
#include "battclock_intern.h"

/*****************************************************************************

    NAME */
#include <proto/battclock.h>

	AROS_LH0(void, ResetBattClock,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct BattClockBase*, BattClockBase, 1, Battclock)

/*  FUNCTION
	Reset the system battery-backed-up clock. This will reset the clock
	back to 0 seconds (or midnight, 1st Jan 1978).

    INPUTS
	None.

    RESULT
	The clock will be reset.

    NOTES
	This function may do nothing when the battery backed up clock either
	doesn't exist, or may not be writable by the operating system.

    EXAMPLE

    BUGS

    SEE ALSO
	ReadBattClock(), WriteBattClock()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    resetbattclock(BattClockBase);
    WriteBattClock(0);

    AROS_LIBFUNC_EXIT
} /* ResetBattClock */
