/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/layers.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH3(struct Layer *, WhichLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(LONG               , x,  D0),
	AROS_LHA(LONG               , y,  D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 22, Layers)

/*  FUNCTION
        Determines in which layer the point (x,y) is to be found.
        Starts with the frontmost layer. 

    INPUTS
        li - pointer to Layers_Info structure
        x  - x-coordinate
        y  - y-coordinate

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

    struct Layer *l;

    D(bug("WhichLayer(li @ $%lx, x %ld, y %ld)\n", li, x, y));

    /* No need for LockLayers() here, as other layers cannot be moved around/resized/
       depth arranged when the layerinfo is locked. Rendering can still happen in
       other layers (and by other tasks) but this is not a problem as this does not
       affect visibility of layers. */
       
    LockLayerInfo(li);
    
    for(l = li->top_layer; l != NULL; l = l->back)
    {
        if (IS_INVISIBLE(l)) continue;
#warning maybe have another flag to immediately see if a layer is completely hidden

        if((x >= l->bounds.MinX) &&
	   (y >= l->bounds.MinY) &&
	   (x <= l->bounds.MaxX) &&
	   (y <= l->bounds.MaxY))
	{
	    if (IL(l)->shape)
	    {
	        /* Layer Shape Region is in layer coords, not in screen coords! */
		
		if (PointInRegion(IL(l)->shape, x - l->bounds.MinX, y - l->bounds.MinY)) break;       
	    }
	    else break;
	}
    }
    
    UnlockLayerInfo(li);
    
    return l;

    AROS_LIBFUNC_EXIT
    
} /* WhichLayer */
