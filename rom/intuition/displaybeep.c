/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function DisplayBeep()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, DisplayBeep,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 16, Intuition)

/*  FUNCTION
	The Amiga has no internal speaker, so it flashes the background
	color of the specified screen as a signal. If the argument is
	NULL all screens will be flashed.

    INPUTS
	screen - The Screen that will be flashed.
		If NULL all screens will flash.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Hardware with a speaker should make an audible beep, too.
	Maybe even leave out the flashing on those architectures.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/DisplayBeep()
    aros_print_not_implemented ("DisplayBeep");

    AROS_LIBFUNC_EXIT
} /* DisplayBeep */
