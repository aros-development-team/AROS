/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function ViewPortAddress()
    Lang: english
*/
#include <graphics/view.h>
#include <intuition/intuition.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(struct ViewPort *, ViewPortAddress,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, Window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 50, Intuition)

/*  FUNCTION
	Returns the address of the viewport of a given window. Use this
	call, if you want to use any graphics, text or animation functions
	that require the address of a viewport for your window.
	
    INPUTS
	Window - pointer to a Window structure

    RESULT
	Address of the Intuition ViewPort structure for the screen that your
	window is displayed on.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	graphics.library

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct IntuitionBase *, IntuitionBase)

  return &(Window->WScreen->ViewPort);

  AROS_LIBFUNC_EXIT
} /* ViewPortAddress */
