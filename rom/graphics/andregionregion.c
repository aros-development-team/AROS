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
    
  struct Region Backup;
  struct Region Work;
  struct Rectangle R;
  struct RegionRectangle * RR = region1->RegionRectangle;
  struct RegionRectangle * _RR;
  struct RegionRectangle * __RR;
  
  /* Region2 will hold the result */
  Backup.bounds          = region2->bounds;
  Backup.RegionRectangle = region2->RegionRectangle;
  
  region1->bounds.MinX = 0;
  region1->bounds.MinY = 0;
  region1->bounds.MaxX = 0;
  region1->bounds.MaxY = 0;
  
  while (NULL != RR)
  {
    /* 
       Backup holds the inital Region 2.
       I have to make another backup of the whole Region first
     */
    Work.bounds = Backup.bounds;
    _RR = Backup.RegionRectangle;
    
    if (NULL != _RR)
    {
      __RR = (struct RegionRectangle *)
                   AllocMem(sizeof(struct RegionRectangle), 0);
      if (NULL == __RR)
      {
        /* no more memory. I have to restore the inital state */
        ClearRegion(&Work);
        ClearRegion(region2);
        region2->bounds          = Backup.bounds;
        region2->RegionRectangle = Backup.RegionRectangle;
        
        return FALSE;
      }
      
      Work.RegionRectangle = __RR;
      __RR->bounds = _RR->bounds;
      __RR->Prev   = NULL;
      
      _RR          = _RR->Next;
                                  
      while (NULL != _RR)
      {
        __RR -> Next = (struct RegionRectangle *)
                         AllocMem(sizeof(struct RegionRectangle), 0);

        if (NULL == __RR->Next)
        {
          /* no more memory. I have to restore the inital state */
          ClearRegion(&Work);
          ClearRegion(region2);
          region2->bounds          = Backup.bounds;
          region2->RegionRectangle = Backup.RegionRectangle;
        
          return FALSE;
        }

        __RR -> Next -> Prev = __RR;
        __RR                 = __RR ->Next;

        __RR->bounds         = _RR->bounds;        
       
        _RR                  = _RR->Next;
      }
      __RR->Next = NULL;  
    }
    else
      Work.RegionRectangle = NULL;
    
    R.MinX = region1->bounds.MinX + RR->bounds.MinX;
    R.MinY = region1->bounds.MinY + RR->bounds.MinY;
    R.MaxX = region1->bounds.MinX + RR->bounds.MaxX;
    R.MaxY = region1->bounds.MinY + RR->bounds.MaxY;
    
    AndRectRegion(&Work, &R);/* can't fail */
    
    /* the result is in Work. I add this temporary result to the
       final result  */
       
    if (FALSE == OrRegionRegion(&Work, region2))
    {
      /* ran out of memory */
      ClearRegion(&Work);
      ClearRegion(region2);
      region2->bounds          = Backup.bounds;
      region2->RegionRectangle = Backup.RegionRectangle;
        
      return FALSE;
    }
       
    RR = RR->Next;
  } /* while (NULL != RR) */   

  return TRUE;
  
  AROS_LIBFUNC_EXIT
}
