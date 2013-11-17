/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/exec.h>
#include <graphics/rpattr.h>

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <intuition/cghooks.h>
#include <proto/intuition.h>

        AROS_LH1(struct RastPort *, ObtainGIRPort,

/*  SYNOPSIS */
        AROS_LHA(struct GadgetInfo *, gInfo, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 93, Intuition)

/*  FUNCTION
        This function sets up a RastPort for exclusive use by custom
        gadget hook routines. Call this function each time a hook
        routine needs to render into the gadget and ReleaseGIRPort()
        immediately afterwards.

    INPUTS
        gInfo - Pointer to GadgetInfo structure, as passed to each
            custom gadget hook function.

    RESULT
        Pointer to a RastPort you can render to. NULL if you aren't
        allowed to render into this gadget.

    NOTES
        If a routine passes a RastPort, eg. GM_RENDER, ObtainGIRPort()
        needn't be called.

    EXAMPLE

    BUGS

    SEE ALSO
        ReleaseGIRPort()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct RastPort *rp;

    DEBUG_OBTAINGIRPORT(dprintf("ObtainGIRPort: GInfo 0x%lx Task 0x%lx <%s>\n",
                                gInfo, FindTask(NULL),
                                FindTask(NULL)->tc_Node.ln_Name ? FindTask(NULL)->tc_Node.ln_Name : "NULL"));

    if (gInfo && gInfo->gi_RastPort)
    {
        rp = CloneRastPort (gInfo->gi_RastPort);
        if (rp)
        {

            LOCKGADGET(IntuitionBase)
            /*bug("+++++++++++++ OBTAIN rp=%p lay=%p font=%p origfont=%p\n",rp,rp->Layer,rp->Font,gInfo->gi_RastPort->Font);*/

            if (rp->Layer)
            {
                LockLayer(0,rp->Layer);

                if (((GetPrivIBase(IntuitionBase)->BackupLayerContext.nestcount)++) == 0)
                {
                    GetPrivIBase(IntuitionBase)->BackupLayerContext.scroll_x = rp->Layer->Scroll_X;
                    GetPrivIBase(IntuitionBase)->BackupLayerContext.scroll_y = rp->Layer->Scroll_Y;

                    rp->Layer->Scroll_X = 0;
                    rp->Layer->Scroll_Y = 0;

                    GetPrivIBase(IntuitionBase)->BackupLayerContext.clipregion = InstallClipRegion(rp->Layer, NULL);
                }

                SetWriteMask(rp, 0xFF);
                SetRPAttrs(rp,RPTAG_DrMd,JAM1,RPTAG_PenMode,TRUE,TAG_DONE);
            }
        }

        DEBUG_OBTAINGIRPORT(dprintf("ObtainGIRPort: RPort 0x%lx\n", rp));
        return rp;
    }
    else
    {
        return NULL;
    }

    AROS_LIBFUNC_EXIT
} /* ObtainGIRPort */
