/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
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

#include "basicfuncs.h"
#include "layers_intern.h"

/*
 * LA_InFrontOf and LA_Behind are intentionally left
 * because they allow to implement AmigaOS4-compatible functionality.
 */
struct Layer *CreateLayerTagList(struct Layer_Info *li, struct BitMap *bm, LONG x0, LONG y0, 
				 LONG x1, LONG y1, LONG flags, int priority, struct TagItem *tagList,
				 struct LayersBase *LayersBase)
{
  struct BitMap * superbitmap = NULL;
  struct Hook * hook = NULL, *shapehook = NULL;
  APTR win = 0;
  int visible = TRUE;
  struct Layer * behind = NULL, * infrontof = NULL, * parent = NULL; 
  struct Layer * l;
  struct Region * layershape = NULL, *shape;
  struct TagItem *tag, *tstate = tagList;
  
  while((tag = NextTagItem(&tstate)))
  {
    switch (tag->ti_Tag)
    { 
    case LA_BackfillHook:
        hook = (struct Hook *)tag->ti_Data;
        break;

    case LA_SuperBitMap:
        superbitmap = (struct BitMap *)tag->ti_Data;
        break;

    case LA_ChildOf:
        parent = (struct Layer *)tag->ti_Data;
        break;

    case LA_InFrontOf:
        infrontof = (struct Layer *)tag->ti_Data;
        break;

    case LA_Behind:
        behind = (struct Layer *)tag->ti_Data;
        break;

    case LA_Hidden:
        visible = !tag->ti_Data;
        break;

    case LA_ShapeRegion:
        layershape = (struct Region *)tag->ti_Data;
        break;

    case LA_ShapeHook:
	shapehook = (struct Hook *)tag->ti_Data;
	break;

    case LA_WindowPtr:
      	win = (APTR)tag->ti_Data;
      	break;

    }

  } /* while((tag = NextTagItem(&tstate))) */
  
  if ((flags & LAYERSUPER) && (NULL == superbitmap)) 
    return NULL;

  if (!parent)
  {
    parent = li->check_lp;
    if (!parent)
    {
      /* Root layer not yet installed */
      
      if (priority != ROOTPRIORITY) /* avoid endless recursion */
      {
        struct TagItem tags[] =
	{
	    {LA_Hidden, TRUE     	},
	    {TAG_DONE	    	    	}
	};

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

    LockLayer(0, l);
    
    if (NULL == (l->DamageList = NewRegion()))
      goto failexit;
    if (NULL == (l->VisibleRegion = NewRegion()))
      goto failexit; 
    if (NULL == (l->visibleshape = NewRegion()))
      goto failexit;
    
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
      struct Layer * _l = l->back;
      struct Layer * lparent = l->parent;

      /*
       * Does this layer have a layer in front of it?
       * If yes, then take that layer's VisibleRegion and
       * cut out that layer's shape. This is then the 
       * VisibleRegion of my layer.
       */
      if (l->front)
      {
        SetRegion(l->front->VisibleRegion, l->VisibleRegion);
        ClearRegionRegion(l->front->visibleshape, l->VisibleRegion);
      }
      else
        SetRegion(li->check_lp->shape, l->VisibleRegion);
     
     
      /*
       * First tell all layers behind this layer to
       * back up their parts that the new layer will
       * be hiding.
       */
      while (1)
      {
        if (IS_VISIBLE(_l) && DO_OVERLAP(&l->shape->bounds, &_l->shape->bounds))
          _BackupPartsOfLayer(_l, l->visibleshape, 0, FALSE, LayersBase);
        else
          ClearRegionRegion(l->visibleshape, _l->VisibleRegion);
        
        if (_l == lparent)
        {
          if (IS_VISIBLE(_l) || (NULL == lparent->parent))
            break;
          else
            lparent = lparent->parent;
        }
        _l = _l->back;
      }

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
  }
  else
    goto failexit;

  UnlockLayers(li);
  
  return l;
  
failexit:
  if (l)
  {
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
