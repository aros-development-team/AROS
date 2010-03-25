/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MakeVPort()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/view.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(ULONG, MakeVPort,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A0),
        AROS_LHA(struct ViewPort *, viewport, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 36, Graphics)

/*  FUNCTION

    INPUTS
        view     - pointer to a View structure
        viewport - pointer to a ViewPort structure
                   the viewport must have a valid pointer to a RasInfo

    RESULT
        error -

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct ViewPortExtra *vpe;
    OOP_Object *bm = OBTAIN_HIDD_BM(viewport->RasInfo->BitMap);

    if (!bm)
        return MVP_NO_MEM;

    /* Attach a temporary ViewPortExtra if needed */
    vpe = GfxLookUp(viewport);
    D(bug("[MakeVPort] ViewPort 0x%p, ViewPortExtra 0x%p\n", viewport, vpe));
    if (!vpe) {
        vpe = GfxNew(VIEWPORT_EXTRA_TYPE);
	if (!vpe) {
	    RELEASE_HIDD_BM(bm, viewport->RasInfo->BitMap);
	    return MVP_NO_VPE;
	}
	vpe->Flags = VPXF_FREE_ME;
	GfxAssociate(viewport, vpe);
    }

    /* Store bitmap object in the ViewPortExtra */
    if (!VPE_BITMAP(vpe)) {
        D(bug("[MakeVPort] Old bitmap object: 0x%p\n", VPE_BITMAP(vpe)));
        RELEASE_HIDD_BM(VPE_BITMAP(vpe), viewport->RasInfo->BitMap);
    }
    VPE_DATA(vpe)->Bitmap = bm;
    D(bug("[MakeVPort] New bitmap object: 0x%p\n", VPE_BITMAP(vpe)));

    /* Use ScrollVPort() in order to validate offsets */
    ScrollVPort(viewport);

    return MVP_OK;

    AROS_LIBFUNC_EXIT
} /* MakeVPort */
