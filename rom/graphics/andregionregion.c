/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function AndRegionRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(BOOL, AndRegionRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Region *, region1, A0),
	AROS_LHA(struct Region *, region2, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 104, Graphics)

/*  FUNCTION
	AND of one region with another region, leaving result in 
	second region.

    INPUTS
	region1 - pointer to a region structure
	region2 - pointer to a region structure

    RESULT
	TRUE if the operation was succesful, else FALSE
	(out of memory)

    NOTES
	
    EXAMPLE

    BUGS

    SEE ALSO
	XorRegionRegion(), OrRegionRegion()

    INTERNALS
        Two regions A and B consist of rectangles a1,...,a3 and b1,...,b3.
        A = a1 + a2 + a3;
        B = b1 + b2 + b3;
        A * B = (a1 + a2 + a3) * (b1 + b2 + b3) =
                 a1            * (b1 + b2 + b3)   +
                      a2       * (b1 + b2 + b3)   +
                           a3  * (b1 + b2 + b3);  

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
#if USE_BANDED_FUNCTIONS

    return _AndRegionRegion(region1, region2, GfxBase);

#else

  struct Region Backup;
  struct Region Work;
  struct RegionRectangle * RR = region1->RegionRectangle;
  /* Work will hold the result */
  Work.bounds.MinX = Work.bounds.MinY = 0;
  Work.bounds.MaxX = Work.bounds.MaxY = 0;
  Work.RegionRectangle = NULL;
  
  Backup.RegionRectangle = NULL;
  
  if (NULL == region2->RegionRectangle)
    return TRUE;
  
  while (NULL != RR)
  {
    struct Rectangle CurRectangle;
    /* make a copy of region2 */
    ClearRegion(&Backup);
    if (!CopyRegionRectangleList(region2->RegionRectangle, &Backup.RegionRectangle))
    {
      ClearRegion(&Backup);
      ClearRegion(&Work);
      return FALSE;
    } 
    Backup.bounds = region2->bounds;
    
    CurRectangle.MinX = region1->bounds.MinX + RR->bounds.MinX;
    CurRectangle.MaxX = region1->bounds.MinX + RR->bounds.MaxX;
    CurRectangle.MinY = region1->bounds.MinY + RR->bounds.MinY;
    CurRectangle.MaxY = region1->bounds.MinY + RR->bounds.MaxY;

    /* AND the Rectangle with the copy of region2 */
    AndRectRegion(&Backup, &CurRectangle); /* cannot fail! */
    
    /* add the result in Backup to the final result */

    if (FALSE == OrRegionRegion(&Backup, &Work))
    {
      ClearRegion(&Backup);
      ClearRegion(&Work);
      return FALSE;
    }     
    /* treat the next RegionRectangle */
    RR=RR->Next; 
  }

  /* everything went alright, so I can change region2 now */
  ClearRegion(region2);
  ClearRegion(&Backup);
  region2->bounds          = Work.bounds;
  region2->RegionRectangle = Work.RegionRectangle;
  
  /* mustn't clear Work as the result is in region2 now !! */

  return TRUE;
#endif
  AROS_LIBFUNC_EXIT
}
