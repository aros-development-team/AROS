/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function FreeVPortCopLists()
*/

#include <aros/debug.h>
#include <graphics/copper.h>
#include <graphics/view.h>
#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*
 * Compatibility: on Kickstart the display runs the merged copper list built
 * by MrgCop(), so FreeVPortCopLists() on a ViewPort of the displayed View does
 * not change the picture until the next MrgCop()/LoadView(). Several old
 * programs rely on this: they free and rebuild the copper lists of their
 * displayed ViewPort (FreeVPortCopLists() + MakeVPort()) without loading the
 * View again. Here the driver displays the ViewPort's own lists, so freeing
 * them blanks the display. Keep the driver data of a displayed ViewPort;
 * MakeVPort() then replaces its lists the safe way, and they are released
 * when the ViewPort leaves the display.
 */
static BOOL IsDisplayedViewPort(struct ViewPort *vp, struct GfxBase *GfxBase)
{
    struct ViewPort *dvp;
    BOOL found = FALSE;

    ObtainSemaphoreShared(GfxBase->ActiViewCprSemaphore);
    if (GfxBase->ActiView)
    {
        for (dvp = GfxBase->ActiView->ViewPort; dvp; dvp = dvp->Next)
        {
            if (dvp == vp)
            {
                found = !(vp->Modes & VP_HIDE);
                break;
            }
        }
    }
    ReleaseSemaphore(GfxBase->ActiViewCprSemaphore);

    return found;
}

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, FreeVPortCopLists,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 90, Graphics)

/*  FUNCTION
        Deallocate all memory associated with the CopList structures
        for display, color, sprite and the user copper list. The
        corresponding fields in this structure will be set to NULL.

    INPUTS
        vp - pointer to a ViewPort structure

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        graphics/view.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPortExtra *vpe;

    vpe = (struct ViewPortExtra *)GfxLookUp(vp);
    D(bug("[FreeVPortCopLists] ViewPort 0x%p, ViewPortExtra 0x%p\n", vp, vpe));
    if(vpe && IsDisplayedViewPort(vp, GfxBase)) {
        D(bug("[FreeVPortCopLists] ViewPort is displayed, keeping its driver data\n"));
        return;
    }
    if(vpe) {
        /* Free the associated DriverData */
        struct HIDD_ViewPortData *vpd = VPE_DATA(vpe);

        if(vpd) {
            /* Do driver-specific cleanup */
            HIDD_Display_DeinitViewPort(VPE_DRIVER(vpe)->display_obj, vpd);

            if(vpe->Flags & VPXF_WRAPPED_BITMAP) {
                OOP_DisposeObject(vpd->Bitmap);
                vpe->Flags &= ~VPXF_WRAPPED_BITMAP;
            }

            if(vpd->PreviousBitmap) {
                if(vpd->Flags & HIDD_VPDF_PREVIOUS_BITMAP_OWNED)
                    OOP_DisposeObject(vpd->PreviousBitmap);
                vpd->PreviousBitmap = NULL;
                vpd->Flags &= ~HIDD_VPDF_PREVIOUS_BITMAP_OWNED;
            }

            FreeMem(vpd, sizeof(struct HIDD_ViewPortData));

            vpe->DriverData[0] = NULL;
        }

        if(vpe->Flags & VPXF_FREE_ME) {
            D(bug("[FreeVPortCopLists] Freeing temporary ViewPortExtra\n"));
            GfxFree(&vpe->n);
        }
    }

    AROS_LIBFUNC_EXIT
} /* FreeVPortCopLists */
