/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: (AROS only) Graphics function SetRegion()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/regions.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH2(BOOL, SetRegion,

/*  SYNOPSIS */
        AROS_LHA(struct Region *, src , A0),
        AROS_LHA(struct Region *, dest, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 195, Graphics)

/*  FUNCTION
        Sets the destination region to the source region.
        Allocates necessary RegionRectangles if necessary
        and deallocates any excessive RegionRectangles in
        the destination Region. The source Region remains
        untouched.
        If the system runs out of memory during allocation
        of RegionRectangles the destination Region will
        .
	
    INPUTS

    RESULT
        TRUE if everything went alright, FALSE otherwise
        (out of memory).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewRegion() DisposeRegion() DisposeRegionRectangle()
	CopyRegion()

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct RegionRectangle * rrs =  src->RegionRectangle;
  struct RegionRectangle * rrd = dest->RegionRectangle;
  struct RegionRectangle * rrd_prev = NULL;
  
  dest->bounds = src->bounds;

  while (NULL != rrs)
  {
    /*
     * Is there a destination region rectangle available?
     */
    if (NULL == rrd)
    {
      rrd = NewRegionRectangle();
    
      if (NULL == rrd)
        return FALSE;
        
      if (NULL == rrd_prev)
        dest->RegionRectangle = rrd;
      else
        rrd_prev->Next = rrd;
      
      rrd->Next = NULL;
    }
    
    /*
     * Copy the bounds.
     */
    rrd->bounds = rrs->bounds;
    rrd->Prev   = rrd_prev;

    /*
     * On to the next one in both lists.
     */
    rrs = rrs->Next;
    rrd_prev = rrd;
    rrd = rrd->Next;
  }
  
  /*
   * Deallocate any excessive RegionRectangles that might be in
   * the destination Region.
   */
  if (NULL == rrd_prev)
  {
    /*
     * Did never enter above loop...
     */
    rrd = dest->RegionRectangle;
    dest->RegionRectangle = NULL;
  }
  else
  {
    /*
     * Was in the loop.
     */
    rrd_prev->Next = NULL;
  }
  
  while (NULL != rrd)
  {
    struct RegionRectangle * _rr = rrd->Next;
    DisposeRegionRectangle(rrd);
    rrd = _rr;
  }
  
  return TRUE;
    
  AROS_LIBFUNC_EXIT
    
} /* SetRegion */
