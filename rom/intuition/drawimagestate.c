/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <intuition/classusr.h>
#include <proto/alib.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <proto/intuition.h>

	AROS_LH6(void, DrawImageState,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp,         A0),
	AROS_LHA(struct Image    *, image,      A1),
	AROS_LHA(LONG             , leftOffset, D0),
	AROS_LHA(LONG             , topOffset,  D1),
	AROS_LHA(ULONG            , state,      D2),
	AROS_LHA(struct DrawInfo *, drawInfo,   A2),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (image != NULL)
    {
	if (image->Depth == CUSTOMIMAGEDEPTH)
	{
	    while(image)
	    {
		DoMethod ((Object *)image
		    , IM_DRAW
		    , rp
		    , (WORD)leftOffset
		    , (WORD)topOffset
		    , state
		    , drawInfo
		);
		
		image = image->NextImage;
	    };
	}
	else
	{
	    DrawImage (rp, image, leftOffset, topOffset);
	}
    }

    AROS_LIBFUNC_EXIT
} /* DrawImageState */
