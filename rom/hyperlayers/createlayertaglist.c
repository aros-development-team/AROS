/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <utility/tagitem.h>
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

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  int i = 0;
  struct BitMap * superbitmap = NULL;
  struct Hook * hook = NULL;
  int priority = UPFRONTPRIORITY;
  int visible = TRUE;
  struct Layer * behind = NULL, * infrontof = NULL, * parent = NULL; 
  struct Layer * l;
  struct RastPort * rp;
  struct Region * layershape = NULL, *shape;

  while (TAG_DONE != tagList[i].ti_Tag)
  {
    switch (tagList[i].ti_Tag)
    {
      case LA_Priority:
        priority = tagList[i].ti_Data;
      break;
      
      case LA_Hook:
        hook = (struct Hook *)tagList[i].ti_Data;
      break;
      
      case LA_SuperBitMap:
        superbitmap = (struct BitMap *)tagList[i].ti_Data;
      break;
      
      case LA_ChildOf:
        parent = (struct Layer *)tagList[i].ti_Data;
      break;
      
      case LA_InFrontOf:
        if (infrontof)
          return NULL;
        infrontof = (struct Layer *)tagList[i].ti_Data;
      break;
      
      case LA_Behind:
        if (behind)
          return NULL;
        behind = (struct Layer *)tagList[i].ti_Data;
      break;
      
      case LA_Visible:
        visible = tagList[i].ti_Data;
      break;
      
      case LA_Shape:
        layershape = (struct Region *)tagList[i].ti_Data;
      break;

      default:
        kprintf("%s: Unknown option %d!\n",__FUNCTION__,tagList[i].ti_Tag);
        return NULL;
    }
    i++;
  }
  
  if ((flags & LAYERSUPER) && (NULL == superbitmap)) 
    return NULL;
    
  if (!parent)
    parent = li->check_lp;

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
  
  l = AllocMem(sizeof(struct Layer), MEMF_CLEAR|MEMF_PUBLIC);
  rp = CreateRastPort();
  

  if (l && rp)
  {
    l->shape = shape;
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
    l->visible = visible;

    if (NULL == li->check_lp)
      li->check_lp = l;

    l->parent = parent;

    LockLayers(li);
    
    /*
     * If neither a layer in front or behind is
     * given the search for the place according to
     * the priority and insert it there. I
     * determine behind or infrontof here!
     */
    if (!infrontof && !behind)
    {
      infrontof = li->top_layer;
      while (infrontof && infrontof->priority > priority)
      {
        if (NULL == infrontof->back)
        {
          behind = infrontof;
          infrontof = NULL;
          break;
        }
        infrontof = infrontof->back;
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

    if (parent)
      l->nesting = parent->nesting+1;
    else
      l->nesting = 0;
    
    


    if (IS_VISIBLE(l))
    {
      /*
       * layer is to be visible
       */
      struct Layer * _l = l->back;
      struct Layer * lparent = l->parent;
      struct Region rtmp;
      rtmp.RegionRectangle = NULL;

      /*
       * Does this layer have a layer in front of it?
       * If yes, then take that layer's VisibleRegion and
       * cut out that layer's shape. This is then the 
       * VisibleRegion of my layer.
       */
      if (l->front)
      {
        _SetRegion(l->front->VisibleRegion, l->VisibleRegion);
        _SetRegion(l->front->shape, &rtmp);
        AndRegionRegion(l->front->parent->shape, &rtmp);
        ClearRegionRegion(&rtmp, l->VisibleRegion);
      }
      else
        _SetRegion(li->check_lp->shape, l->VisibleRegion);
     
      _SetRegion(l->shape, &rtmp);
      AndRegionRegion(l->parent->shape, &rtmp);
     
      /*
       * First tell all layers behind this layer to
       * back up their parts that the new layer will
       * be hiding.
       */
      while (1)
      {
        if (IS_VISIBLE(_l) && DO_OVERLAP(&l->shape->bounds, &_l->shape->bounds))
          _BackupPartsOfLayer(_l, l->shape, 0, FALSE, LayersBase);
        else
          ClearRegionRegion(&rtmp, _l->VisibleRegion);
        
        if (_l == lparent)
        {
          if (IS_VISIBLE(_l) || (NULL == lparent->parent))
            break;
          else
            lparent = lparent->parent;
        }
        _l = _l->back;
      }

      ClearRegion(&rtmp);
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
    FreeMem(l, sizeof(struct Layer));
  }

  if (rp)
    FreeRastPort(rp);

  return NULL;

  AROS_LIBFUNC_EXIT
} /* CreateBehindHookLayer */
