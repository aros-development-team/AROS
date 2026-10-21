/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Return the memory occupied by a single bitplane to the system.
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <graphics/view.h>
#include "graphics_intern.h"

/*
 * Compatibility: some Kickstart 1.x era tools ("add21k" and the like, found on
 * many old game disks) free a plane of the displayed Workbench screen with
 * FreeRaster() to gain chip memory. On AROS the planes of a displayed bitmap
 * are part of a larger allocation owned by the display driver, and the display
 * keeps using them, so freeing one of them corrupts the free memory list.
 * FreeRaster() therefore leaves alone any plane of a bitmap in the active View.
 */
static BOOL IsDisplayedPlane(PLANEPTR p, struct GfxBase *GfxBase)
{
    struct ViewPort *vp;
    BOOL found = FALSE;
    UWORD i;

    Forbid();
    for (vp = GfxBase->ActiView ? GfxBase->ActiView->ViewPort : NULL; vp && !found; vp = vp->Next)
    {
        struct BitMap *bm = vp->RasInfo ? vp->RasInfo->BitMap : NULL;

        if (bm == NULL)
            continue;
        for (i = 0; i < 8; i++)
        {
            if (bm->Planes[i] == p)
            {
                found = TRUE;
                break;
            }
        }
    }
    Permit();

    return found;
}

/*****************************************************************************

    NAME */
#include <graphics/gfx.h>
#include <proto/graphics.h>

        AROS_LH3(void, FreeRaster,

/*  SYNOPSIS */
        AROS_LHA(PLANEPTR, p,      A0),
        AROS_LHA(UWORD,    width,  D0),
        AROS_LHA(UWORD,    height, D1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 83, Graphics)

/*  FUNCTION
        Free the single bitplane allocated by AllocRaster().

    INPUTS
        p - The result of AllocRaster(). Must be non-NULL.
        width, height - The size of the bitplane as passed to AllocRaster().

    RESULT
        The memory occupied by the bitplane will be returned to the system.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocRaster(), AllocBitMap(), FreeBitMap()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (IsDisplayedPlane(p, GfxBase))
        return;

    FreeMem(p, RASSIZE(width, height));

    AROS_LIBFUNC_EXIT
} /* FreeRaster */
