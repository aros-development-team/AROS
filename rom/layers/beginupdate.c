/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/clip.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <graphics/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"

/*****************************************************************************

    NAME */

	AROS_LH1(LONG, BeginUpdate,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l, A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 13, Layers)

/*  FUNCTION
        Converts the damage list to ClipRects and exchanges the
        two lists for faster redrawing. This routine allows a
        faster update of the display as it will only be rendered
        in the damaged areas.
        This routine will automatically lock the layer agains changes.   

    INPUTS
        l - pointer to layer

    RESULT
        TRUE  if damage list conversion was successful
        FALSE if list could not be converted.

    NOTES

    EXAMPLE

    BUGS
      not tested

    SEE ALSO
      EndUpdate()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  /* Convert the list of regionrectangles to a cliprect list */
  struct Region * R = l -> DamageList;
  struct ClipRect * CR;
  struct ClipRect * FirstCR = NULL;
  struct RegionRectangle * RR;

  if (NULL != R)
  {
    RR = R->RegionRectangle;
    /* process all region rectangles */
    while (NULL != RR)
    {
      CR = _AllocClipRect(l);
      /* was allocation successful? */
      if (NULL != CR)
      {
        /* init. ClipRect */
        CR->Next = FirstCR;
        FirstCR  = CR;
        CR->bounds.MinX = R->bounds.MinX + RR->bounds.MinX;
        CR->bounds.MaxX = R->bounds.MinX + RR->bounds.MaxX;
        CR->bounds.MinY = R->bounds.MinY + RR->bounds.MinY;
        CR->bounds.MaxY = R->bounds.MinY + RR->bounds.MaxY;
        /* anything else? */
      }
      else
      {
        /* free the whole list of allocated ClipRects and return FALSE */
        CR = FirstCR;
        while (NULL != CR)
	{
          FirstCR = CR->Next;
          _FreeClipRect(CR, l);
          CR = FirstCR;
	}
        return FALSE;
      } /* else */

      /* go to next regionrectangle and build next cliprect */
      RR = RR->Next;
    } /* while (NULL != RR) */
  }

  /*
    Now exchange the two ClipRect lists. The converted damage list
    will be placed in the position of the regular ClipRect list
    and the regular one will be kept in l->_cliprects until
    EndUpdate() is called.
    For the freeing of the damage list I will wait until 
    EndUpdate(.., TRUE) is called and then free it. 
   */

  LockLayer(0, l);
  
  l->_cliprects = l->ClipRect;
  l->ClipRect   = FirstCR;
  return TRUE;

  AROS_LIBFUNC_EXIT
} /* BeginUpdate */
