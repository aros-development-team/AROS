/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
 
    Desc: Intuition function WBenchToFront()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH0(BOOL, WBenchToFront,

         /*  SYNOPSIS */

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 57, Intuition)

/*  FUNCTION
	Make the WorkBench screen the frontmost.
 
    INPUTS
 
    RESULT
	TRUE if the Workbench screen is open, FALSE else.
 
    NOTES
	This function does not influence the position of the screen,
	it just changes the depth-arrangement of the screens.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
	ScreenToBack(), ScreenToFront(), WBenchToBack()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

	struct Screen *curscreen;

	curscreen = GetPrivIBase(IntuitionBase)->WorkBench;

	if ( curscreen )
	{
		ScreenToFront ( curscreen );
		return TRUE;
	}
	else
	{
		return FALSE;
	}

	AROS_LIBFUNC_EXIT
} /* WBenchToFront */
