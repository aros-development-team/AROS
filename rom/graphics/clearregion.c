/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function ClearRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/exec.h>
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH1(void, ClearRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 88, Graphics)

/*  FUNCTION
	Removes all rectangles in the specified region.

    INPUTS
	region - pointer to the region structure

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	15-01-97    mreckt  initial version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(region);

#if REGIONS_HAVE_RRPOOL
    struct RegionRectanglePool *Pool;

    if (region->RectPoolList)
    {
        while ((Pool = (struct RegionRectanglePool *)RemHead((struct List *)region->RectPoolList)))
        {

            ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);

            FreePooled
            (
                PrivGBase(GfxBase)->regionpool,
                Pool->RectArray,
                SIZERECTBUF * sizeof(struct RegionRectangleExt)
            );

            FreePooled
            (
                PrivGBase(GfxBase)->regionpool,
                Pool,
                sizeof(struct RegionRectanglePool)
            );

            ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);
        }

        ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);

        FreePooled
        (
            PrivGBase(GfxBase)->regionpool,
            region->RectPoolList,
            sizeof(struct MinList)
        );

        ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem);
    }

#else
    DisposeRegionRectangleList(region->RegionRectangle);
#endif

    InitRegion(region);

    AROS_LIBFUNC_EXIT
} /* ClearRegion */






