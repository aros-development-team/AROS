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
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct Region *damage_region, *visible_damage_region;
  /* 
  ** Convert the list of regionrectangles in the damage list 
  ** to a cliprect list. 
  */

  LockLayer(0, l);

  l->cr2 = l->ClipRect;

  /* The DamageList is in layer coords. So convert it to screen coords */
  
  _TranslateRect(&l->DamageList->bounds, l->bounds.MinX, l->bounds.MinY);

  damage_region = CopyRegion(l->DamageList);
  AndRectRegion(damage_region, &l->bounds);

  if (l->ClipRegion)
  {
     /* The ClipRegion is in layer coords. Instead of translating the
        clip region, we do the inverse translation with the damage_region,
	because of paranoia, that the clip region installed by an app might
	at the same time be used for other things by the app, for example
	as a clip region on a layer in another screen */
	
     _TranslateRect(&damage_region->bounds, -l->bounds.MinX, -l->bounds.MinY);
     AndRegionRegion(l->ClipRegion, damage_region);
     _TranslateRect(&damage_region->bounds, l->bounds.MinX, l->bounds.MinY);
  }

  visible_damage_region = CopyRegion(damage_region);    
  AndRegionRegion(l->VisibleRegion, visible_damage_region);
  
  
  l->ClipRect = _CreateClipRectsFromRegion(visible_damage_region,
                                           l,
                                           FALSE,
                                           damage_region);

  DisposeRegion(damage_region);
  DisposeRegion(visible_damage_region);
  
  /* Convert DamageList back to layer coords */
  
  _TranslateRect(&l->DamageList->bounds, -l->bounds.MinX, -l->bounds.MinY);

  /*
  ** Must not set flag before InstallClipRegion!!! Keep this order!!!
  */
  l-> Flags |= LAYERUPDATING;

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* BeginUpdate */
