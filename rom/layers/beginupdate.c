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

  LockLayer(0, l);

  if (l->_cliprects)
    AndRegionRegion(l->ClipRect, l->DamageList);

  if (NULL != R)
  {

kprintf("BeginUpdate: NULL!=R \n");
    RR = R->RegionRectangle;
    /* process all region rectangles */
    while (NULL != RR)
    {
      CR = _AllocClipRect(l);

kprintf("CR: %p\n",CR);
      /* was allocation successful? */
      if (NULL != CR)
      {
        /* init. ClipRect */
        CR->Next = FirstCR;
        FirstCR  = CR;
        /* That's what we need in any case */
        CR->bounds = RR->bounds;
        
        CR->bounds.MinX += R->bounds.MinX + l->bounds.MinX;
        CR->bounds.MinY += R->bounds.MinY + l->bounds.MinY;
        CR->bounds.MaxX += R->bounds.MinX + l->bounds.MinX;
        CR->bounds.MaxY += R->bounds.MinY + l->bounds.MinY;
        
        /* anything else? */
      }
      else
      {
        /* free the whole list of allocated ClipRects and return FALSE */
        CR = FirstCR;
        while (NULL != CR)
	{
kprintf("Error 1!\n");

          FirstCR = CR->Next;
          _FreeClipRect(CR, l);
          CR = FirstCR;
	}
	
	/* stegerg: don't unlock here, because endupdate unlocks always */
	/* UnlockLayer(l); */
        return FALSE;
      } /* else */

      /* go to next regionrectangle and build next cliprect */
      RR = RR->Next;
    } /* while (NULL != RR) */
  }

  /*
    Now exchange the two ClipRect lists. The converted damage list
    will be placed in the position of the regular ClipRect list
    and the regular one will be kept in l->cr until
    EndUpdate() is called.
    For the freeing of the damage list I will wait until 
    EndUpdate(.., TRUE) is called and then free it. 
   */

  l->cr         = l->ClipRect;
  l->ClipRect   = FirstCR;
  
  if (NULL == FirstCR)
  {
    l->Flags &= ~LAYERREFRESH;
    return FALSE;
  }

  l-> Flags |= LAYERUPDATING;
  
  return TRUE;

  AROS_LIBFUNC_EXIT
} /* BeginUpdate */
