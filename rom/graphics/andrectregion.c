/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AndRectRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <clib/macros.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(void, AndRectRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region    *, Reg, A0),
	AROS_LHA(struct Rectangle *, Rect, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 84, Graphics)

/*  FUNCTION
	Remove everything inside 'region' that is outside 'rectangle'

    INPUTS
	region - pointer to Region structure
	rectangle - pointer to Rectangle structure

    NOTES
	This is the only *RectRegion function that cannot fail

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

    /* Is the region non-empty? */
    if (Reg->RegionRectangle)
    {
        struct Rectangle OldBounds = Reg->bounds;


	/* Does the rectangle overlap with the region? */
        if (!_AndRectRect(Rect, &OldBounds, &Reg->bounds))
        {
	    /* If not then just clear the region */
            ClearRegion(Reg);
        }
	else
	/* Else check if the rectangle contains the region */
        if (!_AreRectsEqual(Bounds(Reg), &OldBounds))
        {
	    /* The region is not completely contained in the rectangle */

            struct RegionRectangle *rr, *PtrToFirst;
            struct RegionRectangleExt RRE;
            struct Rectangle Rect2;
            LONG OffX, OffY;

            PtrToFirst  = &RRE.RR;

            /*
               Set the counter to its maximum value so that
                  Chunk(rr->Prev)->Rects[SIZERECTBUF-1].RR.Next = NextRR
               can actually work out well.
            */
            RRE.Counter = SIZERECTBUF - 1;

            PtrToFirst->Next = Reg->RegionRectangle;
            Reg->RegionRectangle->Prev = PtrToFirst;

	    Rect2.MinX = Rect->MinX - OldBounds.MinX;
            Rect2.MinY = Rect->MinY - OldBounds.MinY;
            Rect2.MaxX = Rect->MaxX - OldBounds.MinX;
            Rect2.MaxY = Rect->MaxY - OldBounds.MinY;

	    OffX = OldBounds.MinX - MinX(Reg);
	    OffY = OldBounds.MinY - MinY(Reg);

            for
            (
                rr = Reg->RegionRectangle;
                rr;
            )
            {
		struct RegionRectangle *NextRR = rr->Next;

                if (overlap(rr->bounds, Rect2))
                {
                    /*
                       The rectangle overlaps with this RegionRectangle, so calculate the intersection
                       And add the offsets to adjust the result to the new region's bounds
                    */
                    MinX(rr) = MAX(Rect2.MinX, MinX(rr)) + OffX;
                    MaxX(rr) = MIN(Rect2.MaxX, MaxX(rr)) + OffX;
                    MinY(rr) = MAX(Rect2.MinY, MinY(rr)) + OffY;
                    MaxY(rr) = MIN(Rect2.MaxY, MaxY(rr)) + OffY;
                }
                else
                {
		    /* The rectangle doesn't overlap with this RegionRectangle, thus
                       this Regionrectangle has to be eliminated from the region.
                       The way we handle RegionRectangles doesn't let us just free it,
                       we can just adjust the pointers of the previous and successive rectangles
                       to point to each other.
 		    */

		    /* There's always a previous rectangle. Just fix its next pointer */
                    rr->Prev->Next = NextRR;

		    /* Fix the Next rectangle's Prev pointer */
                    if (NextRR)
    		    {
        		NextRR->Prev = rr->Prev;
    		    }

		    /* Is this RegionRectangle the last one in its chunk? */
                    if (Chunk(rr->Prev) != Chunk(rr) && Chunk(NextRR) != Chunk(rr))
    		    {
			/*
                           If so then update the previous chunk's pointer to the next chunk
                           to point to the correct chunk's rectangle.
                        */
                        Chunk(rr->Prev)->Rects[SIZERECTBUF-1].RR.Next = NextRR;
			/* And dispose this chunk. */
	       		_DisposeRegionRectangleExtChunk(Chunk(rr));
    		    }
                }

                rr = NextRR;
            }

            Reg->RegionRectangle = PtrToFirst->Next;
            if (PtrToFirst->Next)
                PtrToFirst->Next->Prev = NULL;
        }
    }

    AROS_LIBFUNC_EXIT
} /* AndRectRegion */




