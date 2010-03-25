/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MrgCop()
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <graphics/rastport.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, MrgCop,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 35, Graphics)

/*  FUNCTION
        Merge together the display, color, sprite and user coprocessor
		instructions into a single coprocessor instruction stream.
		
    INPUTS
        view - a pointer to the view structure whos coprocessor instructions
		       are to be merged.

    RESULT
        error - ULONG error value indicating either lack of memory to build the system copper lists,
		        or that MrgCop() has no work to do - ie there where no viewPorts in the list.

    NOTES
        Pre-v39 AmigaOS returns void.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	AROS currently doesn't run on Amiga hardware, so we don't work with real copperlists. However
	we try to behave as if we work with them. So if the view is set as active, we immediately apply
	all changes. Otherwise we just perform some validation.

	Currently AROS doesn't have support for screens composition. Only one screen is visible, and
	it's the frontmost one.	The frontmost Intuition screen corresponds to the first ViewPort in
	the view (see intuition/rethinkdisplay.c)

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPort *vp;
    struct HIDD_ViewPortData *vpd = NULL;
    struct HIDD_ViewPortData *prev_vpd;

    /* Build the list of displayed bitmaps */
    for (vp = view->ViewPort; vp; vp = vp->Next) {
        if (!(vp->Modes & VP_HIDE)) {
	    /* We don't check against NULL because MakeVPort() has already took care about this.
	       If MrgCop() was called with some mailformed ViewPorts, it's not our problem, */
	    prev_vpd = vpd;
	    vpd = VPE_DATA((struct ViewPortExtra *)GfxLookUp(vp));
	    vpd->Next = NULL;
	    if (prev_vpd)
	        prev_vpd->Next = vpd;
	}
    }

    /* If the given view is a currently displayed one, refresh immediately */
    if (GfxBase->ActiView == view)
        driver_LoadView(view, GfxBase);

    return vpd ? MCOP_OK : MCOP_NOP;

    AROS_LIBFUNC_EXIT
} /* MrgCop */
