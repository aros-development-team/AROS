/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Basic support functions for layers.library.
*/

#include <aros/config.h>
#include <aros/asmcall.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <graphics/clip.h>
#include <graphics/regions.h>
#include <graphics/layers.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <utility/hooks.h>
#include <setjmp.h>

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/arossupport.h>

#include "layers_intern.h"
#include "basicfuncs.h"

#define CLIPRECTS_OUTSIDE_OF_SHAPE 1



/*
 *  Sections:
 *
 *  + Blitter
 *  + Hook
 *  + Layer
 *  + LayerInfo
 *  + Rectangle
 *  + Resource Handling
 *  + Miscellaneous
 *
 */

/***************************************************************************/
/*                                 BLITTER                                 */
/***************************************************************************/

#define CR2NR_NOBITMAP 0
#define CR2NR_BITMAP   1

#if !(AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
/*
 * These functions cause the infamous "fixed or forbidden register was spilled"
 * bug/feature in m68k gcc, so these were written straight in asm. They can be
 * found in config/m68k-native/layers, for the m68k AROSfA target. Other targets,
 * that use stack passing, can use these versions.
 */

void BltRPtoCR(struct RastPort *    rp,
               struct ClipRect *    cr,
               ULONG                Mode,
               struct LayersBase *  LayersBase)
{
    BltBitMap(rp->BitMap,
              cr->bounds.MinX,
              cr->bounds.MinY,
              cr->BitMap,
              CR_BMOFFX(cr), CR_BMOFFY(cr),
              cr->bounds.MaxX - cr->bounds.MinX + 1,
              cr->bounds.MaxY - cr->bounds.MinY + 1,
              Mode,
              ~0,
              NULL);

}

void BltCRtoRP(struct RastPort *   rp,
               struct ClipRect *   cr,
               ULONG               Mode,
               struct LayersBase  * LayersBase)
{
    BltBitMap(cr->BitMap,
              CR_BMOFFX(cr),
              CR_BMOFFY(cr),
              rp->BitMap,
              cr->bounds.MinX,
              cr->bounds.MinY,
              cr->bounds.MaxX - cr->bounds.MinX + 1,
              cr->bounds.MaxY - cr->bounds.MinY + 1,
              Mode,
              ~0,
              NULL);

}

#endif /* if !native */

/***************************************************************************/
/*                                  HOOK                                   */
/***************************************************************************/

struct layerhookmsg
{
    struct Layer *l;
/*  struct Rectangle rect; (replaced by the next line!) */
    WORD MinX, MinY, MaxX, MaxY;
    LONG OffsetX, OffsetY;
};

void _CallLayerHook(struct Hook * h,
                    struct RastPort * rp,
                    struct Layer * L,
                    struct Rectangle * R,
                    WORD offsetX,
                    WORD offsetY,
                    struct LayersBase * LayersBase)
{
  struct BitMap * bm = rp->BitMap;

  if (L)
  {
    if (IL(L)->intflags & INTFLAG_AVOID_BACKFILL) return;
  }
  
  if (h == LAYERS_BACKFILL)
  {
    /* Use default backfill, which means that I will clear the area */
    BltBitMap(bm,
              0,
              0,
              bm,
              R->MinX,
              R->MinY,
              R->MaxX - R->MinX + 1,
              R->MaxY - R->MinY + 1,
              0x000,
              0xff,
              NULL);
    /* that's it */
    return;
  }
  
  if (h != LAYERS_NOBACKFILL)
  {
    struct layerhookmsg msg;
    msg.l    = L;
    msg.MinX = R->MinX;
    msg.MinY = R->MinY;
    msg.MaxX = R->MaxX;
    msg.MaxY = R->MaxY;
    msg.OffsetX = offsetX;
    msg.OffsetY = offsetY;
    
    AROS_UFC3NR(void, h->h_Entry,
        AROS_UFCA(struct Hook *,         h   ,A0),
        AROS_UFCA(struct RastPort *,     rp  ,A2),
        AROS_UFCA(struct layerhookmsg *, &msg,A1)
    );
  }
  
}


/***************************************************************************/
/*                                 LAYER                                   */
/***************************************************************************/

/*
 * Free a layer and all its associated structures
 */
void _FreeLayer(struct Layer * l, struct LayersBase *LayersBase)
{
  struct ClipRect * cr = l->ClipRect, * _cr;
  
  while (cr)
  {
    if (cr->BitMap && !(cr->Flags & CR_SHAREDBITMAP))
      FreeBitMap(cr->BitMap);
    _cr = cr->Next;

    FreePooled(LayersBase->lb_ClipRectPool, cr, sizeof(struct ClipRect));

    cr = _cr;
  }

  /*
   * also free all backed up cliprects.
   */
  cr = l->SuperSaveClipRects;

  while (cr)
  {
    _cr = cr->Next;
    FreePooled(LayersBase->lb_ClipRectPool, cr, sizeof(struct ClipRect));

    cr = _cr;
  }

  if (IL(l)->surface)
  {
    _AccountSurface(l, 0, LayersBase);
    FreeBitMap(IL(l)->surface);
  }
  if (IL(l)->occludeshape && IL(l)->occludeshape != l->visibleshape)
    DisposeRegion(IL(l)->occludeshape);
  if (IL(l)->compshape)
    DisposeRegion(IL(l)->compshape);
  DisposeRegion(l->DamageList);
  DisposeRegion(l->VisibleRegion);
  DisposeRegion(l->shape);
  DisposeRegion(l->visibleshape);
  FreeMem(l, sizeof(struct IntLayer));
}



/***************************************************************************/
/*                               LAYERINFO                                 */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
/*
 * Allocate LayerInfo_extra and initialize its resource list. Layers uses
 * this resource list to keep track of various memory allocations it makes
 * for the layers. See ResourceNode and ResData in layers_intern.h for the
 * node structure. See AddLayersResource for more information on the basic
 * operation.
 */
BOOL _AllocExtLayerInfo(struct Layer_Info * li, struct LayersBase *LayersBase)
{
    if(++li->fatten_count != 0)
        return TRUE;

    if(!(li->LayerInfo_extra = AllocMem(sizeof(struct LayerInfo_extra),MEMF_PUBLIC|MEMF_CLEAR)))
        return FALSE;

    NewList((struct List *)&((struct LayerInfo_extra *)li->LayerInfo_extra)->lie_ResourceList);

    return TRUE;
}

/*
 * Free LayerInfo_extra.
 */
void _FreeExtLayerInfo(struct Layer_Info * li, struct LayersBase *LayersBase)
{
    if(--li->fatten_count >= 0)
        return;

    /* Kill Root Layer */
    
    if (li->check_lp)
        DeleteLayer(0UL, li->check_lp);
    li->check_lp = NULL;

    if(li->LayerInfo_extra == NULL)
        return;

    /* Should composited layers be left behind, the display must forget their surfaces */
    if (LIE(li)->lie_Surfaces && LIE(li)->lie_ScreenBM)
        LayerSurfacesChanged(LIE(li)->lie_ScreenBM, NULL);

    FreeMem(li->LayerInfo_extra, sizeof(struct LayerInfo_extra));

    li->LayerInfo_extra = NULL;
}

/*
 * Dynamically allocate LayerInfo_extra if it isn't already there.
 */
BOOL SafeAllocExtLI(struct Layer_Info * li,
                    struct LayersBase * LayersBase)
{
    LockLayerInfo(li);

    /* Check to see if we can ignore the rest of this call. :-) */
    if(li->Flags & NEWLAYERINFO_CALLED)
        return TRUE;

    if(_AllocExtLayerInfo(li, LayersBase))
        return TRUE;

    UnlockLayerInfo(li);

    return FALSE;
}

/*
 * Free LayerInfo_extra if it was dynamically allocated, and unlock the LI.
 */
void SafeFreeExtLI(struct Layer_Info * li,
                   struct LayersBase * LayersBase)
{
    if(!(li->Flags & NEWLAYERINFO_CALLED))
        _FreeExtLayerInfo(li, LayersBase);

    UnlockLayerInfo(li);
}

/***************************************************************************/
/*                                RECTANGLE                                */
/***************************************************************************/


#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
/*
void _TranslateRect(struct Rectangle *rect, WORD dx, WORD dy)
{
    rect->MinX += dx;
    rect->MinY += dy;
    rect->MaxX += dx;
    rect->MaxY += dy;
}*/


/***************************************************************************/
/*                            RESOURCE HANDLING                            */
/***************************************************************************/

/*
 * Allocate memory for a ClipRect.
 */

struct ClipRect * _AllocClipRect(struct Layer * L, struct LayersBase *LayersBase)
{
  struct ClipRect * CR;
  
  CR =  L->SuperSaveClipRects;

  if (NULL != CR)
  {
   /* I want to access the list of free ClipRects alone */
    L->SuperSaveClipRects = CR->Next;
    L->SuperSaveClipRectCounter--;

    CR->Flags  = 0;
    CR->Next   = NULL;
    CR->lobs   = NULL;
    CR->BitMap = NULL;
    CR->_p1    = NULL;
    CR->_p2    = NULL;
    return CR;
  }

  CR = AllocPooled(LayersBase->lb_ClipRectPool, sizeof(struct ClipRect));
  if (NULL != CR)
  {
    CR->Flags  = 0;
    CR->Next   = NULL;
    CR->lobs   = NULL;
    CR->BitMap = NULL;
    CR->_p1    = NULL;
    CR->_p2    = NULL;
  }
  return CR;
}

/*
 * Return memory of a ClipRect for later use.
 */

void _FreeClipRect(struct ClipRect   * CR,
                   struct Layer      * L,
                   struct LayersBase * LayersBase)
{
  if (L->SuperSaveClipRectCounter < MAXSUPERSAVECLIPRECTS)
  {
    /* Add the ClipRect to the front of the list */
    CR -> Next = L -> SuperSaveClipRects;
    L -> SuperSaveClipRects = CR;
    L -> SuperSaveClipRectCounter++;
  }
  else
  {
    FreePooled(LayersBase->lb_ClipRectPool, CR, sizeof(struct ClipRect));
  }
}

/*
 * Free a whole list of cliprects including the allocated bitmaps (if any)
 */

void _FreeClipRectListBM(struct Layer * L,
                         struct ClipRect * CR,
                         struct LayersBase *LayersBase)
{
  struct ClipRect * _CR = CR;
  BOOL isSmart;
  if ((L->Flags & (LAYERSUPER|LAYERSMART)) == LAYERSMART)
    isSmart = TRUE;
  else
    isSmart = FALSE;
 
  /*
   * This function is not watching for the upper limit of
   * pre allocated cliprects.
   */
  L->SuperSaveClipRectCounter++;
  
  while (TRUE)
  {
    if (NULL != _CR->BitMap && TRUE == isSmart)
    {
      if (!(_CR->Flags & CR_SHAREDBITMAP))
        FreeBitMap(_CR->BitMap);
      _CR->BitMap = NULL;
    }
    if (NULL != _CR->Next)
    {
      L->SuperSaveClipRectCounter++;
      _CR = _CR->Next;
    }
    else
      break;
  }
  /* _CR is the last ClipRect in the list. I concatenate the
     currently preallocated list of ClipRects with that list. */
  _CR->Next = L->SuperSaveClipRects;
  
  /* CR is the head of the ClipRect list now */
  L->SuperSaveClipRects = CR;
}


/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/

/*
 * Make a hidden smart refresh ClipRect a view into the bitmap of the
 * ClipRect 'owner' that it lies in: no bitmap of its own, no copying.
 */
static void _MakeClipRectView(struct ClipRect *cr, struct ClipRect *owner)
{
  cr->BitMap = owner->BitMap;
  CR_SETBMOFF(cr, CR_BMOFFX(owner) + cr->bounds.MinX - owner->bounds.MinX,
                  CR_BMOFFY(owner) + cr->bounds.MinY - owner->bounds.MinY);
  cr->Flags |= CR_SHAREDBITMAP;
}

/*
 * Give a hidden ClipRect a (not cleared) bitmap of its own. 'aligndx' is
 * a translation the ClipRect undergoes later (MoveLayer): the content is
 * placed so that it is aligned for the final position.
 */
BOOL _AllocClipRectBitMap(struct ClipRect *cr, struct BitMap *display_bm, int aligndx, struct LayersBase *LayersBase)
{
  cr->BitMap = AllocBitMap(cr->bounds.MaxX - cr->bounds.MinX + 1 + ALIGN_CLIPRECT,
                           cr->bounds.MaxY - cr->bounds.MinY + 1,
                           display_bm->Depth,
                           0,
                           display_bm);
  CR_SETBMOFF(cr, ALIGN_OFFSET(cr->bounds.MinX + aligndx), 0);
  cr->Flags &= ~CR_SHAREDBITMAP;
  return (NULL != cr->BitMap);
}

/*
 * Give a hidden smart refresh ClipRect its backing store: a view into the
 * layer's surface when there is one that holds it, else a bitmap of its own.
 */
BOOL _BackClipRect(struct ClipRect *cr, struct Layer *l, int aligndx, struct LayersBase *LayersBase)
{
  if (NULL != IL(l)->surface &&
      cr->bounds.MinX >= l->bounds.MinX && cr->bounds.MinY >= l->bounds.MinY &&
      cr->bounds.MaxX - l->bounds.MinX < IL(l)->surfw &&
      cr->bounds.MaxY - l->bounds.MinY < IL(l)->surfh)
  {
    cr->BitMap = IL(l)->surface;
    CR_SETBMOFF(cr, cr->bounds.MinX - l->bounds.MinX, cr->bounds.MinY - l->bounds.MinY);
    cr->Flags |= CR_SHAREDBITMAP;
    return TRUE;
  }
  return _AllocClipRectBitMap(cr, l->rp->BitMap, aligndx, LayersBase);
}

/*
 * Make the layer's surface at least w x h, keeping its content and
 * re-pointing the ClipRects that view it. Content outside the old size
 * is undefined; the backfill of grown areas takes care of it.
 */
BOOL _EnsureSurfaceSize(struct Layer *l, WORD w, WORD h, struct LayersBase *LayersBase)
{
  struct BitMap *old = IL(l)->surface, *new;
  struct ClipRect *lists[3], *cr;
  ULONG newbytes = 0;
  int i;

  if (NULL == old)
    return FALSE;
  if (w <= IL(l)->surfw && h <= IL(l)->surfh)
    return TRUE;
  if (w < IL(l)->surfw) w = IL(l)->surfw;
  if (h < IL(l)->surfh) h = IL(l)->surfh;

  /* A composited surface stays within the screen's budget (the old one is replaced) */
  if (IS_COMPOSITED(l))
  {
    newbytes = _SurfaceBytes(l->rp->BitMap, w, h, IL(l)->alphasurface, LayersBase);
    if (!_SurfaceBudgetAllows(l->LayerInfo, l->rp->BitMap, newbytes, IL(l)->surfbytes, LayersBase))
      return FALSE;
  }

  /* An alpha surface keeps its format: the screen's layout with its alpha byte */
  if (IL(l)->alphasurface)
    new = AllocBitMap(w, h, 32, BMF_SPECIALFMT | SHIFT_PIXFMT(IL(l)->alphaformat) | BMF_CLEAR, l->rp->BitMap);
  else
    new = AllocBitMap(w, h, l->rp->BitMap->Depth, 0, l->rp->BitMap);
  if (NULL == new)
    return FALSE;
  if (IS_COMPOSITED(l))
    _AccountSurface(l, newbytes, LayersBase);

  BltBitMap(old, 0, 0, new, 0, 0, IL(l)->surfw, IL(l)->surfh, 0x0c0, 0xff, NULL);

  lists[0] = l->ClipRect;
  lists[1] = l->_cliprects;
  lists[2] = l->cr2;
  for (i = 0; i < 3; i++)
    for (cr = lists[i]; NULL != cr; cr = cr->Next)
      if (cr->BitMap == old)
        cr->BitMap = new;

  FreeBitMap(old);
  IL(l)->surface = new;
  IL(l)->surfw = w;
  IL(l)->surfh = h;
  return TRUE;
}

/***************************************************************************/
/*                           COMPOSITED LAYERS                             */
/***************************************************************************/

/*
 * The ClipRect list of a composited layer: exactly one hidden ClipRect
 * covering the layer, a view into the surface at (0,0). Nothing of the
 * layer is ever in the screen bitmap, so the visible/hidden split of the
 * other layer kinds does not apply; VisibleRegion is still maintained by
 * the callers for the hit test and the layers behind.
 */
void _SetCompositedClipRects(struct Layer *l, struct LayersBase *LayersBase)
{
  struct ClipRect * cr = l->ClipRect;

  if (NULL == cr)
  {
    cr = _AllocClipRect(l, LayersBase);
    l->ClipRect = cr;
    if (NULL == cr)
      return;
  }
  else if (NULL != cr->Next)
  {
    _FreeClipRectListBM(l, cr->Next, LayersBase);
    cr->Next = NULL;
  }

  cr->bounds = l->bounds;
  cr->lobs   = (struct Layer *)(IPTR)TRUE;
  cr->BitMap = IL(l)->surface;
  CR_SETBMOFF(cr, 0, 0);
  cr->Flags |= CR_SHAREDBITMAP;
}

/*
 * Bytes a surface of this size takes: chunky, 1, 2 or 4 bytes per pixel
 * from the screen's depth (a 24 bit deep screen is a 32 bit 0RGB/BGR0
 * layout here; a packed 24 bit one is counted generously), 4 for a per
 * pixel alpha surface. The same formula sizes the screen bitmap for the
 * budget, so the two match. (The BytesPerRow of a HIDD struct BitMap is
 * not the real row size, and the pixel format is not reachable from here.)
 */
ULONG _SurfaceBytes(struct BitMap *screenbm, LONG width, LONG height, BOOL alphasurface, struct LayersBase *LayersBase)
{
  ULONG depth = alphasurface ? 32 : GetBitMapAttr(screenbm, BMA_DEPTH);
  ULONG bpp   = (depth <= 8) ? 1 : (depth <= 16) ? 2 : 4;

  return (ULONG)width * (ULONG)height * bpp;
}

/*
 * May a composited surface of newbytes be allocated for this Layer_Info,
 * oldbytes being the surface it replaces (0 for a new one)? The surfaces
 * of a screen together may take LAYERS_SURFACE_BUDGET_FACTOR screen
 * bitmaps, and at least LAYERS_SURFACE_MINFREE of the largest free block
 * must stay free.
 */
BOOL _SurfaceBudgetAllows(struct Layer_Info *li, struct BitMap *screenbm, ULONG newbytes, ULONG oldbytes,
                          struct LayersBase *LayersBase)
{
  ULONG budget = LAYERS_SURFACE_BUDGET_FACTOR *
                 _SurfaceBytes(screenbm, GetBitMapAttr(screenbm, BMA_WIDTH), GetBitMapAttr(screenbm, BMA_HEIGHT), FALSE, LayersBase);
  ULONG inuse  = (NULL != LIE(li)) ? LIE(li)->lie_SurfaceBytes : 0;
  ULONG largest;

  inuse = (inuse > oldbytes) ? inuse - oldbytes : 0;
  if (inuse + newbytes > budget)
    return FALSE;

  largest = AvailMem(MEMF_ANY | MEMF_LARGEST);
  if (largest < newbytes || largest - newbytes < LAYERS_SURFACE_MINFREE)
    return FALSE;

  return TRUE;
}

/* Set the bytes the layer's surface is counted with in its Layer_Info (0: not counted) */
void _AccountSurface(struct Layer *l, ULONG bytes, struct LayersBase *LayersBase)
{
  struct LayerInfo_extra * lie = (NULL != l->LayerInfo) ? LIE(l->LayerInfo) : NULL;

  if (NULL != lie)
    lie->lie_SurfaceBytes = lie->lie_SurfaceBytes - IL(l)->surfbytes + bytes;
  IL(l)->surfbytes = bytes;
}

/*
 * Does the point (screen coordinates) hit the layer? Visible, inside the
 * shape and, for a composited layer with a per pixel alpha surface, not
 * on a fully transparent pixel. WhichLayer() and IsLayerHitAt().
 */
BOOL _IsLayerHitAt(struct Layer *l, LONG x, LONG y, struct LayersBase *LayersBase)
{
  if (!IS_VISIBLE(l) || !IsPointInRegion(l->visibleshape, x, y))
    return FALSE;

  if (IS_COMPOSITED(l) && IL(l)->alphasurface && NULL != IL(l)->surface)
    return 0 != LayerSurfaceAlpha(IL(l)->surface, x - l->bounds.MinX, y - l->bounds.MinY);

  return TRUE;
}

/*
 * Turn a composited layer into an ordinary smart refresh layer in place,
 * keeping its content: the layers behind back up what it now hides (a
 * blended one hid nothing), its visible parts go into the screen bitmap
 * and its surface stays as the backing store of its hidden ClipRects (as
 * far as it is big enough; _BackClipRect gives the rest bitmaps of their
 * own). Used when a growing surface is not allowed any more (budget) or
 * cannot be allocated. Called with the layers locked.
 */
void _UncompositeLayer(struct Layer *l, struct LayersBase *LayersBase)
{
  struct Layer_Info * li = l->LayerInfo;
  struct Layer * _l, * lparent;
  struct Region r;

  if (!IS_COMPOSITED(l))
    return;

  if (IS_TRANSLUCENT(l))
  {
    if (IS_VISIBLE(l))
    {
      /* as CreateLayerTagList() does for a new opaque layer */
      lparent = l->parent;
      _l = l->back;
      while (NULL != _l)
      {
        if (IS_VISIBLE(_l) && DO_OVERLAP(&l->visibleshape->bounds, &_l->shape->bounds))
          _BackupPartsOfLayer(_l, l->visibleshape, 0, FALSE, LayersBase);
        else
          ClearRegionRegion(l->visibleshape, _l->VisibleRegion);

        if (_l == lparent)
        {
          if (IS_VISIBLE(_l) || NULL == lparent || NULL == lparent->parent)
            break;
          lparent = lparent->parent;
        }
        _l = _l->back;
      }
    }
    if (IL(l)->occludeshape && IL(l)->occludeshape != l->visibleshape)
      DisposeRegion(IL(l)->occludeshape);
    IL(l)->occludeshape = l->visibleshape;
    IL(l)->intflags &= ~INTFLAG_TRANSLUCENT;
  }

  l->Flags &= ~LAYERCOMPOSITED;
  if (NULL != LIE(li))
    LIE(li)->lie_NumComposited--;
  _AccountSurface(l, 0, LayersBase);

  if (IS_VISIBLE(l))
  {
    /* its visible parts become ClipRects in the screen bitmap, copied from the surface */
    InitRegion(&r);
    SetRegion(l->VisibleRegion, &r);
    ClearRegion(l->VisibleRegion);
    _ShowPartsOfLayer(l, &r, LayersBase);
    ClearRegion(&r);
  }

  /* the display stops showing the surface */
  UPDATE_SURFACES(li);
}

/*
 * Rebuild the Layer_Info's back to front list of surface requests from its
 * visible composited layers and hand it to the display. Called once at the
 * end of every layer operation of a Layer_Info that has composited layers
 * (UPDATE_SURFACES). A request's Shape is the part of the layer that is
 * visible (its visibleshape inside its VisibleRegion, screen coordinates),
 * NULL when that is the whole rectangle. The Shape regions belong to the
 * layers and are only changed here, right before the hand over.
 */
void _UpdateSurfaces(struct Layer_Info *li, struct LayersBase *LayersBase)
{
  struct LayerInfo_extra * lie = LIE(li);
  struct HIDD_CompositeRequest * head = NULL, ** tail = &head;
  struct Layer * l;
  ULONG z = 0;

  if (NULL == lie || NULL == lie->lie_ScreenBM)
    return;

  /* bottom layer first */
  l = li->top_layer;
  while (NULL != l && NULL != l->back)
    l = l->back;

  for (; NULL != l; l = l->front)
  {
    struct HIDD_CompositeRequest * rq;
    struct Region * vis;

    if (!IS_COMPOSITED(l) || !IS_VISIBLE(l) || NULL == IL(l)->surface)
      continue;

    vis = AndRegionRegionND(l->visibleshape, l->VisibleRegion);
    if (NULL == vis)
      continue;
    if (IS_EMPTYREGION(vis))
    {
      DisposeRegion(vis);
      continue;
    }

    rq = &IL(l)->request;
    rq->BitMap    = HIDD_BM_OBJ(IL(l)->surface);
    rq->Src.MinX  = 0;
    rq->Src.MinY  = 0;
    rq->Src.MaxX  = l->Width - 1;
    rq->Src.MaxY  = l->Height - 1;
    rq->Dest      = l->bounds;

    if (NULL == vis->RegionRectangle->Next &&
        vis->bounds.MinX + vis->RegionRectangle->bounds.MinX == l->bounds.MinX &&
        vis->bounds.MinY + vis->RegionRectangle->bounds.MinY == l->bounds.MinY &&
        vis->bounds.MinX + vis->RegionRectangle->bounds.MaxX == l->bounds.MaxX &&
        vis->bounds.MinY + vis->RegionRectangle->bounds.MaxY == l->bounds.MaxY)
    {
      rq->Shape = NULL;
    }
    else
    {
      if (NULL == IL(l)->compshape)
        IL(l)->compshape = NewRegion();
      if (NULL != IL(l)->compshape && SetRegion(vis, IL(l)->compshape))
        rq->Shape = IL(l)->compshape;
      else
        rq->Shape = NULL; /* out of memory: show the whole rectangle rather than nothing */
    }
    DisposeRegion(vis);

    rq->Opacity = IL(l)->opacity;
    if (IL(l)->alphasurface)
      rq->AlphaMode = vHidd_CompAlpha_PerPixel;
    else if (IL(l)->opacity != 0xFF)
      rq->AlphaMode = vHidd_CompAlpha_Fixed;
    else
      rq->AlphaMode = vHidd_CompAlpha_None;
    rq->Flags  = (vHidd_CompAlpha_None == rq->AlphaMode) ? CRF_OPAQUE : 0;
    rq->ZOrder = z++;
    rq->Next   = NULL;

    *tail = rq;
    tail  = &rq->Next;
  }

  lie->lie_Surfaces = head;
  LayerSurfacesChanged(lie->lie_ScreenBM, head);
}

/*
 * Backfill a (screen relative) region of a composited layer into its surface.
 */
static void _BackFillSurface(struct Layer * l, struct Region * r, struct LayersBase * LayersBase)
{
  struct RegionRectangle * RR;
  struct BitMap * bm = l->rp->BitMap;

  if (NULL == IL(l)->surface)
    return;

  l->rp->BitMap = IL(l)->surface;
  for (RR = r->RegionRectangle; NULL != RR; RR = RR->Next)
  {
    struct Rectangle bmrect;

    bmrect.MinX = RR->bounds.MinX + r->bounds.MinX - l->bounds.MinX;
    bmrect.MinY = RR->bounds.MinY + r->bounds.MinY - l->bounds.MinY;
    bmrect.MaxX = RR->bounds.MaxX + r->bounds.MinX - l->bounds.MinX;
    bmrect.MaxY = RR->bounds.MaxY + r->bounds.MinY - l->bounds.MinY;

    _CallLayerHook(l->BackFill,
                   l->rp,
                   l,
                   &bmrect,
                   bmrect.MinX + l->Scroll_X,
                   bmrect.MinY + l->Scroll_Y,
                   LayersBase);
  }
  l->rp->BitMap = bm;
}

/*
 * From region r create a list of ClipRects: visible ones for r itself,
 * hidden ones for the rest of the layer (or of 'inverter' if given).
 * Hidden ClipRects of a smart refresh layer get a bitmap of their own,
 * or, if 'viewsof' is given (a clip region or damage list being derived
 * from the layer's regular ClipRects), become views into the hidden
 * ClipRects of that list, which they lie within.
 */
struct ClipRect * _CreateClipRectsFromRegion(struct Region *r,
                                             struct Layer * l,
                                             int invisible,
                                             struct Region * inverter,
                                             int aligndx,
                                             struct ClipRect * viewsof,
                                             struct LayersBase *LayersBase)
{
  int looped = FALSE;
  struct ClipRect * firstcr = NULL, * cr;
  struct BitMap * display_bm = l->rp->BitMap;

  /* A composited layer has no visible ClipRects: all are views into its surface */
  if (IS_COMPOSITED(l))
    invisible = TRUE;

  while (1)
  {
    struct RegionRectangle * rr = r->RegionRectangle;

    while (rr)
    {
      struct Rectangle bounds;

      bounds.MinX = rr->bounds.MinX + r->bounds.MinX;
      bounds.MinY = rr->bounds.MinY + r->bounds.MinY;
      bounds.MaxX = rr->bounds.MaxX + r->bounds.MinX;
      bounds.MaxY = rr->bounds.MaxY + r->bounds.MinY;

      if (TRUE == invisible && IS_SMARTREFRESH(l) && NULL != viewsof)
      {
        /*
         * Views: one per hidden ClipRect of 'viewsof' overlapping this
         * rectangle. Should they not cover it completely (they always
         * do), fall back to a ClipRect with a bitmap of its own.
         */
        struct ClipRect * ocr;
        int covered = 0;

        for (ocr = viewsof; NULL != ocr; ocr = ocr->Next)
        {
          struct Rectangle is;

          if (NULL != ocr->lobs && NULL != ocr->BitMap && AndRectRect(&bounds, &ocr->bounds, &is))
            covered += RECTAREA(&is);
        }

        if (covered >= RECTAREA(&bounds))
        {
          for (ocr = viewsof; NULL != ocr; ocr = ocr->Next)
          {
            struct Rectangle is;

            if (NULL == ocr->lobs || NULL == ocr->BitMap || !AndRectRect(&bounds, &ocr->bounds, &is))
              continue;
            cr = _AllocClipRect(l, LayersBase);
            cr->bounds = is;
            cr->lobs   = (struct Layer *)(IPTR)TRUE;
            _MakeClipRectView(cr, ocr);
            cr->Next = firstcr;
            firstcr  = cr;
          }
          rr = rr->Next;
          continue;
        }
      }

      cr = _AllocClipRect(l, LayersBase);
      cr->bounds = bounds;
      cr->lobs   = (struct Layer *)(IPTR)invisible;
      if (TRUE == invisible && IS_SMARTREFRESH(l))
        _BackClipRect(cr, l, aligndx, LayersBase);
      cr->Next = firstcr;
      firstcr  = cr;

      rr = rr->Next;
    }

    if (FALSE == looped)
    {
      /*
       * Flip the shape to the opposite part and
       * limit it to its own shape.
       */
      if (inverter)
        XorRegionRegion(inverter, r);
      else
        XorRectRegion(r,&l->bounds);

#if !CLIPRECTS_OUTSIDE_OF_SHAPE
      AndRegionRegion(l->shape,r);
#else
      AndRectRegion(r,&l->bounds);
#endif
      if (TRUE == invisible && !IS_COMPOSITED(l))
        invisible = FALSE;
      else
        invisible = TRUE;
      looped = TRUE;
    }
    else
      break;
  } /* while (1) */

  return firstcr;
}


int _CopyClipRectsToClipRects(struct Layer * l,
                              struct ClipRect * oldcr,
                              struct ClipRect * newcr,
                              int srcdx,
                              int destdx,
                              int backupmode,
                              int freelist,
                              int addtodamagelist,
                              int fresh,
                              struct LayersBase *LayersBase)
{
  struct BitMap * display_bm = l->rp->BitMap;

  /*
   * Every ClipRect of a composited layer is a view into its surface:
   * there is nothing to copy and nothing to back up.
   */
  if (IS_COMPOSITED(l))
  {
    if (TRUE == freelist)
    {
      while (NULL != oldcr)
      {
        struct ClipRect * _cr = oldcr->Next;
        _FreeClipRect(oldcr, l, LayersBase);
        oldcr = _cr;
      }
    }
    if (IS_EMPTYREGION(l->DamageList))
      l->Flags &= ~LAYERREFRESH;
    else
      l->Flags |=  LAYERREFRESH;
    return TRUE;
  }

  /*
   * New backing bitmaps are allocated uncleared. Normally the old
   * ClipRects tile the whole layer and fill them completely below. The
   * parts they do not cover (the layer grew while covered) are
   * backfilled into the backing bitmap, as AmigaOS does: the hook is
   * called with the ClipRect's bitmap in the RastPort, the bounds in
   * bitmap coordinates and the offsets in layer coordinates. Only for
   * freshly created lists: when copying back into an existing list
   * (EndUpdate, clip region removal) the destination already holds
   * valid content outside the source.
   */
  if (fresh && IS_SMARTREFRESH(l))
  {
    struct ClipRect * _cr;

    for (_cr = newcr; NULL != _cr; _cr = _cr->Next)
    {
      struct ClipRect * ocr;
      struct Region * uncovered;
      struct RegionRectangle * rr;
      int covered = 0, area;

      if (NULL == _cr->lobs || NULL == _cr->BitMap)
        continue;

      area = RECTAREA(&_cr->bounds);
      for (ocr = oldcr; NULL != ocr && covered < area; ocr = ocr->Next)
      {
        struct Rectangle is;

        if (AndRectRect(&_cr->bounds, &ocr->bounds, &is))
          covered += RECTAREA(&is);
      }
      if (covered >= area)
        continue;

      if (l->BackFill == LAYERS_NOBACKFILL || NULL == (uncovered = NewRegion()))
      {
        /* nothing will paint it: at least do not show stale memory */
        BltBitMap(_cr->BitMap, 0, 0, _cr->BitMap, 0, 0,
                  _cr->bounds.MaxX - _cr->bounds.MinX + 1 + ALIGN_CLIPRECT,
                  _cr->bounds.MaxY - _cr->bounds.MinY + 1,
                  0x000, 0xff, NULL);
        continue;
      }

      OrRectRegion(uncovered, &_cr->bounds);
      for (ocr = oldcr; NULL != ocr; ocr = ocr->Next)
      {
        struct Rectangle is;

        if (AndRectRect(&_cr->bounds, &ocr->bounds, &is))
          ClearRectRegion(uncovered, &is);
      }

      for (rr = uncovered->RegionRectangle; NULL != rr; rr = rr->Next)
      {
        struct Rectangle screenrect, bmrect;
        struct BitMap * bm = l->rp->BitMap;

        screenrect.MinX = rr->bounds.MinX + uncovered->bounds.MinX;
        screenrect.MinY = rr->bounds.MinY + uncovered->bounds.MinY;
        screenrect.MaxX = rr->bounds.MaxX + uncovered->bounds.MinX;
        screenrect.MaxY = rr->bounds.MaxY + uncovered->bounds.MinY;

        bmrect.MinX = screenrect.MinX - _cr->bounds.MinX + CR_BMOFFX(_cr);
        bmrect.MinY = screenrect.MinY - _cr->bounds.MinY + CR_BMOFFY(_cr);
        bmrect.MaxX = screenrect.MaxX - _cr->bounds.MinX + CR_BMOFFX(_cr);
        bmrect.MaxY = screenrect.MaxY - _cr->bounds.MinY + CR_BMOFFY(_cr);

        l->rp->BitMap = _cr->BitMap;
        _CallLayerHook(l->BackFill,
                       l->rp,
                       l,
                       &bmrect,
                       screenrect.MinX - l->bounds.MinX + l->Scroll_X,
                       screenrect.MinY - l->bounds.MinY + l->Scroll_Y,
                       LayersBase);
        l->rp->BitMap = bm;
      }
      DisposeRegion(uncovered);
    }
  }

  while (NULL != oldcr)
  {
    struct ClipRect * _cr = newcr;
    int area = RECTAREA(&oldcr->bounds);
    while ((NULL != _cr) &&  (0 != area) )
    {
      struct Rectangle intersect;
      
      /*
       * Do the two rectangles overlap?
       */
      if (AndRectRect(&_cr->bounds, &oldcr->bounds, &intersect))
      {
        LONG xSize = intersect.MaxX - intersect.MinX + 1;
        LONG ySize = intersect.MaxY - intersect.MinY + 1;
        
        /*
         * Is this new one supposed to be invisible?
         */
        if (NULL != _cr->lobs)
        {
          struct BitMap * srcbm;
          /*
           * The new one is supposed to be invisible.
           * So for SIMPLEREFRESH layers I don't have to
           * do anything if
           * a) not in backupmode or
           * b) old cr was invisible
           */
          if (IS_SIMPLEREFRESH(l) && FALSE == backupmode && NULL == _cr->BitMap)
          {
            if (FALSE == addtodamagelist)
            {
              struct Rectangle rect = intersect;
            
              _TranslateRect(&rect, -l->bounds.MinX, -l->bounds.MinY);

/* FIXME:
 * stegerg: Not sure if this is a good idea. What for example if
 * updating is done in several passes? And CopyClipRectsToClipRects is
 * used by all kinds of functions including BeginUpdate/EndUpdate/InstallClipRegion/etc.
 */
              ClearRectRegion(l->DamageList, &rect);
#if 0
kprintf("");
kprintf("%s: Removing %d/%d-%d/%d from damagelist!\t",
        __FUNCTION__,
        rect.MinX,
        rect.MinY,
        rect.MaxX,
        rect.MaxY
        );
kprintf("%s: Layer: %d/%d-%d/%d!\n",
        __FUNCTION__,
        l->bounds.MinX,
        l->bounds.MinY,
        l->bounds.MaxX,
        l->bounds.MaxY
        );
kprintf("%s: oldcr: %d/%d-%d/%d!\t",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY
        );
kprintf("%s: _cr: %d/%d-%d/%d!\n\n",
        __FUNCTION__,
        _cr->bounds.MinX,
        _cr->bounds.MinY,
        _cr->bounds.MaxX,
        _cr->bounds.MaxY
        );
#endif
            }
          }
          else if (IS_SIMPLEREFRESH(l) && TRUE == backupmode && NULL  != oldcr->lobs)
          {
            if (TRUE == addtodamagelist)
            {
              struct Rectangle rect = intersect;
              _TranslateRect(&rect, -l->bounds.MinX, -l->bounds.MinY);
            
              // FIXME (if possible)
              // !!! Also areas where a child disappears beyond the
              // boundaries of its parent are added here!
              OrRectRegion(l->DamageList, &rect);
#if 0
kprintf("_cr->BitMap: %p ,_cr->lobs: %d\n",_cr->BitMap,_cr->lobs);
#endif
#if 0
kprintf("%s: Adding %d/%d-%d/%d to damagelist of l=%p!\t",
        __FUNCTION__,
        rect.MinX,
        rect.MinY,
        rect.MaxX,
        rect.MaxY,
        l
        );
#endif
#if 0
kprintf("%s: Layer: %d/%d-%d/%d!\n",
        __FUNCTION__,
        l->bounds.MinX,
        l->bounds.MinY,
        l->bounds.MaxX,
        l->bounds.MaxY
        );
kprintf("%s: oldcr: %d/%d-%d/%d!\t",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY
        );
kprintf("%s: _cr: %d/%d-%d/%d!\n\n",
        __FUNCTION__,
        _cr->bounds.MinX,
        _cr->bounds.MinY,
        _cr->bounds.MaxX,
        _cr->bounds.MaxY
        );
#endif
            } else {
//kprintf("Not adding to damage list for l=%p!\n",l);
            }
          }
          else
          {
            LONG xSrc, xDest;
            LONG ySrc, yDest;
            struct BitMap * destbm;

            if (IS_SUPERREFRESH(l))
            {
              /*
               * Superbitmap layer: the superbitmap is the only backing
               * store. A part that was already hidden is in it; a part
               * that was on screen is copied to where it belongs. The
               * superbitmap coordinates are the rastport coordinates,
               * and the layer shows the superbitmap from Scroll_X/Y.
               */
              if (NULL == oldcr->lobs)
              {
                BltBitMap(l->rp->BitMap,
                          intersect.MinX,
                          intersect.MinY,
                          l->SuperBitMap,
                          intersect.MinX - l->bounds.MinX + l->Scroll_X,
                          intersect.MinY - l->bounds.MinY + l->Scroll_Y,
                          xSize,
                          ySize,
                          0x0c0,
                          0xff,
                          NULL);
              }
              area -= (xSize * ySize);
            }
            else
            {

              /*
               * Copy the intersection into the new hidden ClipRect's
               * bitmap, from the old ClipRect's bitmap or from the screen.
               */
              if (IS_SIMPLEREFRESH(l) && NULL == _cr->BitMap && TRUE == backupmode)
              {
                /* Get a bitmap (if not there) and make a backup */
                _AllocClipRectBitMap(_cr, display_bm, destdx, LayersBase);
              }
              destbm = _cr->BitMap;
              xDest  = intersect.MinX - _cr->bounds.MinX + CR_BMOFFX(_cr);
              yDest  = intersect.MinY - _cr->bounds.MinY + CR_BMOFFY(_cr);

              if (oldcr->lobs)
              {
                srcbm = oldcr->BitMap;
                xSrc  = intersect.MinX - oldcr->bounds.MinX + CR_BMOFFX(oldcr);
                ySrc  = intersect.MinY - oldcr->bounds.MinY + CR_BMOFFY(oldcr);
              }
              else
              {
                srcbm = l->rp->BitMap;
                xSrc  = intersect.MinX;
                ySrc  = intersect.MinY;
              }

              /* A view of the same content needs no copy */
              if (NULL != srcbm && NULL != destbm &&
                  !(srcbm == destbm && xSrc == xDest && ySrc == yDest))
              {
                BltBitMap(srcbm,
                          xSrc,
                          ySrc,
                          destbm,
                          xDest,
                          yDest,
                          xSize,
                          ySize,
                          0x0c0,
                          0xff,
                          NULL);
              }
              area -= (xSize * ySize);
            }
            
          }
        }
        else //if (FALSE == backupmode)
        {
          /*
           * The new one is visible. if the old one was not visible
           * then I have to show it.
           * If it is a simple refresh layer and it has it
           * backed up (only when moving!) then I must show
           * this. If it has nothing backed up then I must
           * add a part to the damage list.
           */
          if (IS_SIMPLEREFRESH(l) &&
              (NULL != oldcr->lobs) &&
              (NULL == oldcr->BitMap))
          {
            if (NULL != oldcr->lobs && NULL == oldcr->BitMap)
            {
              struct Rectangle rect = intersect;
           
              _CallLayerHook(l->BackFill,
                             l->rp,
                             l,
                             &rect,
                             rect.MinX,
                             rect.MinY,
                             LayersBase);
              _TranslateRect(&rect, -l->bounds.MinX, -l->bounds.MinY);
              OrRectRegion(l->DamageList, &rect);
#if 0
kprintf("Adding: %d\n",addtodamagelist);
kprintf("%s: Adding %d/%d-%d/%d to damagelist!\t",
        __FUNCTION__,
        rect.MinX,
        rect.MinY,
        rect.MaxX,
        rect.MaxY
        );
kprintf("%s: Layer: %d/%d-%d/%d!\n",
        __FUNCTION__,
        l->bounds.MinX,
        l->bounds.MinY,
        l->bounds.MaxX,
        l->bounds.MaxY
        );
kprintf("%s: oldcr: %d/%d-%d/%d!\t",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY
        );
kprintf("%s: _cr: %d/%d-%d/%d!\n",
        __FUNCTION__,
        _cr->bounds.MinX,
        _cr->bounds.MinY,
        _cr->bounds.MaxX,
        _cr->bounds.MaxY
        );
#endif
            }
          }
          else
          {
//kprintf("%s: Showing a part of a backed up bitmap!\n",__FUNCTION__);
            if (NULL != oldcr->lobs)
            {
              /*
               * Copy out of hidden bitmap
               */
              LONG xSrc, xDest;
              LONG ySrc, yDest;
              struct BitMap * srcbm;
              
              xDest = intersect.MinX;
              yDest = intersect.MinY;
              if (IS_SUPERREFRESH(l))
              {
                /*
                 * Superbitmap layer: the part is in the superbitmap at
                 * its rastport coordinates; the layer shows the
                 * superbitmap from Scroll_X/Y.
                 */
                srcbm = l->SuperBitMap;
                xSrc  = intersect.MinX - l->bounds.MinX + l->Scroll_X;
                ySrc  = intersect.MinY - l->bounds.MinY + l->Scroll_Y;
              }
              else
              {
                /*
                 * From the hidden bitmap (a moved simple refresh layer
                 * may have one too).
                 */
                srcbm = oldcr->BitMap;
                xSrc  = intersect.MinX - oldcr->bounds.MinX + CR_BMOFFX(oldcr);
                ySrc  = intersect.MinY - oldcr->bounds.MinY + CR_BMOFFY(oldcr);
              }
              
#if 0
kprintf("\t\t%s: Show cliprect: %d/%d-%d/%d; blitting to %d/%d _cr->lobs: %d\n",
        __FUNCTION__,
        oldcr->bounds.MinX,
        oldcr->bounds.MinY,
        oldcr->bounds.MaxX,
        oldcr->bounds.MaxY,
        xDest,
        yDest,
        _cr->lobs);
#endif



              if (NULL != srcbm)
              BltBitMap(srcbm,
                        xSrc,
                        ySrc,
                        l->rp->BitMap,
                        xDest,
                        yDest,
                        xSize,
                        ySize,
                        0x0c0,
                        0xff,
                        NULL);
              
            } /* if was hidden cliprect */
          } /* if is simple else ... */
        } /* if new cliprect is visible or invisible   */
      } /* if rectangles overlap */
      else
      {

        if (IS_SMARTREFRESH(l) && TRUE == backupmode && NULL == _cr->BitMap && NULL != _cr->lobs)
        {
           _cr->BitMap = AllocBitMap(_cr->bounds.MaxX - _cr->bounds.MinX + 1 + ALIGN_CLIPRECT ,
                                     _cr->bounds.MaxY - _cr->bounds.MinY + 1,
                                     display_bm->Depth,
                                     BMF_CLEAR,
                                     display_bm);
        }
      }
      _cr = _cr->Next;
    } /* all new cliprects */
    
    if (TRUE == freelist)
    {
      _cr = oldcr->Next;
      if (oldcr->BitMap && !(oldcr->Flags & CR_SHAREDBITMAP))
        FreeBitMap(oldcr->BitMap);
      _FreeClipRect(oldcr, l, LayersBase);
      oldcr = _cr;
    }
    else
      oldcr = oldcr->Next;
  } /* for all old cliprects */

  if (IS_EMPTYREGION(l->DamageList))
    l->Flags &= ~LAYERREFRESH;
  else
    l->Flags |=  LAYERREFRESH;

  /*
   * If this is a simple refresh layer and I am not in
   * backup mode and I am not adding to the damagelist
   * then I must call the backfillhook for the
   * area of the damage list of a simple refresh layer
   */

  if (IS_SIMPLEREFRESH(l) &&
      (l->Flags & LAYERREFRESH) &&
      FALSE == backupmode &&
      FALSE == addtodamagelist)
  {
    struct Region * dr = l->DamageList;
    struct RegionRectangle * rr;

    _TranslateRect(&dr->bounds, l->bounds.MinX, l->bounds.MinY);
    AndRectRegion(dr, &l->bounds);
    AndRegionRegion(l->VisibleRegion, dr);
    AndRegionRegion(l->visibleshape, dr);
    
    _TranslateRect(&dr->bounds, -l->bounds.MinX, -l->bounds.MinY);
    
    rr = dr->RegionRectangle;
    while (rr)
    {
      _TranslateRect(&rr->bounds,
                     dr->bounds.MinX + l->bounds.MinX,
                     dr->bounds.MinY + l->bounds.MinY);

      _CallLayerHook(l->BackFill,
                     l->rp,
                     l,
                     &rr->bounds,
                     rr->bounds.MinX,
                     rr->bounds.MinY,
                     LayersBase);

      _TranslateRect(&rr->bounds,
                     -dr->bounds.MinX-l->bounds.MinX,
                     -dr->bounds.MinY-l->bounds.MinY);
      rr = rr->Next;
 
    }
  }

  return TRUE;
}

/*
 * Backup any parts of the layer that overlap with the backup_region
 * and that are not already backed up. Create the cliprects and
 * bitmaps if necessary.
 * Assumption: Only visible parts become invisible,
 *             invisible parts will not become visible.
 *
 * This function MUST not manipulate hide_region!!!!
 */
int _BackupPartsOfLayer(struct Layer * l,
                        struct Region * hide_region,
                        int dx,
                        int backupsimplerefresh,
                        struct LayersBase * LayersBase)
{
  struct ClipRect * newcr;
  struct Region *r, * clipregion;

  /*
   * Uninstall clipping region. This causes all pixels to
   * be copied into the cliprects that cover the complete
   * area of the layer.
   */

  clipregion = _InternalInstallClipRegion(l, NULL, 0, 0, LayersBase);

  ClearRegionRegion(hide_region,l->VisibleRegion);

  if (IS_COMPOSITED(l))
  {
    /* nothing to copy: the layer lives in its surface, the display composes it */
    _SetCompositedClipRects(l, LayersBase);
    r = NULL;
  }
  else
    r = AndRegionRegionND(l->visibleshape, l->VisibleRegion);
  
  if (r)
  {
     newcr = _CreateClipRectsFromRegion(r,l,FALSE,NULL,dx,NULL,LayersBase);
     DisposeRegion(r);
     if (newcr)
     {
         _CopyClipRectsToClipRects(l,
                                   l->ClipRect /* source */,
                                   newcr  /* destination */,
                                   0,
                                   dx,
                                   backupsimplerefresh,
                                   TRUE,
                                   TRUE,
                                   TRUE,
                                   LayersBase);

          l->ClipRect = newcr;
     }
  }
  
  /*
   * Reinstall the clipping region. This causes the
   * whole visible area of the layer to be copied
   * into the clipping regions cliprects. The
   * regular list of cliprects is still maintained.
   */
  if (clipregion)
    _InternalInstallClipRegion(l, clipregion, dx, dx, LayersBase);

  return TRUE;
}

/*
 * Show any parts of the layer that overlap with the backup_region
 * and that are not already show.
 *
 * This function MUST not manipulate show_region!!!!
 */

int _ShowPartsOfLayer(struct Layer * l,
                      struct Region * show_region,
                      struct LayersBase * LayersBase)
{
  struct ClipRect * newcr;
  struct Region *r;
  struct Region * clipregion;

//kprintf("%s called for %p\n",__FUNCTION__,l);

  /*
   * If there is a clipping region then the whole
   * window is currently backed up in l->ClipRect
   * That covers the complete area. I must first
   * make these visible, move them back to
   * l->_cliprects and recreate the clipping cliprects
   * according to the clipregion
   */

 if (show_region == l->VisibleRegion)
   kprintf("ERROR - same regions!! %s\n",__FUNCTION__);

  clipregion = InstallClipRegion(l, NULL);

  OrRegionRegion(show_region,l->VisibleRegion);

  if (IS_COMPOSITED(l))
  {
    _SetCompositedClipRects(l, LayersBase);
    r = NULL;
  }
  else
    r = AndRegionRegionND(l->visibleshape, l->VisibleRegion);
  if (r != NULL)
  {
      newcr = _CreateClipRectsFromRegion(r,l,FALSE,NULL,0,NULL,LayersBase);
      DisposeRegion(r);
      _CopyClipRectsToClipRects(l,
                                l->ClipRect /* source */,
                                newcr /* destination */,
                                0,
                                0,
                                FALSE,
                                TRUE,
                                FALSE,
                                TRUE,
                                LayersBase);


      l->ClipRect = newcr;
  }
  if (clipregion)
    InstallClipRegion(l, clipregion);

  return TRUE;
}

int _ShowLayer(struct Layer * l, struct LayersBase *LayersBase)
{
  struct Region *r;
  struct RegionRectangle * rr;
  struct ClipRect * prevcr = NULL;
  struct BitMap * bm = l->rp->BitMap;
  int invisible = FALSE;

  if (IS_COMPOSITED(l))
  {
    /* One ClipRect, the surface; the backfill hook paints the whole surface */
    struct Region * all = NewRectRegion(l->bounds.MinX, l->bounds.MinY, l->bounds.MaxX, l->bounds.MaxY);

    _SetCompositedClipRects(l, LayersBase);
    if (all)
    {
      _BackFillSurface(l, all, LayersBase);
      DisposeRegion(all);
    }
    return TRUE;
  }

  r = AndRegionRegionND(l->shape, l->VisibleRegion);
  AndRegionRegion(l->parent->shape, r);

  while (1)
  {
    rr = r->RegionRectangle;

    while (NULL != rr)
    {
      struct ClipRect * cr;

      cr = _AllocClipRect(l, LayersBase);

//kprintf("\t\tinvisible: %d !!!!!!!!!!!!\n",invisible);

      MinX(cr) = MinX(rr) + MinX(r);
      MinY(cr) = MinY(rr) + MinY(r);
      MaxX(cr) = MaxX(rr) + MinX(r);
      MaxY(cr) = MaxY(rr) + MinY(r);
      cr->lobs = (struct Layer *)(IPTR)invisible;
#if 0
kprintf("\t\t%s: Created cliprect %d/%d-%d/%d invisible: %d\n",
        __FUNCTION__,
        cr->bounds.MinX,
        cr->bounds.MinY,
        cr->bounds.MaxX,
        cr->bounds.MaxY,
        invisible);
#endif
      if (prevcr)
        prevcr->Next = cr;
      else
        l->ClipRect = cr;

      prevcr = cr;

      if (FALSE == invisible)
      {
#if 0
kprintf("\t\tClearing background! %d/%d-%d/%d  bitmap: %p\n",
         cr->bounds.MinX,
         cr->bounds.MinY,
         cr->bounds.MaxX,
         cr->bounds.MaxY,
         l->rp->BitMap
         );
#endif
        if (IS_SUPERREFRESH(l))
        {
          BltBitMap(l->SuperBitMap,
                    cr->bounds.MinX - l->bounds.MinX + l->Scroll_X,
                    cr->bounds.MinY - l->bounds.MinY + l->Scroll_Y,
                    l->rp->BitMap,
                    cr->bounds.MinX,
                    cr->bounds.MinY,
                    cr->bounds.MaxX - cr->bounds.MinX + 1,
                    cr->bounds.MaxY - cr->bounds.MinY + 1,
                    0x0c0,
                    0xff,
                    NULL);
        }
        else
        {
          _CallLayerHook(l->BackFill,
                         l->rp,
                         l,
                         &cr->bounds,
                         cr->bounds.MinX,
                         cr->bounds.MinY,
                         LayersBase);
        }
      }
      else
      {
        /*
         * This part is to be invisible! The backfill hook is run into
         * the backing bitmap, as AmigaOS does (verified on Kickstart
         * 3.1: CreateBehindHookLayer() under another layer calls the
         * hook for the hidden part too).
         */
        if (IS_SMARTREFRESH(l))
        {
          if (_BackClipRect(cr, l, 0, LayersBase))
          {
            struct Rectangle bmrect;

            bmrect.MinX = CR_BMOFFX(cr);
            bmrect.MinY = CR_BMOFFY(cr);
            bmrect.MaxX = bmrect.MinX + cr->bounds.MaxX - cr->bounds.MinX;
            bmrect.MaxY = bmrect.MinY + cr->bounds.MaxY - cr->bounds.MinY;

            l->rp->BitMap = cr->BitMap;
            _CallLayerHook(l->BackFill,
                           l->rp,
                           l,
                           &bmrect,
                           cr->bounds.MinX - l->bounds.MinX + l->Scroll_X,
                           cr->bounds.MinY - l->bounds.MinY + l->Scroll_Y,
                           LayersBase);
            l->rp->BitMap = bm;
          }
        }
      }

      rr=rr->Next;
    }

    if (FALSE == invisible)
    {
      XorRectRegion(r, &l->bounds);
#if !CLIPRECTS_OUTSIDE_OF_SHAPE
      AndRegionRegion(l->shape, r);
#endif
      invisible = TRUE;
    }
    else
      break;
  }

  DisposeRegion(r);

  return TRUE;
}

/*
 * It is assumed that the region r is not needed anymore.
 */
void _BackFillRegion(struct Layer * l,
                     struct Region * r,
                     int addtodamagelist,
                     struct LayersBase * LayersBase)
{
  struct RegionRectangle * RR;

  RR = r->RegionRectangle;
  if (NULL == RR) return;
  
  if (IS_SIMPLEREFRESH(l))
  {
    /* Only for simple refresh layers, becuase smart refresh layers
       may have damage outside of visibleshape, like when being dragged
       off screen */
       
    AndRegionRegion(l->visibleshape, r);
  }
  else
  {
    /* Maybe not needed, but to be sure ... */
    
    AndRectRegion(r, &l->bounds);
  }
  
  
  if (TRUE == addtodamagelist)
  {
    l->Flags |= LAYERREFRESH;

#if 1
     /* Region coords are screen relative, but damagelist coords are layer relative! */
      
    _TranslateRect(&r->bounds, -l->bounds.MinX, -l->bounds.MinY);
    OrRegionRegion(r, l->DamageList);
    _TranslateRect(&r->bounds, l->bounds.MinX, l->bounds.MinY);
    
#else
    while (NULL != RR)
    {
      struct Rectangle rect = RR->bounds;

//kprintf("%s: adding to damagelist!\n",__FUNCTION__);

      /* Region coords are screen relative, but damagelist coords are layer relative! */

      _TranslateRect(&rect,
                     r->bounds.MinX - l->bounds.MinX,
                     r->bounds.MinY - l->bounds.MinY);
#if 0
kprintf("%s: Adding %d/%d-%d/%d to damagelist!\n",
      __FUNCTION__,
      rect.MinX,
      rect.MinY,
      rect.MaxX,
      rect.MaxY
      );
#endif
      OrRectRegion(l->DamageList, &rect);

      _TranslateRect(&rect,
                     -r->bounds.MinX + l->bounds.MinX,
                     -r->bounds.MinY + l->bounds.MinY);

      RR = RR->Next;
      
    } /* while (NULL != RR) */
#endif
    
  } /* if (TRUE == addtodamagelist) */

  if (IS_COMPOSITED(l))
  {
    /* The whole area, covered or not: the layer is complete in its surface */
    if (l->shaperegion)
    {
      _TranslateRect(&r->bounds, -l->bounds.MinX, -l->bounds.MinY);
      AndRegionRegion(l->shaperegion, r);
      _TranslateRect(&r->bounds, l->bounds.MinX, l->bounds.MinY);
    }
    _BackFillSurface(l, r, LayersBase);
    return;
  }

  AndRegionRegion(l->VisibleRegion, r);
  if (l->shaperegion)
  {
    /* shaperegion is layer relative, while r is screen relative */
    
    _TranslateRect(&r->bounds, -l->bounds.MinX, -l->bounds.MinY);
    AndRegionRegion(l->shaperegion, r);
    _TranslateRect(&r->bounds, l->bounds.MinX, l->bounds.MinY);
  }
  
  RR  = r->RegionRectangle;
  /* check if a region is empty */
  while (NULL != RR)
  {
     _TranslateRect(&RR->bounds, r->bounds.MinX, r->bounds.MinY);

#if 0
kprintf("\t\t: %s Clearing rect : %d/%d-%d/%d  layer: %p, hook: %p, bitmap: %p\n",
        __FUNCTION__,
        RR->bounds.MinX,
        RR->bounds.MinY,
        RR->bounds.MaxX,
        RR->bounds.MaxY,
        l,
        l->BackFill,
        l->rp->BitMap);
#endif
     if (IS_SUPERREFRESH(l))
     {
       /*
        * A superbitmap layer has no backfill: the newly exposed area
        * shows the superbitmap (rastport coordinates, shown from
        * Scroll_X/Y).
        */
       BltBitMap(l->SuperBitMap,
                 RR->bounds.MinX - l->bounds.MinX + l->Scroll_X,
                 RR->bounds.MinY - l->bounds.MinY + l->Scroll_Y,
                 l->rp->BitMap,
                 RR->bounds.MinX,
                 RR->bounds.MinY,
                 RR->bounds.MaxX - RR->bounds.MinX + 1,
                 RR->bounds.MaxY - RR->bounds.MinY + 1,
                 0x0c0,
                 0xff,
                 NULL);
     }
     else
     _CallLayerHook(l->BackFill,
                    l->rp,
                    l,
                    &RR->bounds,
                    RR->bounds.MinX,
                    RR->bounds.MinY,
                    LayersBase);
    RR = RR->Next;
  }

}

struct Region *_InternalInstallClipRegion(struct Layer *l, struct Region *region,
                                          WORD srcdx, WORD destdx,
                                          struct LayersBase *LayersBase)
{
  struct Region * OldRegion;
  BOOL updating = FALSE;
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
        if (IS_SMARTREFRESH(l))
          _CopyClipRectsToClipRects(l,
                                    l->ClipRect,
                                    l->_cliprects,
                                    srcdx,
                                    destdx,
                                    FALSE,
                                    TRUE,
                                    FALSE,
                                    FALSE,
                                    LayersBase);
        else
          _FreeClipRectListBM(l, l->ClipRect, LayersBase);
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
      struct Region *r;

      /* convert the region to a list of ClipRects */
      /* backup the old cliprects */
      l->_cliprects = l->ClipRect;

      _TranslateRect(&region->bounds, l->bounds.MinX, l->bounds.MinY);

      r = AndRegionRegionND(l->VisibleRegion, region);
      AndRegionRegion(l->shape, r);
      
      l->ClipRect = _CreateClipRectsFromRegion(r,
                                               l,
                                               FALSE,
                                               region,
                                               destdx,
                                               l->_cliprects,
                                               LayersBase);
      DisposeRegion(r);

      _CopyClipRectsToClipRects(l,
                                l->_cliprects,
                                l->ClipRect,
                                srcdx,
                                destdx,
                                FALSE,
                                FALSE,
                                TRUE,
                                TRUE,
                                LayersBase); /* stegerg: should be FALSE. but that does not work??? */

      _TranslateRect(&region->bounds, -l->bounds.MinX, -l->bounds.MinY);

      /* right now I am assuming that everything went alright */
    }

    if (updating)
      BeginUpdate(l);

  } /* if ((OldRegion != NULL) || (region != NULL)) */

  return OldRegion;
}
