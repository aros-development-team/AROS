/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <intuition/extensions.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <cybergraphx/cybergraphics.h>

#include "basicfuncs.h"
#include "layers_intern.h"

/*
 * The tags CreateLayerTagList() understands. Their values are collected in
 * an array indexed by LATAG_xxx; latags[] maps each index to its tag.
 */
enum
{
  LATAG_BackfillHook, LATAG_SuperBitMap, LATAG_ChildOf, LATAG_InFrontOf, LATAG_Behind,
  LATAG_Hidden, LATAG_ShapeRegion, LATAG_ShapeHook, LATAG_WindowPtr, LATAG_Composited,
  LATAG_Opacity, LATAG_AlphaSurface, LATAG_InputShape, LATAG_COUNT
};

static const Tag latags[LATAG_COUNT] =
{
  [LATAG_BackfillHook] = LA_BackfillHook,
  [LATAG_SuperBitMap]  = LA_SuperBitMap,
  [LATAG_ChildOf]      = LA_ChildOf,
  [LATAG_InFrontOf]    = LA_InFrontOf,
  [LATAG_Behind]       = LA_Behind,
  [LATAG_Hidden]       = LA_Hidden,
  [LATAG_ShapeRegion]  = LA_ShapeRegion,
  [LATAG_ShapeHook]    = LA_ShapeHook,
  [LATAG_WindowPtr]    = LA_WindowPtr,
  [LATAG_Composited]   = LA_Composited,
  [LATAG_Opacity]      = LA_Opacity,
  [LATAG_AlphaSurface] = LA_AlphaSurface,
  [LATAG_InputShape]   = LA_InputShape
};

/*
 * LA_InFrontOf and LA_Behind are intentionally left
 * because they allow to implement AmigaOS4-compatible functionality.
 */
struct Layer *CreateLayerTagList(struct Layer_Info *li, struct BitMap *bm, LONG x0, LONG y0,
                                 LONG x1, LONG y1, LONG flags, int priority, struct TagItem *tagList,
                                 struct LayersBase *LayersBase)
{
  struct BitMap * superbitmap;
  struct Hook * hook, *shapehook;
  APTR win;
  int visible;
  int composited, alphasurface;
  ULONG alphaformat = (ULONG)-1;
  ULONG opacity;
  struct Layer * behind, * infrontof, * parent;
  struct Layer * l;
  struct Region * layershape, *shape;
  struct Region * inputshape;
  struct TagItem *tag, *tstate = tagList;
  IPTR tv[LATAG_COUNT];
  int i;

  for (i = 0; i < LATAG_COUNT; i++)
    tv[i] = 0;
  tv[LATAG_Opacity] = 0xFF;

  while((tag = NextTagItem(&tstate)))
  {
    for (i = 0; i < LATAG_COUNT; i++)
    {
      if (tag->ti_Tag == latags[i])
      {
        tv[i] = tag->ti_Data;
        break;
      }
    }
  } /* while((tag = NextTagItem(&tstate))) */

  hook         = (struct Hook *)tv[LATAG_BackfillHook];
  superbitmap  = (struct BitMap *)tv[LATAG_SuperBitMap];
  parent       = (struct Layer *)tv[LATAG_ChildOf];
  infrontof    = (struct Layer *)tv[LATAG_InFrontOf];
  behind       = (struct Layer *)tv[LATAG_Behind];
  visible      = !tv[LATAG_Hidden];
  layershape   = (struct Region *)tv[LATAG_ShapeRegion];
  shapehook    = (struct Hook *)tv[LATAG_ShapeHook];
  win          = (APTR)tv[LATAG_WindowPtr];
  composited   = tv[LATAG_Composited] ? TRUE : FALSE;
  opacity      = (tv[LATAG_Opacity] > 0xFF) ? 0xFF : tv[LATAG_Opacity];
  alphasurface = tv[LATAG_AlphaSurface] ? TRUE : FALSE;
  inputshape   = (struct Region *)tv[LATAG_InputShape];

  if ((flags & LAYERSUPER) && (NULL == superbitmap))
    return NULL;

  /*
   * A composited layer renders into a surface of its own that the screen's
   * display composes; it needs a display that can do that and a
   * LayerInfo_extra to keep the screen's surface list in (Intuition
   * screens get theirs here). Otherwise it is an ordinary smart refresh
   * layer, as a composited one behaves like one.
   */
  if (composited)
  {
    if (NULL == li->LayerInfo_extra && !_AllocExtLayerInfo(li, LayersBase))
      composited = FALSE;
    else if (!CompositeLayersSupported(bm, &alphaformat))
      composited = FALSE;
  }
  if (composited)
  {
    flags = (flags & ~(LAYERSIMPLE | LAYERSUPER)) | LAYERSMART | LAYERCOMPOSITED;
    superbitmap = NULL;
  }
  else
    flags &= ~LAYERCOMPOSITED;

  if (!parent)
  {
    parent = li->check_lp;
    if (!parent)
    {
      /* Root layer not yet installed */
      
      if (priority != ROOTPRIORITY) /* avoid endless recursion */
      {
        /*
         * The root layer is what gets backfilled where no other layer
         * is. Its backfill hook is the Layer_Info's BlankHook, which
         * InstallLayerInfoHook() may already have set before any layer
         * existed (Intuition installs SA_BackFill right after InitLayers()).
         */
        struct TagItem tags[3];

        tags[0].ti_Tag  = LA_Hidden;
        tags[0].ti_Data = TRUE;
        tags[1].ti_Tag  = LA_BackfillHook;
        tags[1].ti_Data = (IPTR)li->BlankHook;
        tags[2].ti_Tag  = TAG_DONE;
        tags[2].ti_Data = 0;

        if (!(CreateLayerTagList(li,
                                 bm,
                                 0,
                                 0,
                                 GetBitMapAttr(bm, BMA_WIDTH) - 1,
                                 GetBitMapAttr(bm, BMA_HEIGHT) - 1,
                                 0,
                                 ROOTPRIORITY,
                                 tags,
                                 LayersBase)))
        {
          li->check_lp = NULL;
          return NULL;
        }
        
        parent = li->check_lp;
      }
    }
  }
  
  /*
   * User gives coordinates reltive to parent.
   * Adjust the shape to the absolute coordinates
   * If this is the root layer, I don't have to
   * do anything. I recognize the root layer if there
   * is no parent.
   */

  if (infrontof && infrontof->priority > priority)
    return NULL;
    
  if (behind && behind->priority < priority)
    return NULL;
  
  /* First create the shape region relative to nothing, that is 0,0 origin */

  shape = NewRectRegion(0, 0, x1 - x0, y1 - y0);
  if (!shape)
    return NULL;

  if (shapehook)
  {
        struct ShapeHookMsg msg;
        struct Rectangle NewBounds, OldBounds;

        msg.NewShape  = layershape;
        msg.OldShape  = layershape;
        msg.NewBounds = &NewBounds;
        msg.OldBounds = &OldBounds;

        NewBounds.MinX = x0;
        NewBounds.MinY = y0;
        NewBounds.MaxX = x1;
        NewBounds.MaxY = y1;
        OldBounds.MinX = 0;
        OldBounds.MinY = 0;
        OldBounds.MaxX = 0;
        OldBounds.MaxY = 0;

        if (CallHookPkt(shapehook, NULL, &msg))
            layershape = msg.NewShape;
  }

  if (layershape) if (!AndRegionRegion(layershape, shape))
  {
    DisposeRegion(shape);
    return NULL;
  }

  if (parent)
  {
    /* Convert from parent layer relative coords to screen relative coords */
    
    x0 += parent->bounds.MinX;
    y0 += parent->bounds.MinY;
    x1 += parent->bounds.MinX;
    y1 += parent->bounds.MinY;
  }
  
  /* Make shape region relative to screen */
  _TranslateRect(&shape->bounds, x0, y0);
  
  l  = AllocMem(sizeof(struct IntLayer), MEMF_CLEAR|MEMF_PUBLIC);
  if (l)
  {
    IL(l)->shapehook = shapehook;
    l->Window = win;
    l->shaperegion = layershape;
    l->rp = &IL(l)->rp;

    InitRastPort(l->rp);
    l->rp->Layer = l;
    l->rp->BitMap = bm;

    l->Flags = (WORD) flags;
    l->LayerInfo = li;
    
    l->SuperBitMap = superbitmap;
    l->BackFill = hook;
    l->priority = priority;
    
    InitSemaphore(&l->Lock);

    l->shape = shape;
    l->bounds.MinX = x0;
    l->bounds.MaxX = x1;
    l->bounds.MinY = y0;
    l->bounds.MaxY = y1;
    l->Width  = x1 - x0 + 1;
    l->Height = y1 - y0 + 1;
    IL(l)->opacity = 0xFF;
    IL(l)->alphaformat = alphaformat;
    if (inputshape)
    {
      /* A copy: the caller's region may go away with the call */
      if (NULL == (IL(l)->inputshape = NewRegion()) ||
          !OrRegionRegion(inputshape, IL(l)->inputshape))
        goto failexit;
    }
    if (IS_COMPOSITED(l))
    {
      ULONG bytes;

      /*
       * The surface: the screen's format or, for per pixel alpha, the
       * screen's pixel layout with an alpha byte (cleared, i.e. fully
       * transparent; pens and blits without alpha render opaque, see
       * <graphics/layersext.h>). A screen whose layout has no room for alpha
       * gets a constant opacity surface instead.
       *
       * Memory: the screen's composited surfaces together stay within
       * LAYERS_SURFACE_BUDGET_FACTOR screen bitmaps and leave
       * LAYERS_SURFACE_MINFREE of memory alone. A layer that does not fit,
       * or whose surface cannot be allocated, is an ordinary smart refresh
       * layer, exactly as if LA_Composited had not been given.
       */
      if (alphasurface && (ULONG)-1 == alphaformat)
        alphasurface = FALSE;

      bytes = _SurfaceBytes(bm, l->Width, l->Height, alphasurface, LayersBase);
      if (!_SurfaceBudgetAllows(li, bm, bytes, 0, LayersBase))
        l->Flags &= ~LAYERCOMPOSITED;
      else
      {
        if (alphasurface)
          IL(l)->surface = AllocBitMap(l->Width, l->Height, 32,
                                       BMF_SPECIALFMT | SHIFT_PIXFMT(alphaformat) | BMF_CLEAR, bm);
        else
          IL(l)->surface = AllocBitMap(l->Width, l->Height, bm->Depth, 0, bm);

        if (NULL != IL(l)->surface)
        {
          IL(l)->surfw = l->Width;
          IL(l)->surfh = l->Height;
          IL(l)->opacity = opacity;
          IL(l)->alphasurface = alphasurface;
          _AccountSurface(l, bytes, LayersBase);
          if (!LAYERS_COMPOSITED_OCCLUDES || 0xFF != opacity || alphasurface)
            IL(l)->intflags |= INTFLAG_TRANSLUCENT;
        }
        else
          l->Flags &= ~LAYERCOMPOSITED;
      }
    }
#if LAYERS_BACKING_SURFACE
    if (!IS_COMPOSITED(l) && IS_SMARTREFRESH(l) && NULL == IL(l)->surface)
    {
      /* no surface (out of memory) means per ClipRect bitmaps as before */
      IL(l)->surface = AllocBitMap(l->Width, l->Height, bm->Depth, 0, bm);
      IL(l)->surfw = l->Width;
      IL(l)->surfh = l->Height;
    }
#endif
    LockLayer(0, l);
    
    if (NULL == (l->DamageList = NewRegion()))
      goto failexit;
    if (NULL == (l->VisibleRegion = NewRegion()))
      goto failexit;
    if (NULL == (l->visibleshape = NewRegion()))
      goto failexit;
    /* what the layer hides from those behind: its visible shape, or nothing when blended */
    if (IS_TRANSLUCENT(l))
    {
      if (NULL == (IL(l)->occludeshape = NewRegion()))
        goto failexit;
    }
    else
      IL(l)->occludeshape = l->visibleshape;
    
    l->visible = visible;

    if (NULL == li->check_lp)
      li->check_lp = l;

    l->parent = parent;

    if (parent)
      l->nesting = parent->nesting+1;
    else
      l->nesting = 0;

    LockLayers(li);

    
    /*
     * If neither a layer in front or behind is
     * given the search for the place according to
     * the priority and insert it there. I
     * determine behind or infrontof here!
     */
    if (NULL != li->top_layer)
    {
      if (!infrontof && !behind)
      {
        int end = FALSE;
        struct Layer * lastgood;
        infrontof = parent;
 
        do
        {
          lastgood = infrontof;
          infrontof = infrontof->front;
          if (NULL == infrontof)
          {
            infrontof = lastgood;
            break;
          }
          if (infrontof->parent != l->parent)
          {
            infrontof = lastgood;
            break;
          }
          while (infrontof->nesting >= l->nesting)
          {
            lastgood = infrontof;
            infrontof = infrontof->front;
            if (NULL == infrontof ||
                infrontof->priority > l->priority)
            {
              infrontof = lastgood;
              end = TRUE;
              break;
            }
          }
          
          
        }
        while (FALSE == end);
      
      }
    }

    if (infrontof || (NULL == behind))
    {
      if (NULL == infrontof)
        infrontof = li->top_layer;

      if (li->top_layer == infrontof)
      {
        li->top_layer = l;
        l->front  = NULL;
        l->back   = infrontof;
        if (NULL != infrontof)
          infrontof->front = l;
      }
      else
      {
        l->front = infrontof->front;
        l->back  = infrontof;
        infrontof->front->back = l;
        infrontof->front = l;
      }
    }
    else if (behind)
    {
      l->front = behind;
      l->back  = behind->back;
      if (l->back)
        l->back->front = l;
      behind->back = l;
    }

    
    
    SetRegion(l->shape, l->visibleshape);
    if (l->parent)
      AndRegionRegion(l->parent->shape, l->visibleshape);


    if (IS_VISIBLE(l))
    {
      /*
       * layer is to be visible
       */

      /*
       * Does this layer have a layer in front of it?
       * If yes, then take that layer's VisibleRegion and
       * cut out that layer's shape. This is then the
       * VisibleRegion of my layer.
       */
      if (l->front)
      {
        SetRegion(l->front->VisibleRegion, l->VisibleRegion);
        ClearRegionRegion(OCCLUDESHAPE(l->front), l->VisibleRegion);
      }
      else
        SetRegion(li->check_lp->shape, l->VisibleRegion);
     
     
      /*
       * First tell all layers behind this layer to
       * back up their parts that the new layer will
       * be hiding.
       */
      _HideLayersBehind(l, OCCLUDESHAPE(l), &l->shape->bounds, LayersBase);

    }
    /*
     * Show the layer according to its visible area
     * This function creates the cliprects in the area
     * of the layer.
     * This also works for invisible layers since their Visible
     * Region is non existent.
     */
    if (!IS_ROOTLAYER(l))
      _ShowLayer(l, LayersBase);

    if (IS_COMPOSITED(l))
    {
      LIE(li)->lie_NumComposited++;
      LIE(li)->lie_ScreenBM = bm;
    }
    UPDATE_SURFACES(li);
  }
  else
    goto failexit;

  UnlockLayers(li);
  
  return l;
  
failexit:
  if (l)
  {
    if (IL(l)->inputshape)
      DisposeRegion(IL(l)->inputshape);
    if (IL(l)->surface)
    {
      _AccountSurface(l, 0, LayersBase);
      FreeBitMap(IL(l)->surface);
    }
    if (IL(l)->occludeshape && IL(l)->occludeshape != l->visibleshape)
      DisposeRegion(IL(l)->occludeshape);
    if (l->VisibleRegion)
      DisposeRegion(l->VisibleRegion);
    if (l->DamageList)
      DisposeRegion(l->DamageList);
    if (l->visibleshape)
      DisposeRegion(l->visibleshape);
    FreeMem(l, sizeof(struct IntLayer));
  }

  return NULL;
} /* CreateBehindHookLayer */
