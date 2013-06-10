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
        Prepare the view for being displayed. Calculate necessary internal data.
        For Amiga(tm) chipset this function also merges together the display, color, sprite and user
        coprocessor instructions into a single coprocessor instruction stream.

    INPUTS
        view - a pointer to the view structure to prepare

    RESULT
        error - ULONG error value indicating either lack of memory to build the system data,
                        or that MrgCop() has no work to do - ie there where no viewPorts in the list.

    NOTES
        Pre-v39 AmigaOS returns void.
        
        If the given view is already on display, changes appear immediately.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPort *first, *vp;
    struct HIDD_ViewPortData *vpd;
    struct HIDD_ViewPortData *prev_vpd;
    struct monitor_driverdata *mdd;
    struct monitor_driverdata *prev_mdd = NULL;
    ULONG ret;

    /* Build lists of displayed bitmaps, one list per display.
       Lists are embedded in ViewPortExtra.DriverData field. Start of
       the list is the first ViewPort for this display. */
    for (first = view->ViewPort; first; first = first->Next) {
        /* Ignore hidden ViewPorts */
        if (!(first->Modes & VP_HIDE)) {

            mdd = GET_VP_DRIVERDATA(first);
            /* Ignore next ViewPort if it belongs to the same display.
               This makes us slightly faster */
            if (mdd == prev_mdd)
                continue;
            prev_mdd = mdd;

            /* We don't check GfxLookUp() result against NULL because MakeVPort() has
               already took care about this. If MrgCop() was called with some mailformed
               ViewPorts, it's not our problem */
            prev_vpd = VPE_DATA((struct ViewPortExtra *)GfxLookUp(first));
            prev_vpd->Next = NULL;
            D(bug("[MrgCop] First ViewPort: 0x%p, data 0x%p\n", first, prev_vpd));

            /* Now we look down the list for ViewPorts with the same display driver as
               current ViewPort and add them to a list */
            for (vp = first->Next; vp; vp = vp->Next) {
                if (!(vp->Modes & VP_HIDE)) {

                    if (GET_VP_DRIVERDATA(vp) == mdd) {
                        vpd = VPE_DATA((struct ViewPortExtra *)GfxLookUp(vp));
                        D(bug("[MrgCop] Attaching ViewPort 0x%p, data 0x%p\n", vp, vpd));

                        vpd->Next = NULL;
                        prev_vpd->Next = vpd;
                        prev_vpd = vpd;
                    }
                }
            }
        }
    }

    if (prev_mdd)
    {
        ret = DoViewFunction(view, driver_PrepareViewPorts, GfxBase);

        if (ret == MCOP_OK)
        {
            ObtainSemaphore(GfxBase->ActiViewCprSemaphore);

            /* If the given view is a currently displayed one, refresh immediately */
            if (GfxBase->ActiView == view)
                DoViewFunction(view, driver_LoadViewPorts, GfxBase);

            ReleaseSemaphore(GfxBase->ActiViewCprSemaphore);
        }
    }
    else
        ret = MCOP_NOP;

    return ret;

    AROS_LIBFUNC_EXIT
} /* MrgCop */
