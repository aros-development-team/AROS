
/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <proto/intuition.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH1(VOID, rtScreenToFrontSafely,

/*  SYNOPSIS */

	AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */

	struct Library *, RTBase, 23, ReqTools)

/*  FUNCTION

    Brings the specified screen to the front of the display, but only after
    checking it is still in the list of currently open screens.

    This function can be used to bring a screen back to the front of the
    display after bringing another screen to the front.  If the first screen
    closed while you where busy it is harmless to call this function, unlike
    calling the normal ScreenToFront().

    INPUTS

    screen  --  pointer to the screen

    RESULT

    NOTES

    This function is for the advanced ReqTools user.

    EXAMPLE

    BUGS

    SEE ALSO

    intuition.library/ScreenToFront()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Screen *scr = ((IntuiBase *)IntuitionBase)->FirstScreen;

    Forbid();

    while(scr != NULL)
    {
	if(scr == screen)
	{
	    ScreenToFront(screen);
	    break;
	}
	
	scr = scr->NextScreen;
    }

    Permit();

    AROS_LIBFUNC_EXIT
} /* rtScreenToFrontSafely*/
