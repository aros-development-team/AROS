/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function SetPointer()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH6(void, SetPointer,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(UWORD         *, pointer, A1),
	AROS_LHA(LONG           , height, D0),
	AROS_LHA(LONG           , width, D1),
	AROS_LHA(LONG           , xOffset, D2),
	AROS_LHA(LONG           , yOffset, D3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 45, Intuition)

/*  FUNCTION
	Changes the shape of the mouse pointer for a given window.

    INPUTS
	window - Change it for this window
	pointer - The shape of the new pointer as a bitmap with depth 2.
	height - Height of the pointer
	width - Width of the pointer (must be <= 16)
	xOffset, yOffset - The offset of the "hot spot" relative to the
		left, top edge of the bitmap.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ClearPointer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/SetPointer()
    aros_print_not_implemented ("SetPointer");

    if( window )
    {
	window->Pointer = pointer;
    }

    /* Call driver's function */

    AROS_LIBFUNC_EXIT
} /* SetPointer */
