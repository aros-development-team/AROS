/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  2000/11/17 23:50:41  stegerg
    follow image->NextImage. Has still to be fixed/changed if
    it is possible that one calls DrawImageState with mixed
    normal/boopsi images in the list.

    Revision 1.6  1998/10/20 16:45:54  hkiel
    Amiga Research OS

    Revision 1.5  1997/01/27 00:36:37  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:02  aros
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
