/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
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
    BOOL    	    	result = FALSE;
    WORD    	    	X = (point >> 16L);
    WORD    	    	Y =  point & 0x0000FFFFL;
    struct impHitTest 	method;

    if (image != NULL)
    {
        if (image->Depth == CUSTOMIMAGEDEPTH)
        {
            method.MethodID    = IM_HITTEST;
            method.imp_Point.X = X;
            method.imp_Point.Y = Y;
	    
            result = DoMethodA((Object *)image, (Msg)&method) != 0;
        }
        else
        {

            if ((X >= image->LeftEdge && X <= image->LeftEdge + image->Width) &&
                (Y >= image->TopEdge  && Y <= image->TopEdge  + image->Height))
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
