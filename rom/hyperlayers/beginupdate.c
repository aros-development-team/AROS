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

  struct Region * r = NewRegion();
  /* 
  ** Convert the list of regionrectangles in the damage list 
  ** to a cliprect list. 
  */

  LockLayer(0, l);

  l->cr2 = l->ClipRect;

  l->DamageList->bounds.MinX += l->bounds.MinX;
  l->DamageList->bounds.MinY += l->bounds.MinY;
  l->DamageList->bounds.MaxX += l->bounds.MinX;
  l->DamageList->bounds.MaxY += l->bounds.MinY;

//#warning If the damagelist was correct (which it currently is not) then this following statement would not be necessary!
  OrRegionRegion(l->DamageList, r);
  AndRegionRegion(l->VisibleRegion, r);

  l->ClipRect = _CreateClipRectsFromRegion(r,
                                           l,
                                           FALSE,
                                           l->DamageList);

  DisposeRegion(r);

  l->DamageList->bounds.MinX -= l->bounds.MinX;
  l->DamageList->bounds.MinY -= l->bounds.MinY;
  l->DamageList->bounds.MaxX -= l->bounds.MinX;
  l->DamageList->bounds.MaxY -= l->bounds.MinY;

  /*
  ** Must not set flag before InstallClipRegion!!! Keep this order!!!
  */
  l-> Flags |= LAYERUPDATING;

  return TRUE;

  AROS_LIBFUNC_EXIT
} /* BeginUpdate */
