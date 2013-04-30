/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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

	AROS_LH2(void, EndUpdate,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l    , A0),
	AROS_LHA(UWORD         , flag , D0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 14, Layers)

/*  FUNCTION
        After the damaged areas are updated, this routine should be
        called so the regular cliprects of the layer can be installed.

    INPUTS
        l    -  pointer to layer
        flag -  TRUE if the update was complete. The damage list is disposed.
                FALSE it the update was partial. The damage list is kept. 

    RESULT

    NOTES

    EXAMPLE

    BUGS
      not tested

    SEE ALSO
         BeginUpdate()

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  /*
  ** must reset the flag NOW !! Keep this order!!
  */
  l->Flags &= ~LAYERUPDATING;

  if (NULL != l->ClipRect)
  {
    if (IS_SMARTREFRESH(l))
      _CopyClipRectsToClipRects(l, 
                                l->ClipRect, 
                                l->cr2,
                                0,
				0,
                                FALSE,
                                TRUE,
				FALSE,
				LayersBase);
    else
      _FreeClipRectListBM(l,l->ClipRect, LayersBase);
  }
      
  l->ClipRect = l->cr2;
  l->cr2 = NULL;

  if (FALSE != flag)
  {
    /* the update was complete so I free the damage list */
    ClearRegion(l->DamageList);
    l->Flags &= ~LAYERREFRESH;
  }
  
  UnlockLayer(l);

  AROS_LIBFUNC_EXIT
} /* EndUpdate */
