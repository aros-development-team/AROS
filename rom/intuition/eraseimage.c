/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1997/01/27 00:36:37  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:03  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/11/08 11:28:01  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.2  1996/10/24 15:51:19  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/23 15:33:51  aros
    Three new functions: DrawImageState(), EraseImage() and PointInImage()
    by C. Aldi.

    First version of IMAGECLASS by C. Aldi


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <intuition/classusr.h>
#include <proto/graphics.h>
#include <proto/alib.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <proto/intuition.h>

	AROS_LH4(void, EraseImage,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(struct Image    *, image, A1),
	AROS_LHA(LONG             , leftOffset, D0),
	AROS_LHA(LONG             , topOffset, D1),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

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

    AROS_LIBFUNC_EXIT
} /* EraseImage */
