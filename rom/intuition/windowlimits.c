/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.6  2000/01/19 19:04:42  stegerg
    don't call intui_windowlimits

    Revision 1.5  1998/10/20 16:46:08  hkiel
    Amiga Research OS

    Revision 1.4  1998/01/08 21:12:13  hkiel
    Added intui_WindowLimits().

    Revision 1.3  1997/12/29 21:18:41  hkiel
    Supplied WindowLimits() function with life.


    Desc: Set the minimum and maximum size of a window.
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH5(BOOL, WindowLimits,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(WORD,            MinWidth, D0),
	AROS_LHA(WORD,            MinHeight, D1),
	AROS_LHA(UWORD,           MaxWidth, D2),
	AROS_LHA(UWORD,           MaxHeight, D3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 53, Intuition)

/*  FUNCTION
	This functions sets the minimum and maximum sizes of a window.

    INPUTS
	Window - window to change
	MinWidth, MinHeight - the minimum size, may be 0 for no change
	MaxWidth, MaxHeight - the maximum size, may be 0 for no change,
	    may be -1 for no maximum size

    RESULT
	A boolean. FALSE is returned if any of the provided sizes is out
	of range. Note that the other sizes take effect, though. TRUE if
	all sizes could be set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenWindow()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    BOOL retval = TRUE;

    if(window->Width >= MinWidth)
	window->MinWidth = MinWidth;
    else
	retval = FALSE;

    if(window->Height >= MinHeight)
	window->MinHeight = MinHeight;
    else
	retval = FALSE;

    if(window->Width <= MaxWidth)
	window->MaxWidth = MaxWidth;
    else
	retval = FALSE;

    if(window->Height <= MaxHeight)
	window->MaxHeight = MaxHeight;
    else
	retval = FALSE;

    return retval;
    AROS_LIBFUNC_EXIT
} /* WindowLimits */
