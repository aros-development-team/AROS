/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <graphics/rastport.h>
#include <proto/intuition.h>

AROS_LH1(void, ReleaseGIRPort,

         /*  SYNOPSIS */
         AROS_LHA(struct RastPort *, rp, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 94, Intuition)

/*  FUNCTION
    Release a RastPort previously obtained by ObtainGIRPort().
 
    INPUTS
    rp - The result of ObtainGIRPort()
 
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

    DEBUG_RELEASEGIRPORT(dprintf("ReleaseGIRPort: RPort 0x%lx\n", rp));

    if (rp)
    {
        if (rp->Layer)
        {
            if ((--(GetPrivIBase(IntuitionBase)->BackupLayerContext.nestcount)) == 0)
            {
                InstallClipRegion(rp->Layer,GetPrivIBase(IntuitionBase)->BackupLayerContext.clipregion);

                rp->Layer->Scroll_X = GetPrivIBase(IntuitionBase)->BackupLayerContext.scroll_x;
                rp->Layer->Scroll_Y = GetPrivIBase(IntuitionBase)->BackupLayerContext.scroll_y;
            }

            /*  bug("----------- RELEASE: %x\n",rp->Layer);*/
            UnlockLayer(rp->Layer);
        }

        UNLOCKGADGET

        FreeRastPort (rp);
    }

    AROS_LIBFUNC_EXIT
} /* ReleaseGIRPort */
