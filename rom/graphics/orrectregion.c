/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function OrRectRegion()
    Lang: english
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <proto/exec.h>
#include "intregions.h"
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, OrRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, Reg,  A0),
	AROS_LHA(struct Rectangle *, Rect, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 85, Graphics)

/*  FUNCTION
	Add the given Rectangle to the given Region (if not
	already there)

    INPUTS
	region - pointer to Region structure
	rectangle - pointer to Rectangle structure

    RESULT
	TRUE if the operation was successful, else FALSE
	(out of memory)

    NOTES
	All relevant data is copied, you may throw away the
	given rectangle after calling this function

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion(), XorRectRegion(), ClearRectRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	16-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Region Res;
    struct RegionRectangle rr;

    InitRegion(&Res);

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
} /* OrRectRegion */
