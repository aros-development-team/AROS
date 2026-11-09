/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function AndRectRegion()
*/

#include "graphics_intern.h"
#include <graphics/regions.h>
#include <proto/exec.h>
#include <clib/macros.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(struct Region *, OrRectRegionND,

/*  SYNOPSIS */
        AROS_LHA(struct Region    *, Reg, A0),
        AROS_LHA(struct Rectangle *, Rect, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 188, Graphics)

/*  FUNCTION
        Add the given Rectangle to the given Region (if not
        already there)

    INPUTS
        region - pointer to Region structure
        rectangle - pointer to Rectangle structure

    RESULT
        The resulting region or NULL in case there's no enough free memory

    NOTES

    BUGS

    SEE ALSO
        AndRegionRegion(), OrRectRegion(), XorRectRegion(), ClearRectRegion()
        NewRegion()

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            graphics_lib.fd and clib/graphics_protos.h
        16-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region *Res = NewRegion();

    if(Res) {
        if(IS_RECT_EVIL(Rect)) {
            /* Source Rect is empty. Make a plain copy of Reg. */
            return _CopyRegionND(Reg, Res, GfxBase);
        } else {
            struct Region R;
            struct RegionRectangle rr;

            _RectToRegion(Rect, &R, &rr);
            return _RegionOpND(_OrBandBand, Reg, &R, Res, GfxBase);
        }
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AndRectRegion */




