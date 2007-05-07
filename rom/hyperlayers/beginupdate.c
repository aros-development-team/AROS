/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/clip.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <graphics/layers.h>
#include <proto/graphics.h>
#include <proto/layers.h>
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
        This routine will automatically lock the layer to prevent 
        changes.   

    INPUTS
        l - pointer to layer

    RESULT
        TRUE  if damage list conversion was successful
        FALSE if list could not be converted.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
      EndUpdate()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  struct Region *damage_region, *visible_damage_region;
  /* 
  ** Convert the list of regionrectangles in the damage list 
  ** to a cliprect list. 
  */

  LockLayer(0, l);

  l->cr2 = l->ClipRect;


  if (l->ClipRegion)
      damage_region = AndRegionRegionND(l->ClipRegion, l->DamageList);
  else
      damage_region = CopyRegion(l->DamageList);

  /* The DamageList is in layer coords. So convert it to screen coords */

  _TranslateRect(&damage_region->bounds, l->bounds.MinX, l->bounds.MinY);

  AndRectRegion(damage_region, &l->bounds);

  visible_damage_region = AndRegionRegionND(l->VisibleRegion, damage_region);
  AndRegionRegion(l->visibleshape, visible_damage_region);
  
  if (l->shaperegion)
  {
    _TranslateRect(&visible_damage_region->bounds, -l->bounds.MinX, -l->bounds.MinY);
    AndRegionRegion(l->shaperegion, visible_damage_region);
    _TranslateRect(&visible_damage_region->bounds, l->bounds.MinX, l->bounds.MinY);
  }

  l->ClipRect = _CreateClipRectsFromRegion(visible_damage_region,
                                           l,
                                           FALSE,
                                           damage_region,
					   LayersBase);

  if (IS_SMARTREFRESH(l))
  {
      _CopyClipRectsToClipRects(l,
                                l->cr2,
                                l->ClipRect,
                                0,
				0,
                                FALSE,
                                FALSE,
				FALSE,
				LayersBase);

  }

  DisposeRegion(damage_region);
  DisposeRegion(visible_damage_region);

  /*
  ** Must not set flag before InstallClipRegion!!! Keep this order!!!
  */
  l-> Flags |= LAYERUPDATING;

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* BeginUpdate */
