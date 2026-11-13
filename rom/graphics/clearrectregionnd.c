/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function AndRectRegion()
*/

#include <graphics/regions.h>
#include <proto/exec.h>
#include <clib/macros.h>

#include "graphics_intern.h"
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(struct Region *, ClearRectRegionND,

/*  SYNOPSIS */
        AROS_LHA(struct Region    *, Reg, A0),
        AROS_LHA(struct Rectangle *, Rect, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 192, Graphics)

/*  FUNCTION
        Clear the given Rectangle from the given Region

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

    if(!Res)
        return NULL;

    if(!Reg->RegionRectangle        ||
            IS_RECT_EVIL(Rect)           ||
            !overlap(*Rect, Reg->bounds)) {
        /* Nothing to clear. Make a plain copy. */
        return _CopyRegionND(Reg, Res, GfxBase);
    }
    else if(Rect->MinX > MinX(Reg) || Rect->MinY > MinY(Reg) ||
            Rect->MaxX < MaxX(Reg) || Rect->MaxY < MaxY(Reg)) {
        /* Partial overlapping detected. Do the complete algorithm. */
        struct Region R;
        struct RegionRectangle rr;

        _RectToRegion(Rect, &R, &rr);
        return _RegionOpND(_ClearBandBand, &R, Reg, Res, GfxBase);
    }

    /* If neither of two cases above were true, we've removed everything (Rect completely covers Reg). Return empty Res. */
    return Res;

    AROS_LIBFUNC_EXIT
} /* AndRectRegion */




