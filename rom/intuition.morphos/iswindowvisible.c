/*
	(C) 1995-99 AROS - The Amiga Research OS
	$Id$
 
	Desc: Intuition function IsWindowVisible()
	Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************
 
	NAME */
#include <proto/intuition.h>

AROS_LH1(LONG, IsWindowVisible,

		 /*  SYNOPSIS */
		 AROS_LHA(struct Window *, window, A0),

		 /*  LOCATION */
		 struct IntuitionBase *, IntuitionBase, 139, Intuition)

/*  FUNCTION
		Check whether a window is visible or not. This does not
		check whether the window is within the visible area of
		the screen but rather whether it is in visible state.
		
 
	INPUTS
	window - The window to affect. 
 
	RESULT
		TRUE if window is currently visible, FALSE otherwise.
 
	NOTES
 
	EXAMPLE
 
	BUGS
 
	SEE ALSO
 
	INTERNALS
 
	HISTORY
 
*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

	/* shut up the compiler */
	IntuitionBase = IntuitionBase;

	SANITY_CHECKR(window,FALSE)

	return IsLayerVisible(WLAYER(window));

	AROS_LIBFUNC_EXIT
} /* IsWindowVisible */
