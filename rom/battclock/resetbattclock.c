/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
    $Id$

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
	APTR, BattClockBase, 1, Battclock)

/*  FUNCTION
	Reset the system battery backed up clock. This will reset the clock
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
	ReadBattClock, WriteBattClock

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    battclock_lib.fd and clib/battclock_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return;

    AROS_LIBFUNC_EXIT
} /* ResetBattClock */
