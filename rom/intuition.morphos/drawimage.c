/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Render an image.
*/

#include <intuition/imageclass.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH4(void, DrawImage,

         /*  SYNOPSIS */
         AROS_LHA(struct RastPort *, rp, A0),
         AROS_LHA(struct Image    *, image, A1),
         AROS_LHA(LONG             , leftOffset, D0),
         AROS_LHA(LONG             , topOffset, D1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 19, Intuition)

/*  FUNCTION
    Draw an image.
 
    INPUTS
    rp - The RastPort to render into
    image - The image to render
    leftOffset, topOffset - Where to place the image.
 
    RESULT
    None.
 
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

    DrawImageState(rp, image, leftOffset, topOffset, IDS_NORMAL, NULL);

    AROS_LIBFUNC_EXIT
} /* DrawImage */
