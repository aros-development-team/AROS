/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include "intuition_intern.h"
#include <intuition/classusr.h>
#ifdef __MORPHOS__
#include <clib/alib_protos.h>
#else
#include <proto/alib.h>
#endif

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    EXTENDWORD(leftOffset);
    EXTENDWORD(topOffset);

    SANITY_CHECK(rp)
    SANITY_CHECK(image)

    if (rp->Layer) LockLayer(0,rp->Layer);
    
    if (image != NULL)
    {
        if (image->Depth == CUSTOMIMAGEDEPTH)
        {
            struct impErase method;

    	#ifdef __MORPHOS__
            IPTR penmode;

            GetRPAttrs(rp,RPTAG_PenMode,(ULONG)&penmode,TAG_DONE);
    	#endif

            method.MethodID 	= IM_ERASE;
            method.imp_RPort 	= rp;
            method.imp_Offset.X = leftOffset;
            method.imp_Offset.Y = topOffset;
	    
            DoMethodA ((Object *)image, (Msg)&method);

    	#ifdef __MORPHOS__
            SetRPAttrs(rp,RPTAG_PenMode,penmode,TAG_DONE);
    	#endif
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

    if (rp->Layer) UnlockLayer(rp->Layer);

    AROS_LIBFUNC_EXIT
} /* EraseImage */
