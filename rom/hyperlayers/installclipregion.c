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
#include <proto/layers.h>
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

  struct Region * OldRegion;
  
  LockLayer(0, l);
    
  OldRegion = _InternalInstallClipRegion(l, region, 0, 0, LayersBase);
  
  UnlockLayer(l);

  return OldRegion;

  AROS_LIBFUNC_EXIT
} /* InstallClipRegion */
