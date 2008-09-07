/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AndRectRegion()
    Lang: english
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
	struct GfxBase *, GfxBase, 123, Graphics)

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

    if (Res)
    {

        struct RegionRectangle rr;

        rr.bounds = *Rect;
        rr.Next   = NULL;
        rr.Prev   = NULL;

        if
        (
            _DoOperationBandBand
            (
                _OrBandBand,
                MinX(Reg),
                0,
	        MinY(Reg),
                0,
                Reg->RegionRectangle,
                &rr,
                &Res->RegionRectangle,
                &Res->bounds,
                GfxBase
            )
        )
        {

            _TranslateRegionRectangles(Res->RegionRectangle, -MinX(Res), -MinY(Res));

            return Res;
        }

        DisposeRegion(Res);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AndRectRegion */




