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
	AROS_LH4(struct Layer *, CreateLayerTagList,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(struct BitMap     *, bm, A1),
	AROS_LHA(LONG               , flags, D0),
	AROS_LHA(struct TagItem    *, tagItem, A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 37, Layers)

/*  FUNCTION
        Create a new layer at the given position and with the
        given size. 

    INPUTS
        li    - pointer to LayerInfo structure
        bm    - pointer to common bitmap
        flags - choose the type of layer by setting some flags
        
    RESULT
        Pointer to the newly created layer. NULL if layer could not be 
        created (Probably out of memory).
        
    NOTES
       You MUST provide LA_PRIORITY!


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
  struct Region * layershape;

kprintf("%s entered!\n",__FUNCTION__);

  while (TAG_DONE != tagItem[i].ti_Tag)
  {
    switch (tagItem[i].ti_Tag)
    {
      case LA_PRIORITY:
        priority = tagItem[i].ti_Data;
      break;
      
      case LA_HOOK:
        hook = (struct Hook *)tagItem[i].ti_Data;
      break;
      
      case LA_SUPERBITMAP:
        superbitmap = (struct BitMap *)tagItem[i].ti_Data;
      break;
      
      case LA_CHILDOF:
        parent = (struct Layer *)tagItem[i].ti_Data;
      break;
      
      case LA_INFRONTOF:
        if (infrontof)
          return NULL;
        infrontof = (struct Layer *)tagItem[i].ti_Data;
      break;
      
      case LA_BEHIND:
        if (behind)
          return NULL;
        behind = (struct Layer *)tagItem[i].ti_Data;
      break;
      
      case LA_VISIBLE:
        visible = tagItem[i].ti_Data;
      break;
      
      case LA_SHAPE:
        layershape = (struct Region *)tagItem[i].ti_Data;
      break;

      default:
        kprintf("%s: Unknown option %d!\n",__FUNCTION__,tagItem[i].ti_Tag);
        return NULL;
    }
    i++;
  }
  
  if ((flags & LAYERSUPER) && (NULL == superbitmap)) 
  {
//    kprintf("%s: LAYERSUPER but not bitmap!\n",__FUNCTION__);
    return NULL;
  }
    
  if (!layershape)
  {
//    kprintf("No layer shape!\n");
    return NULL;
  }

  if (infrontof && infrontof->priority > priority)
    return NULL;
    
  if (behind && behind->priority < priority)
    return NULL;

  l = AllocMem(sizeof(struct Layer), MEMF_CLEAR|MEMF_PUBLIC);
  rp = CreateRastPort();

  if (l && rp)
  {
    l->shape = layershape;
    l->rp = rp;
    
    rp->Layer = l;
    rp->BitMap = bm;

    l->bounds.MinX = layershape->bounds.MinX;
    l->bounds.MaxX = layershape->bounds.MaxX;
    l->bounds.MinY = layershape->bounds.MinY;
    l->bounds.MaxY = layershape->bounds.MaxY;
    
    l->Flags = (WORD) flags;
    l->LayerInfo = li;
    l->Width  = layershape->bounds.MaxX - layershape->bounds.MinX + 1;
    l->Height = layershape->bounds.MaxY - layershape->bounds.MinY + 1;
    
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

    if (!parent)
      parent = li->check_lp;
    
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
//kprintf("Creating a layer on top!\n");
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
    
    
    /*
     * Does this layer have a layer in front of it?
     * If yes, then take that layer's VisibleRegion and
     * cut out that layer's shape. This is then the 
     * VisibleRegion of my layer.
     */
    if (l->front)
    {
#warning Write a function to duplicate a region.
         OrRegionRegion(l->front->VisibleRegion, l->VisibleRegion);
      ClearRegionRegion(l->front->shape, l->VisibleRegion);
    }
    else
      OrRegionRegion(li->check_lp->shape, l->VisibleRegion);

    if (IS_VISIBLE(l))
    {
      /*
       * layer is to be visible
       */
      struct Layer * _l = l->back;

      /*
       * First tell all layers behind this layer to
       * back up their parts that the new layer will
       * be hiding.
       */
      while (NULL != _l && _l->parent == parent)
      {
        if (IS_VISIBLE(_l))
        {
//kprintf("\t\tbacking up parts of layer %p!\n",_l);
          _BackupPartsOfLayer(_l, l->shape, 0, FALSE);
        }
        _l = _l->back;
      }

      /*
       * Show the layer according to its visible area
       * This function creates the cliprects in the area
       * of the layer.
       */
      _ShowLayer(l);
    }
  }
  else
    goto failexit;

//  kprintf("Leaving %s l=%p\n",__FUNCTION__,l);
  
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
  {
    FreeRastPort(rp);
  }

//  kprintf("Leaving %s - faiure!\n",__FUNCTION__);
  
  return NULL;


  AROS_LIBFUNC_EXIT
} /* CreateBehindHookLayer */
