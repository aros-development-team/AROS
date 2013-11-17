/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH4(BOOL, TimedDisplayAlert,

/*  SYNOPSIS */
        AROS_LHA(ULONG  , alertnumber, D0),
        AROS_LHA(UBYTE *, string     , A0),
        AROS_LHA(UWORD  , height     , D1),
        AROS_LHA(ULONG  , time       , A1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 137, Intuition)

/*  FUNCTION
	Display an alert with automatic time-out. See DisplayAlert()
	documentation.

    INPUTS
	alertnumber - Alert code
	string - Text data to display
	height - Total height of alert display in pixels
	time   - Timeout measured in display frame refresh periods.

    RESULT
	TRUE or FALSE depending on user's reaction. FALSE in case of timeout.

    NOTES
	See DisplayAlert() documentation for detailed description of
	parameters.

    EXAMPLE

    BUGS
	In AROS timeout is currently not implemented. Note that this
	function is obsolete and strongly deprecated for use in software.
	It is present only for backwards compatibility with AmigaOS(tm).

    SEE ALSO
	DisplayAlert()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return DisplayAlert(alertnumber, string, height);
    
    AROS_LIBFUNC_EXIT
} /* TimedDisplayAlert */
