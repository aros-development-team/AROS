/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/types.h>
#include <layers_intern.h>
#include <aros/libcall.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <utility/hooks.h>
#include "basicfuncs.h"

struct CollectPixelsMsg
{
	struct Rectangle * rect;
};

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH3(BOOL, CollectPixelsLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer  *, l        , A0),
	AROS_LHA(struct Region *, r        , A1),
	AROS_LHA(struct Hook   *, callback , A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 45, Layers)

/*  FUNCTION
        This function collects all the pixel within region r
        and calls the provided callback hook for all areas
        that were found. You can do with the pixels whatever
        you want...

    INPUTS
       l               - pointer to layer where to start out
       r               - region where to look for hidden or
                         visible pixels
       callback        - the callback will be invoked for the
                         found pixels along with information
                         about the size of the area that may
                         be copied.
       

    RESULT
  
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
	LockLayers(l->LayerInfo);
	while (NULL != l && FALSE == IS_EMPTYREGION(r)) {
		if (IS_VISIBLE(l)) {
			/*
 			 * Find out what both layers have in common.
			 */
			struct Region * _r = AndRegionRegionND(r,l->ClipRegion);
			/*
			 * Try to find the relevant parts in the
			 * layer l and on those parts of the
			 * bitmap that are relevant to call the callback hook
			 */
			// TODO
			if (NULL != callback) {
				struct CollectPixelsMsg cpm;
//				cpm.rect = r;
				CallHookPkt(callback,l, &cpm);
			}
			/*
			 * Region _r was treated. No need to look at it somewhere else.
			 * Could call this function again, but better in a loop...
			 */
			ClearRegionRegion(r,_r);
			DisposeRegion(_r);
		}
		l = l->back;
	}
	UnlockLayers(l->LayerInfo);
}

#if 0
DefaultCollectPixelsCallback(...)
{
	LONG xDest, yDest;
	BltBitMap(srcBm,
	          rect->MinX,
	          rect->MinY,
                  destBm,
	          xDest,
                  yDest,
	          rect->MaxX - rect->MinX + 1,
	          rect->MaxY - rect->MinY + 1,
	          0x0c0,
	          ~0,
	          NULL);
}
#endif
