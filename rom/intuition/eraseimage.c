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
#include <clib/graphics_protos.h>
#include <clib/alib_protos.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <intuition/intuition.h>
	#include <intuition/imageclass.h>
	#include <clib/intuition_protos.h>

	__AROS_LH4(void, EraseImage,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A0),
	__AROS_LHA(struct Image    *, image, A1),
	__AROS_LHA(long             , leftOffset, D0),
	__AROS_LHA(long             , topOffset, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 105, Intuition)

/*  FUNCTION
	Erase an image on the screen.

    INPUTS
	rp - Render in this RastPort
	image - Erase this image
	leftOffset, topOffset - Add this offset the the position in the
		image.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	DrawImage(), DrawImageState()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h
	23-10.96    aldi    commited the code

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (image != NULL)
    {
	if (image->Depth == CUSTOMIMAGEDEPTH)
	{
		DoMethod ((Object *)image,
		    IM_ERASE, rp, (WORD)leftOffset, (WORD)topOffset
		);
	}
	else
	{
	    EraseRect (rp,
		leftOffset + image->LeftEdge,
		topOffset  + image->TopEdge,
		leftOffset + image->LeftEdge + image->Width,
		topOffset  + image->TopEdge  + image->Height
	    );
	}
    }

    __AROS_FUNC_EXIT
} /* EraseImage */
