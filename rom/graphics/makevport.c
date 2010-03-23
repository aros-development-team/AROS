/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MakeVPort()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>

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

    /* Attach a temporary ViewPortExtra if needed */
    vpe = GfxLookUp(viewport);
    if (!vpe) {
        vpe = GfxNew(VIEWPORT_EXTRA_TYPE);
	if (!vpe)
	    return MVP_NO_VPE;
	vpe->Flags = VPXF_FREE_ME;
	GfxAssociate(viewport, vpe);
    }

    /* Use ScrollVPort() in order to validate offsets */
    ScrollVPort(viewport);

    /* We don't have copper and don't use copperlists, so nothing left to do */
    return MVP_OK;

    AROS_LIBFUNC_EXIT
} /* MakeVPort */
