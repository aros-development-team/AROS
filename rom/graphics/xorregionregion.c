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

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h
	19-01-97    mreckt  intital version

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  struct Rectangle R;
  struct Region Work1, Work2;

  /* First I have to create a backup of the whole region1 and 2 */
  Work1.bounds = region1->bounds;
  Work2.bounds = region2->bounds;
  
  /* make a duplcate of region1 and region 2 */
  if (NULL == (Work1.RegionRectangle = copyrrects(region1->RegionRectangle)))
  {
    /* no more memory */
    return FALSE;
  }

  if (NULL == (Work2.RegionRectangle = copyrrects(region2->RegionRectangle)))
  {
    /* no more memory */
    /* free duplicate of region1 */
    ClearRegion(&Work1);
    return FALSE;
  }


  /* I need not(BackupRegion2) now and will also calculate 
     region1 AND NOT(region2) in the next step     */
  R = Work2.bounds;
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
  R = Work1.bounds;
  if (FALSE == XorRectRegion(&Work1, &R) /* NOT(region1) */ ||
      FALSE == AndRegionRegion(region1, &Work1) /* NOT(reg1) and reg2 */)
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

  region2->bounds = Work2.bounds;
  region2->RegionRectangle = Work2.RegionRectangle;

  /* must not free Work2 as it is the result that is in region2 now */
  
  /* everything went alright */
  return TRUE; 
  
  AROS_LIBFUNC_EXIT
}
