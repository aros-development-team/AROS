/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function ScrollVPort()
*/

#include <aros/debug.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "graphics_display.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, ScrollVPort,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 98, Graphics)

/*  FUNCTION
        Move the ViewPort to the position specified in DxOffset and DyOffset
        members of the ViewPort structure.

    INPUTS

    RESULT
        None.

    NOTES
        AROS video drivers can perform a validation of offsets, and may refuse
        to scroll the screen too far (if they somehow can't provide the requested
        offset). In this case offset values in the ViewPort will be updated in
        order to reflect the real result of the operation.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * Bitmap object pointer is contained in struct ViewPortData,
     * connected to a ViewPortExtra.
     * This is true even for planar Amiga bitmaps.
     */
    struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxLookUp(vp);

    if(vpe) {
        OOP_Object *bm = VPE_DATA(vpe)->Bitmap;
        struct monitor_displaydata *mdd = (struct monitor_displaydata *)VPE_DRIVER(vpe);
        IPTR x = vp->DxOffset;
        IPTR y = vp->DyOffset;
        BOOL compositing = FALSE;

        D(bug("[ScrollVPort] ViewPort 0x%p, Extra 0x%p, offset (%ld, %ld)\n", vp, vpe, x, y));

        /*
         * First we actually move the bitmap.
         * The display's compositor validates the offsets against its composition
         * rules and writes the result back; if it composes, our bitmap is mirrored
         * into the composed bitmap and the driver framebuffer must not be installed
         * into it.
         */
        OOP_SetAttrsTags(bm, aHidd_BitMap_LeftEdge, x, aHidd_BitMap_TopEdge, y, TAG_DONE);

        compositing = HIDD_Display_ViewPortPositionChange(mdd->mdisplay.display_obj, bm, &x, &y);

        if(compositing) {
            /* Composition is active. Uninstall the framebuffer from the frontmost bitmap */
            display_FBUninstall(mdd, GfxBase);
        } else if(!mdd->bm_bak) {
            /*
             * Composition is inactive. Install the framebuffer into the frontmost
             * bitmap, if not already done. This will actually trigger only once,
             * when composition switched from active to inactive state.
             */
            display_FBInstall(mdd, GfxBase);
        }

        /* The bitmap may fail to move. Fix up offsets now. */
        if(!compositing) {
            /*
             * If software composition is inactive, we have our bitmap on display.
             * Get validated offsets from it.
             */
            OOP_GetAttr(bm, aHidd_BitMap_LeftEdge, &x);
            OOP_GetAttr(bm, aHidd_BitMap_TopEdge, &y);
        }

        D(bug("[ScrollVPort] Resulting offset (%ld, %ld), composition %d\n", x, y, compositing));

        vp->DxOffset = x;
        vp->DyOffset = y;
    }

    AROS_LIBFUNC_EXIT
} /* ScrollVPort */
