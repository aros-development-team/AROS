/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function XorRegionRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

#define NEW_IMPLEMENTATION 1

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

#if NEW_IMPLEMENTATION
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

#else

  struct Rectangle R;
  struct Region Work1, Work2;

  /* First I have to create a backup of the whole region1 and 2 */
  Work1.bounds = region1->bounds;
  Work2.bounds = region2->bounds;

  /* make a duplcate of region1 and region 2 */
  if (NULL != region1->RegionRectangle)
  {
    if (NULL == (Work1.RegionRectangle = copyrrects(region1->RegionRectangle)))
    {
      /* no more memory */
      return FALSE;
    }
  }
  else
  {
    /* region1 is empty. Nothing to do as region2 remains unchanged */
    return TRUE;
  }

  if (NULL != region2->RegionRectangle)
  {
    if (NULL == (Work2.RegionRectangle = copyrrects(region2->RegionRectangle)))
    {
      /* no more memory */
      /* free duplicate of region1 */
      ClearRegion(&Work1);
      return FALSE;
    }
  }
  else
  {
    /* region2 empty. There *is* something to do! */
    Work2.RegionRectangle = NULL;
  }

  /* The rectangle R has to cover the biggest part possible */

  if (region1->bounds.MinX < region2->bounds.MinX)
    R.MinX = region1->bounds.MinX;
  else
    R.MinX = region2->bounds.MinX;

  if (region1->bounds.MinY < region2->bounds.MinY)
    R.MinY = region1->bounds.MinY;
  else
    R.MinY = region2->bounds.MinY;

  if (region1->bounds.MaxX > region2->bounds.MaxX)
    R.MaxX = region1->bounds.MaxX;
  else
    R.MaxX = region2->bounds.MaxX;

  if (region1->bounds.MaxY > region2->bounds.MaxY)
    R.MaxY = region1->bounds.MaxY;
  else
    R.MaxY = region2->bounds.MaxY;


  /* I need not(BackupRegion2) now and will also calculate
     region1 AND NOT(region2) in the next step     */

  if (FALSE == XorRectRegion(&Work2, &R) /* NOT(region2) */ ||
      FALSE == AndRegionRegion(region1, &Work2)  /* reg1 and NOT(reg2) */)
  {
    /* no more memory - region1 and region2 are still correct */
    ClearRegion(&Work1);
    ClearRegion(&Work2);
    return FALSE;
  }



  /* Work2 now contains reg1 and NOT(reg2), the first part */

  /* create NOT(reg1) and       NOT(reg1) AND reg2   in the next step */

  if (FALSE == XorRectRegion(&Work1, &R) /* NOT(region1) */ ||
      FALSE == AndRegionRegion(region2, &Work1) /* NOT(reg1) and reg2 */)
  {
    /* no more memory, region1 and region2 are still correct */
    ClearRegion(&Work1);
    ClearRegion(&Work2);
    return FALSE;
  }

  /* combine the two parts: (NOT(reg1) * reg2) + (reg1 * NOT(reg2)) */
  if (FALSE == OrRegionRegion(&Work2, &Work1))
  {
    /* no more memory, region1 and region2 are still correct */
    ClearRegion(&Work1);
    ClearRegion(&Work2);
    return FALSE;
  }

  ClearRegion(&Work2);

  /* Work1 contains the result now, nothing went wrong. So i can dispose whatever
     was in region2 */
  ClearRegion(region2);

  region2->bounds          = Work1.bounds;
  region2->RegionRectangle = Work1.RegionRectangle;

  /* must not free Work2 as it is the result that is in region2 now */

  /* everything went alright */
  return TRUE;

#endif

  AROS_LIBFUNC_EXIT
}
