/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/10/23 15:33:52  aros
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
	#include <intuition/intuition.h>
	#include <intuition/imageclass.h>
	#include <clib/intuition_protos.h>

	__AROS_LH2(BOOL, PointInImage,

/*  SYNOPSIS */
	__AROS_LHA(ULONG,          point, D0),
	__AROS_LHA(struct Image *, image, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 104, Intuition)

/*  FUNCTION
	Check whether a point is inside an image.

    INPUTS
	point - This are the packed point coordinates. The X coordinate
		in in the upper 16 bits and the Y coordinate is in the
		lower 16 bits. The coordinates are signed.
	image - Check against this image.

    RESULT
	TRUE is the point is inside the image, FALSE otherwise.

    NOTES

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
    BOOL result;
    WORD X = (point >> 16L);
    WORD Y =  point & 0x0000FFFFL;

    if (image != NULL)
    {
	if (image->Depth == CUSTOMIMAGEDEPTH)
	{
	    /* I'm making a possibly wrong assumption here regarding the
	     * X/Y point structure and the packed word order of the point.
	     */
	    result = (BOOL)DoMethod((Object *)image,
		IM_HITTEST, (WORD)X, (WORD)Y
	    );
	}
	else
	{

	    if ((X >= image->LeftEdge && X <= image->LeftEdge + image->Width) &&
		(Y >= image->TopEdge  && Y <= image->TopEdge  + image->Height)
	    )
	    {
		result = TRUE;
	    }
	}
    }
    else
    {
	/* NULL image returns TRUE per intuition autodoc! */
	result = TRUE;
    }

    return (result);
    __AROS_FUNC_EXIT
} /* PointInImage */
