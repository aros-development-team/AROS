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

	AROS_LH2(struct Region *, InstallClipRegion,

/*  SYNOPSIS */
	AROS_LHA(struct Layer  *, l     , A0),
	AROS_LHA(struct Region *, region, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 29, Layers)

/*  FUNCTION
       Install a transparent Clip region in the layer. All subsequent
       graphics call to the rastport of the layer will be clipped to
       that region. 
       None of the system functions will free the ClipRegion for you,
       so you will have to call InstallClipRegion(l, NULL) before
       closing a window or deleting a layer.

    INPUTS
       l      - pointer to layer
       region - pointer to region to be clipped against.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct Region * OldRegion;
  BOOL updating = FALSE;
  
  LockLayer(0, l);
    
  OldRegion = l->ClipRegion;

  if ((OldRegion != NULL) || (region != NULL))
  {
    if (l->Flags & LAYERUPDATING)
    {
      /* InstallClipRegion does not work if the layer is in update state (BeginUpdate) */

      updating = TRUE;
      EndUpdate(l, FALSE);
      
      OldRegion = l->ClipRegion;
    }

    /* is there a clipregion currently installed? */
    if (NULL != OldRegion)
    { 
      /*
       *  Copy the contents of the region cliprects to the regular
       *  cliprects if layer is a SMARTLAYER. Also free the list of 
       *  region cliprects.
       */
      if (NULL != l->ClipRect)
      {
	if (LAYERSMART == (l->Flags & (LAYERSMART|LAYERSUPER)))
          CopyAndFreeClipRectsClipRects(l, l->ClipRect, l->_cliprects);
	else
          _FreeClipRectListBM(l, l->ClipRect);
      }

      /* restore the regular ClipRects */
      l->ClipRect = l->_cliprects;    

    }

    /* at this point the regular cliprects are in l->ClipRect in any case !*/

    /* if there's no new region to install then there's not much to do */
    l->ClipRegion = region;

    if (NULL == region)
      l->_cliprects = NULL;
    else
    {

      /* convert the region to a list of ClipRects */
      /* backup the old cliprects */
      l->_cliprects = l->ClipRect;

      l->ClipRect = CopyClipRectsInRegion(l, l->_cliprects, region);  

      /* right now I am assuming that everything went alright */
    }
  
    if (updating)
      BeginUpdate(l);

  } /* if ((OldRegion != NULL) || (region != NULL)) */
  
  UnlockLayer(l);
  
  return OldRegion;

  AROS_LIBFUNC_EXIT
} /* InstallClipRegion */
