/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include "basicfuncs.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH8(struct Layer *, CreateLayerTagList,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(struct BitMap     *, bm, A1),
        AROS_LHA(LONG               , x0, D0),
        AROS_LHA(LONG               , y0, D1),
        AROS_LHA(LONG               , x1, D2),
        AROS_LHA(LONG               , y1, D3),
	AROS_LHA(LONG               , flags, D4),
	AROS_LHA(struct TagItem    *, tagList, A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 37, Layers)

/*  FUNCTION
        Create a new layer according to the tags given.

    INPUTS
        li    - pointer to LayerInfo structure
        bm    - pointer to common bitmap
	x0,y0 - upper left corner of the layer (in parent layer coords)
	x1,y1 - lower right corner of the layer (in parent layer coords)
        flags - choose the type of layer by setting some flags
                If it is to be a super bitmap layer then the tag
                LA_SUPERBITMAP must be provided along with a 
                pointer to a valid super bitmap.
        tagList - a list of tags that specify the properties of the
                  layer. The following tags are currently supported:
                  LA_PRIORITY : priority class of the layer. The
                                higher the number the further the
                                layer will be in front of everything
                                else.
                                Default value is UPFRONTPRIORITY.
                  LA_HOOK     : Backfill hook
                  LA_SUPERBITMAP : pointer to a superbitmap. The flags
                                  must also represent that this
                                  layer is supposed to be a superbitmap
                                  layer.
                  LA_CHILDOF  : pointer to parent layer. If NULL then
                                this layer will be created as a old-style
                                layer.
                  LA_INFRONTOF : pointer to a layer in front of which
                                 this layer is to be created.
                  LA_BEHIND : pointer to a layer behind which this layer
                             is to be created. Must not give both LA_INFRONTOF
                             and LA_BEHIND.
                  LA_VISIBLE : FALSE if this layer is to be invisible.
                               Default value is TRUE
                  LA_SHAPE : The region of the layer that comprises its shape.
                             This value is optional. The region must be relative to the layer.
                  
        
    RESULT
        Pointer to the newly created layer. NULL if layer could not be 
        created (Probably out of memory).
        If the layer is created successful you must not free its shape.
        The shape is automatically freed when the layer is deleted.
        
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  struct BitMap * superbitmap = NULL;
  struct Hook * hook = NULL, *shapehook = NULL;
  int priority = UPFRONTPRIORITY;
  int visible = TRUE;
  struct Layer * behind = NULL, * infrontof = NULL, * parent = NULL; 
  struct Layer * l;
  struct RastPort * rp;
  struct Region * layershape = NULL, *shape;
  const struct TagItem *tstate = tagList;
  struct TagItem *tag;
  
  while((tag = NextTagItem(&tstate)))
  {
    switch (tag->ti_Tag)
    {
      case LA_Priority:
        priority = tag->ti_Data;
      break;
      
      case LA_Hook:
        hook = (struct Hook *)tag->ti_Data;
      break;
      
      case LA_SuperBitMap:
        superbitmap = (struct BitMap *)tag->ti_Data;
      break;
      
      case LA_ChildOf:
        parent = (struct Layer *)tag->ti_Data;
      break;
      
      case LA_InFrontOf:
        if (infrontof)
          return NULL;
        infrontof = (struct Layer *)tag->ti_Data;
      break;
      
      case LA_Behind:
        if (behind)
          return NULL;
        behind = (struct Layer *)tag->ti_Data;
      break;
      
      case LA_Visible:
        visible = tag->ti_Data;
      break;
      
      case LA_Shape:
        layershape = (struct Region *)tag->ti_Data;
      break;
      
      case LA_ShapeHook:
      	shapehook = (struct Hook *)tag->ti_Data;
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
      
      if (!(flags & LAYER_ROOT_LAYER)) /* avoid endless recursion */
      {
        struct TagItem tags[] =
	{
	    {LA_Visible , FALSE     	},
	    {LA_Priority, ROOTPRIORITY	},
	    {TAG_DONE	    	    	}
	};
	
        if (!(CreateLayerTagList(li,
	    	    	    	 bm,
				 0,
				 0,
				 GetBitMapAttr(bm, BMA_WIDTH) - 1,
				 GetBitMapAttr(bm, BMA_HEIGHT) - 1,
				 LAYER_ROOT_LAYER,
				 tags)))
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
    
    msg.Action = SHAPEHOOKACTION_CREATELAYER;
    msg.Layer  = 0;
    msg.ActualShape = layershape;
    msg.NewBounds.MinX = x0;
    msg.NewBounds.MinY = y0;
    msg.NewBounds.MaxX = x1;
    msg.NewBounds.MaxY = y1;
    msg.OldBounds.MinX = 0;
    msg.OldBounds.MinY = 0;
    msg.OldBounds.MaxX = 0;
    msg.OldBounds.MaxY = 0;
    
    layershape = (struct Region *)CallHookPkt(shapehook, NULL, &msg);
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
  
  l = AllocMem(sizeof(struct IntLayer), MEMF_CLEAR|MEMF_PUBLIC);
  rp = CreateRastPort();
  

  if (l && rp)
  {
    l->shape = shape;
    IL(l)->shapehook = shapehook;
    l->shaperegion = layershape;
    l->rp = rp;
    
    rp->Layer = l;
    rp->BitMap = bm;

    l->bounds.MinX = x0;
    l->bounds.MaxX = x1;
    l->bounds.MinY = y0;
    l->bounds.MaxY = y1;
    
    l->Flags = (WORD) flags;
    l->LayerInfo = li;
    l->Width  = x1 - x0 + 1;
    l->Height = y1 - y0 + 1;
    
    l->SuperBitMap = superbitmap;
    l->BackFill = hook;
    l->priority = priority;
    
    InitSemaphore(&l->Lock);
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

  if (rp)
    FreeRastPort(rp);

  return NULL;

  AROS_LIBFUNC_EXIT
} /* CreateBehindHookLayer */
