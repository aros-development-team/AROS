/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/libcall.h>
#include <utility/hooks.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "layers_intern.h"
#include "basicfuncs.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))

struct CollectPixelsMsg
{
        struct Rectangle * rect;
};

/*
 * Call the hook for the parts of region _r that the ClipRects of layer l
 * hold. A simple refresh layer has its pixels (as far as it has any) in
 * the screen's bitmap; a smart refresh layer has the hidden ones in the
 * ClipRects' bitmaps.
 */
static void _CollectLayerPixels(struct Layer *l, struct Region *_r, struct Hook *callback,
                                       BOOL simple, struct LayersBase *LayersBase)
{
        struct RegionRectangle * _rr = _r->RegionRectangle;

        while (NULL != _rr) {
                struct Rectangle _rect;
                struct ClipRect * cr;

                _rect = _rr->bounds;
                _rect.MinX += _r->bounds.MinX;
                _rect.MinY += _r->bounds.MinY;
                _rect.MaxX += _r->bounds.MinX;
                _rect.MaxY += _r->bounds.MinY;

                cr = l->ClipRect;
                while (NULL != cr) {
                        struct Rectangle intersect;
                        /*
                         * Check for overlap with _rect
                         * Call callback with overlapping area!
                         */
                        if (AndRectRect(&_rect,&cr->bounds,&intersect)) {
                                struct CollectPixelsLayerMsg cplm;

                                /* Take data from sceen's bitmap... */
                                cplm.xSrc  = intersect.MinX;
                                cplm.ySrc  = intersect.MinY;
                                cplm.width = intersect.MaxX - intersect.MinX + 1;
                                cplm.height= intersect.MaxY - intersect.MinY + 1;
                                cplm.xDest = intersect.MinX;
                                cplm.yDest = intersect.MinY;
                                cplm.bm    = l->rp->BitMap;
                                if (!simple && NULL != cr->lobs) {
                                        /* ...or, hidden, from the ClipRect's bitmap */
                                        cplm.xSrc  = intersect.MinX - cr->bounds.MinX + CR_BMOFFX(cr);
                                        cplm.ySrc  = intersect.MinY - cr->bounds.MinY + CR_BMOFFY(cr);
                                        cplm.bm    = cr->BitMap;
                                }
                                cplm.layer   = l;
                                cplm.minterm = simple ? 0x0 : 0x0c0;
                                CallHookPkt(callback,l,&cplm);
                        }
                        cr = cr->Next;
                }
                _rr = _rr->Next;
        }
}

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

        /* The walk below ends with l NULL once it has passed the root layer */
        struct Layer_Info *li = l->LayerInfo;

        LockLayers(li);
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
                                struct Region * _r;
                                D(bug("l->shape=%p\n",l->shape));
                                _r = AndRegionRegionND(r,l->shape);
                                /*
                                 * Try to find the relevant parts in the
                                 * layer l and on those parts of the
                                 * bitmap that are relevant to call the callback hook
                                 */
                                if (IS_SIMPLEREFRESH(l)) {
                                        _CollectLayerPixels(l, _r, callback, TRUE, LayersBase);
                                } else
                                if (IS_SMARTREFRESH(l)) {
                                        _CollectLayerPixels(l, _r, callback, FALSE, LayersBase);
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
/* FIXME: Potential deadlock! */
                                if (l->parent) {
                                        LockLayer(0,l->parent);
                                }
                                UnlockLayer(l);
                                l = l->parent;
                        }
                }
/* FIXME: Potential deadlock! */
                if (l) {
                        if (l->back) {
                                LockLayer(0,l->back);
                        }
                        UnlockLayer(l);
                        l = l->back;
                }
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
                        cplm.ySrc  = _rect.MinY;
                        cplm.width = _rect.MaxX - _rect.MinX + 1;
                        cplm.height= _rect.MaxY - _rect.MinY + 1;
                        cplm.xDest = _rect.MinX;
                        cplm.yDest = _rect.MinY;
                        cplm.bm    = NULL;
                        cplm.layer = NULL;
                        D(bug("Calling callback now!\n"));
                        CallHookPkt(callback,NULL,&cplm);
                }
                
        } else {
                D(bug("Complete region handled! - Nothing to take from screen!\n"));
        }
        UnlockLayers(li);

        AROS_LIBFUNC_EXIT
}

