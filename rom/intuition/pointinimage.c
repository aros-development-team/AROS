/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1997/01/27 00:36:42  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:07  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:23  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/23 15:33:52  aros
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
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <proto/intuition.h>

	AROS_LH2(BOOL, PointInImage,

/*  SYNOPSIS */
	AROS_LHA(ULONG,          point, D0),
	AROS_LHA(struct Image *, image, A0),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
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
    AROS_LIBFUNC_EXIT
} /* PointInImage */
