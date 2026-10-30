/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH0(ULONG, CamdTime,

/*  SYNOPSIS */

/*  LOCATION */
	struct CamdBase *, CamdBase, 42, Camd)

/*  FUNCTION
		Gives CAMD's clock: milliseconds since camd.library started.

    INPUTS

    RESULT
		The time in milliseconds. It only moves forward, also when the
		system time is set, and wraps to 0 after 2^32 milliseconds
		(49.7 days): compare times by subtracting them.

    NOTES
		New in V42. Callable from any task and from interrupts.

		A node created with MIDI_SystemClock set has the messages it
		receives stamped with this clock.

    EXAMPLE

    BUGS

    SEE ALSO
		CreateMidiA(), SetMidiAttrsA()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

	return CamdClock();

    AROS_LIBFUNC_EXIT
}
