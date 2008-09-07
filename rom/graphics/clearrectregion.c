/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ClearRectRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include <proto/graphics.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(BOOL, ClearRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, Reg   , A0),
	AROS_LHA(struct Rectangle *, Rect, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 87, Graphics)

/*  FUNCTION
	Clear the given Rectangle from the given Region

    INPUTS
	region - pointer to a Region structure
	rectangle - pointer to a Rectangle structure

    RESULT
	FALSE if not enough memory was available, else TRUE

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion(), OrRectRegion(), XorRectRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	20-01-96    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region Res;
    struct RegionRectangle rr;

    /* If the rectangle and the region don't overlap just return */
    if (!overlap(*Rect, Reg->bounds))
        return TRUE;

    /* if the rectangle completely covers the region just clear the region and return */
    if
    (
        Rect->MinX <= MinX(Reg) &&
        Rect->MinY <= MinY(Reg) &&
        Rect->MaxX >= MaxX(Reg) &&
        Rect->MaxY >= MaxY(Reg)
    )
    {
        ClearRegion(Reg);
        return TRUE;
    }

    InitRegion(&Res);

    rr.bounds = *Rect;
    rr.Next   = NULL;
    rr.Prev   = NULL;

    if
    (
        _DoOperationBandBand
        (
            _ClearBandBand,
            0,
            MinX(Reg),
            0,
	    MinY(Reg),
            &rr,
            Reg->RegionRectangle,
            &Res.RegionRectangle,
            &Res.bounds,
            GfxBase
        )
    )
    {
	ClearRegion(Reg);

        *Reg = Res;

        _TranslateRegionRectangles(Res.RegionRectangle, -MinX(&Res), -MinY(&Res));

        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* ClearRectRegion */
