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

#include "../graphics/intregions.h"
#include "basicfuncs.h"

#define DEBUG 1
#include <aros/debug.h>

#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))

struct CollectPixelsMsg
{
	struct Rectangle * rect;
};

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH3(void, CollectPixelsLayer,

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
        AROS_LIBFUNC_INIT

	LockLayers(l->LayerInfo);
	LockLayer(0,l);
	D(bug("%s: layer=%p,region=%p\n",
	      __FUNCTION__,
	      l,
	      r));
	while (NULL != l && !IS_ROOTLAYER(l) && FALSE == IS_EMPTYREGION(r)) {
		if (IS_VISIBLE(l)) {
			/*
			 * For every layer check whether its parent is
			 * overlapping with the area of r at all. If it
			 * does not, can immediately jump to the layer
			 * behind the parent.
			 */
			if (DO_OVERLAP(&l->parent->shape->bounds,&r->bounds)) {
				/*
 				 * Find out what both layers have in common.
				 */
				D(bug("l->shape=%p\n",l->shape));
				struct Region * _r = AndRegionRegionND(r,l->shape);
				/*
				 * Try to find the relevant parts in the
				 * layer l and on those parts of the
				 * bitmap that are relevant to call the callback hook
				 */
				if (IS_SIMPLEREFRESH(l)) {
					D(bug("SIMPLEFRESH layer found! %d/%d - %d/%d\n",
					      l->bounds.MinX,
					      l->bounds.MinY,
					      l->bounds.MaxX,
					      l->bounds.MaxY));
					struct RegionRectangle * _rr = _r->RegionRectangle;
					while (NULL != _rr) {
						struct Rectangle _rect = _rr->bounds;
						_rect.MinX += _r->bounds.MinX;
						_rect.MinY += _r->bounds.MinY;
						_rect.MaxX += _r->bounds.MinX;
						_rect.MaxY += _r->bounds.MinY;

						struct ClipRect * cr = l->ClipRect;
						while (NULL != cr) {
							struct Rectangle intersect;
							/*
							 * Check for overlap with _rect
							 * Call callback with overlapping area!
							 */
							if (_AndRectRect(&_rect,&cr->bounds,&intersect)) {
								struct CollectPixelsLayerMsg cplm;
								cplm.xSrc    = intersect.MinX;
								cplm.ySrc    = intersect.MinY;
								cplm.width   = intersect.MaxX - intersect.MinX + 1;
								cplm.height  = intersect.MaxY - intersect.MinY + 1;
								cplm.xDest   = intersect.MinX;
								cplm.yDest   = intersect.MinY;
								cplm.bm      = l->rp->BitMap;
								cplm.layer   = l;
								cplm.minterm = 0x0;
								D(bug("SimpleRefresh: Calling callback now! bm=%p\n",cplm.bm));
								CallHookPkt(callback,l,&cplm);
								D(bug("Returned from callback!\n"));
							}

							cr  = cr->Next;
						}

						_rr = _rr->Next;
					}
				} else 
				if (IS_SMARTREFRESH(l)) {
					D(bug("SMARTREFRESH layer found!\n"));
					struct RegionRectangle * _rr = _r->RegionRectangle;
					while (NULL != _rr) {
						struct Rectangle _rect = _rr->bounds;
						_rect.MinX += _r->bounds.MinX;
						_rect.MinY += _r->bounds.MinY;
						_rect.MaxX += _r->bounds.MinX;
						_rect.MaxY += _r->bounds.MinY;
						//Following does not work for some reason!
						//_TranslateRect(&_rect,_r->bounds.MinX,_r->bounds.MinY);
						/*
						 * Compare this rr against all hidden cliprects...
						 */
						struct ClipRect * cr = l->ClipRect;
						while (NULL != cr) {
							struct Rectangle intersect;
							/*
							 * Check for overlap with _rect
							 * Call callback with overlapping area!
							 */
							if (_AndRectRect(&_rect,&cr->bounds,&intersect)) {
								struct CollectPixelsLayerMsg cplm;
								D(bug("Overlapping: %d/%d-%d/%d\n",
								      intersect.MinX,
								      intersect.MinY,
								      intersect.MaxX,
								      intersect.MaxY));
								D(bug("CR: %d/%d-%d/%d\n",
								      cr->bounds.MinX,
								      cr->bounds.MinY,
								      cr->bounds.MaxX,
								      cr->bounds.MaxY));
								D(bug("Rect: %d/%d-%d/%d\n",
								      _rect.MinX,
								      _rect.MinY,
								      _rect.MaxX,
								      _rect.MaxY));
								D(bug("Visible: %s\n",
								      (NULL == cr->lobs) ? "TRUE"
								                         : "FALSE" ));

								if (NULL == cr->lobs) {
									/*
									 * Take data from sceen's bitmap
									 */
									cplm.xSrc  = intersect.MinX;
									cplm.ySrc  = intersect.MinY;
									cplm.width = intersect.MaxX - intersect.MinX + 1;
									cplm.height= intersect.MaxY - intersect.MinY + 1;
									cplm.xDest = intersect.MinX;
									cplm.yDest = intersect.MinY;
									cplm.bm    = l->rp->BitMap;
								} else {
									cplm.xSrc  = intersect.MinX - cr->bounds.MinX + ALIGN_OFFSET(intersect.MinX);
									cplm.ySrc  = intersect.MinY - cr->bounds.MinY;
									cplm.width = intersect.MaxX - intersect.MinX + 1;
									cplm.height= intersect.MaxY - intersect.MinY + 1;
									cplm.xDest = intersect.MinX;
									cplm.yDest = intersect.MinY;
									cplm.bm    = cr->BitMap;
								}
								cplm.layer   = l;
								cplm.minterm = 0x0c0;
								D(bug("SmartRefresh: Calling callback now! bm=%p\n",cplm.bm));
								CallHookPkt(callback,l,&cplm);

							}
							cr = cr->Next;
						}
						_rr = _rr->Next;
					}
				} else 
				if (IS_SUPERREFRESH(l)) {
				}
				/*
				 * Region _r was treated. No need to look at it somewhere else.
				 * Could call this function again, but better in a loop...
				 */
				ClearRegionRegion(_r,r);
				DisposeRegion(_r);
				if (IS_EMPTYREGION(r)) {
					D(bug("Got empty region now!\n"));
				}
			} else {
				/*
				 * Jump to the parent layer.
				 */
#warning Potential deadlock!
				if (l->parent) {
					LockLayer(0,l->parent);
				}
				UnlockLayer(l);
				l = l->parent;
			}
		}
#warning Potential deadlock!
		if (l->back) {
			LockLayer(0,l->back);
		}
		UnlockLayer(l);
		l = l->back;
	}
	if (l)
		UnlockLayer(l);
	
	if (!IS_EMPTYREGION(r)) {
		struct RegionRectangle * _rr = r->RegionRectangle;
		while (NULL != _rr) {
			struct CollectPixelsLayerMsg cplm;
			struct Rectangle _rect = _rr->bounds;
			_rect.MinX += r->bounds.MinX;
			_rect.MinY += r->bounds.MinY;
			_rect.MaxX += r->bounds.MinX;
			_rect.MaxY += r->bounds.MinY;
			D(bug("Rect: %d/%d-%d/%d\n",
			      _rect.MinX,
			      _rect.MinY,
			      _rect.MaxX,
			      _rect.MaxY));
			D(bug("Directly from screen background!\n"));
			_rr = _rr->Next;
			
			cplm.xSrc  = _rect.MinX;
			cplm.xSrc  = _rect.MinY;
			cplm.width = _rect.MaxX - _rect.MinX + 1;
			cplm.height= _rect.MaxY - _rect.MinY + 1;
			cplm.xDest = _rect.MinX;
			cplm.yDest = _rect.MaxY;
			cplm.bm    = NULL;
			cplm.layer = NULL;
			D(bug("Calling callback now!\n"));
			CallHookPkt(callback,NULL,&cplm);
		}
		
	} else {
		D(bug("Complete region handled! - Nothing to take from screen!\n"));
	}
	UnlockLayers(l->LayerInfo);

        AROS_LIBFUNC_EXIT
}

