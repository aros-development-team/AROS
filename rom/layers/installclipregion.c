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

  struct Region * OldRegion = l->ClipRegion;
  struct ClipRect * FirstCR; 


  /* is there a clipregion currently installed? */
  if (NULL != OldRegion)
  { 
    /* free all the ClipRects that make up this layer and
       were created due to it being a clipregioned layer 
     */
    _FreeClipRectListBM(l,l->ClipRect);
    
    /* only reinstall the regular cliprects if there is no
       new region given
    */
    if (NULL == region)
    {
      l->ClipRect = l->_cliprects;
      l->_cliprects = NULL;
      l->ClipRegion = NULL;
      return OldRegion;
    }
  }

  /* if there's no new region to install then there's nothing else to do */
  if (NULL == region)
    return;

  /* First I cut down the region to the rectangle of the layer */
  l -> ClipRegion = region;
    
  /* convert the region to a list of ClipRects */
  /* backup the old cliprects */
  l->_cliprects = l->ClipRect;

  l->ClipRect = CopyClipRectsInRegion(l, l->_cliprects, region);  

  /* right now I am assuming that everything went alright */

  return OldRegion;

  AROS_LIBFUNC_EXIT
} /* InstallClipRegion */
