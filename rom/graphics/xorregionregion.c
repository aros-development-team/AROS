/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function XorRegionRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, XorRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region1, A0),
	AROS_LHA(struct Region *, region2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 103, Graphics)

/*  FUNCTION
	Exclusive-OR of one region with another region,
	leaving result in second region.

    INPUTS
	region1 - pointer to a region structure
	region2 - pointer to a region structure

    RESULT
	TRUE if the operation was succesful,
        FALSE otherwise (out of memory)

    NOTES


    EXAMPLE

    BUGS

    SEE ALSO
	AndRegionRegion(), OrRegionRegion()

    INTERNALS
        Two regions A and B consist of rectangles a1,...,a3 and b1,...,b3.
        A xor B := A*NOT(B) + NOT(A)*B

	falemagn:

	1) A xor B                                   :=
	2) A*NOT(B) + NOT(A)*B                       :=
	3) A*NOT(A) + A*NOT(B) + B*NOT(B)+ NOT(A)*B  :=
	4) A*(NOT(A) + NOT(B)) + B*(NOT(A) + NOT(B)) :=
	5) (NOT(A) + NOT(B)) * (A+B)                 :=
	6) NOT(A*B) * (A+B)

	X - Y |
	------|
	0   0 | 0
	0   1 | 0
	1   0 | 1
	1   1 | 0

	Hence:
	X - Y := m2 := X * NOT(Y)

	If we set

	7) X := A+B

	and

	8) Y := A*B

	then

	9) X - Y := (A+B) - (A*B) := (A+B) * NOT(A*B)

	But (9) := (6) := (1)

	Hence:

	A xor B := (A+B) - (A*B)

	The "-" operation is given us by ClearRegionRegion

	If we implement (2) then we have to use

	    2 times AndRegionRegion
	    1 time  OrRegionRegion
	    2 times ClearRegionRegion (in XorRectRegion)
	    2 times AndRectRegion     (in XorRectRegion)
	    2 times OrRectRegion      (in XorRectRegion)

	If we implement (9) then we have to use

	    1 time AndRegionRegion
	    1 time OrRegionRegion
	    1 time ClearRegionRegion

	it's evident that (9) is considerably faster than (2)

    HISTORY
	27-11-96    digulla  automatically created from
			     graphics_lib.fd and clib/graphics_protos.h
	19-01-97    mreckt   intital version

        22-09-2001  falemagn changed implementation

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
#if USE_BANDED_FUNCTIONS
    return _XorRegionRegion(region1, region2, GfxBase);

#else

    struct Region* intersection, *copy2;

    if ((intersection = CopyRegion(region2)))
    {
	if ((copy2 = CopyRegion(region2)))
        {
            if (AndRegionRegion(region1, intersection))
	    {
 	        if (OrRegionRegion(region1, region2))
	        {
    	            if (intersection->RegionRectangle)
    		    {
		        BOOL result = ClearRegionRegion(intersection, region2);

		        if (!result)
		        {
		            /* reinstall old RegionRectangles */
			    struct Region tmp;
			    tmp      = *region2;
			    *region2 = *copy2;
			    *copy2    = tmp;
		        }

		        DisposeRegion(intersection);
	                DisposeRegion(copy2);
		        return result;
		    }

		    DisposeRegion(intersection);
	            DisposeRegion(copy2);
		    return TRUE;
    	        }
	    }
	    DisposeRegion(copy2);
	}
        DisposeRegion(intersection);
    }
    return FALSE;

#endif
  AROS_LIBFUNC_EXIT
}
