/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition Function ObtainGIRPort()
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/exec.h>

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

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct RastPort *rp;
    
    if (gInfo)
    {
    	rp = CloneRastPort (gInfo->gi_RastPort);
	if (!rp) return NULL;
	
	ObtainSemaphore(&GetPrivIBase(IntuitionBase)->GadgetLock);

/*	bug("+++++++++++++ OBTAIN rp=%x lay=%x font=%x origfont=%x\n",rp,rp->Layer,rp->Font,gInfo->gi_RastPort->Font); */
	
	LockLayerRom(rp->Layer);

	if (((GetPrivIBase(IntuitionBase)->BackupLayerContext.nestcount)++) == 0)
	{
	    GetPrivIBase(IntuitionBase)->BackupLayerContext.scroll_x = rp->Layer->Scroll_X;
	    GetPrivIBase(IntuitionBase)->BackupLayerContext.scroll_y = rp->Layer->Scroll_Y;

	    rp->Layer->Scroll_X = 0;
	    rp->Layer->Scroll_Y = 0;

	    GetPrivIBase(IntuitionBase)->BackupLayerContext.clipregion = InstallClipRegion(rp->Layer, NULL);
	}
	
	SetDrMd(rp, JAM1),
	SetWriteMask(rp, 0xFF);

	return rp;
    }
    else
    {
    	return NULL;
    }
    
    AROS_LIBFUNC_EXIT
} /* ObtainGIRPort */
