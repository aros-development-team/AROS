/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/10/23 15:33:51  aros
    Three new functions: DrawImageState(), EraseImage() and PointInImage()
    by C. Aldi.

    First version of IMAGECLASS by C. Aldi


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <intuition/classusr.h>
#include <clib/alib_protos.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <intuition/intuition.h>
	#include <intuition/imageclass.h>
	#include <clib/intuition_protos.h>

	__AROS_LH6(void, DrawImageState,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp,         A0),
	__AROS_LHA(struct Image    *, image,      A1),
	__AROS_LHA(long             , leftOffset, D0),
	__AROS_LHA(long             , topOffset,  D1),
	__AROS_LHA(ULONG            , state,      D2),
	__AROS_LHA(struct DrawInfo *, drawInfo,   A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 103, Intuition)

/*  FUNCTION
	This function renders an image in a certain state.

    INPUTS
	rp - Render in this RastPort
	image - Render this image
	leftOffset, topOffset - Add this offset to the position stored in the
		image.
	state - Which state (see intuition/imageclass.h for possible
		valued).
	drawInfo - The DrawInfo from the screen.

    RESULT
	None.

    NOTES
	DrawImageState(), handles both boopsi and conventional images.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (image != NULL)
    {
	if (image->Depth == CUSTOMIMAGEDEPTH)
	{
	    DoMethod ((Object *)image
		, IM_DRAW
		, rp
		, (WORD)leftOffset
		, (WORD)topOffset
		, state
		, drawInfo
	    );
	}
	else
	{
	    DrawImage (rp, image, leftOffset, topOffset);
	}
    }

    __AROS_FUNC_EXIT
} /* DrawImageState */
