/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include "layers_intern.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>

	AROS_LH2(LONG, MoveLayerInFrontOf,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, layer_to_move, A0),
	AROS_LHA(struct Layer *, other_layer, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 28, Layers)

/*  FUNCTION
        Moves layer directly in front of another layer. Other layers
        might become visible. You cannot move a backdrop layer in front
        of a non-backdrop layer.
        If parts of a simple layer become visible these areas are added
        to the damage list.

    INPUTS
        layer_to_move - pointer to layer that is to be moved
        other_layer   - pointer to other layer that will be behind the
                        layer_to_move.

    RESULT
        TRUE  - layer was moved
        FALSE - layer could not be moved. (probably out of memory)

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

  struct ClipRect * CR;
  struct Layer_Info * LI = layer_to_move->LayerInfo;
  struct Region * oldclipregion;

  D(bug("MoveLayerInFrontOf(layer_to_move @ $%lx, other_layer @ $%lx)\n", layer_to_move, other_layer));

  /* Do a few checks whether I will have to do all this here at all... */
  
  /* do they have the same root ? */
  if (LI != other_layer->LayerInfo)
    return FALSE;

  /* if other_layer is top layer then use UpfrontLayer() */
  if (other_layer == LI->top_layer )  
   return UpfrontLayer(0, layer_to_move);

  /* won't move a BACKDROPLAYER infront of a non-BACKDROPLAYER and
     won't move a non-BACKDROPLAYER behind a BACKDROP layer*/
  if ((layer_to_move->Flags & LAYERBACKDROP) != 0 &&
      (other_layer  ->Flags & LAYERBACKDROP) == 0)
    return FALSE;

  if ( other_layer->front != NULL &&
      (other_layer->front->Flags & LAYERBACKDROP) != 0 &&
      (layer_to_move     ->Flags & LAYERBACKDROP) == 0 )
    return FALSE;

  /* if both layers are the same */
  if (other_layer == layer_to_move)
    return FALSE;

  /* if I am not moving it at all */
  if (other_layer == layer_to_move->back)
   return TRUE;

  /* I guess we can actually move this layer_to_move layer */
  LockLayers(LI);

  /* 
     If the top layer is to be moved then I first have to split it. 
   */

  SetLayerPriorities(LI);

  oldclipregion = InstallClipRegion(layer_to_move, NULL);

  UninstallClipRegionClipRects(LI);

  if (layer_to_move == LI->top_layer)
    CreateClipRectsTop(LI, FALSE);

  /* 
     Unlink the layer from its old position and link it into the
     new position.
   */
  if (NULL != layer_to_move -> back)
    layer_to_move -> back -> front = layer_to_move -> front;

  if (NULL != layer_to_move -> front)
    layer_to_move -> front -> back = layer_to_move -> back;
  else
    LI -> top_layer = layer_to_move -> back;

  /* Linking into new position */
  if (NULL != other_layer -> front)
    other_layer -> front -> back = layer_to_move;
  else
    LI->top_layer = layer_to_move;

  layer_to_move -> back  = other_layer;
  layer_to_move -> front = other_layer -> front;

  other_layer -> front = layer_to_move;

  CR = layer_to_move -> ClipRect;

  /*
     Comment: This function depends very much on the consistency 
              of the cliprects.
   */

  /* check whether we're moving further to the front or to the back */
  if (other_layer->priority > layer_to_move->priority)
  {
    /* 
       The layer_to_move goes further in the front, which
       means that visible parts stay visible and hidden
       parts might become visible... 
       I have to visit all the ClipRects of the layer_to_move.
     */

     while (NULL != CR)
     {
       /* Has this ClipRect been hidden? */
       if (NULL != CR->lobs)
       {
         /* 
            It was hidden. Should it be visible now? HAVE to use 
            other_layer for priority comparison. 
          */
         if (CR->lobs->priority <= other_layer->priority)
	 {
           /* 
              I have to make this ClipRect CR visible now.
           */
           struct Layer * L_tmp;
           struct ClipRect * CR_tmp;

           L_tmp = CR->lobs;
           /* 
              L_tmp is a pointer to the layer that was visible before
              in that area.
	    */
	   /*
              Now I have to look for the ClipRects in L_tmp that
              must now be hidden. There might be more than just
              one ClipRect that needs to be hidden.
	    */

           CR_tmp = L_tmp->ClipRect;
           while (NULL != CR_tmp)
           {
             if (NULL == CR_tmp->lobs &&
                 !(CR_tmp->bounds.MinX > CR->bounds.MaxX ||
                   CR_tmp->bounds.MinY > CR->bounds.MaxY ||
                   CR_tmp->bounds.MaxX < CR->bounds.MinX ||
                   CR_tmp->bounds.MaxY < CR->bounds.MinY))
             {
               /*   
                  CR_tmp needs to get the content of what's visible in
                  the area now. It will be hidden. 
                */
               if (0 == (L_tmp->Flags & LAYERSIMPLE))
               {
                 if (0 == (L_tmp->Flags & LAYERSUPER))
                 {
                   /* it's a smart layer */
                   /* I need to allocate a bitmap to backup the data */
                   CR_tmp->BitMap = 
                     AllocBitMap(CR_tmp->bounds.MaxX - CR_tmp->bounds.MinX + 1,
                                 CR_tmp->bounds.MaxY - CR_tmp->bounds.MinY + 1,
                                 GetBitMapAttr(L_tmp->rp->BitMap, BMA_DEPTH),
                                 0,
                                 L_tmp->rp->BitMap);
               
                   BltBitMap(L_tmp->rp->BitMap,
                             CR_tmp->bounds.MinX,
                             CR_tmp->bounds.MinY,
                             CR_tmp->BitMap,
                             ALIGN_OFFSET(CR_tmp->bounds.MinX),
                             0,
                             CR_tmp->bounds.MaxX - CR_tmp->bounds.MinX + 1,
                             CR_tmp->bounds.MaxY - CR_tmp->bounds.MinY + 1,
                             0x0c0,
                             0xff,
                             NULL);
                 }
                 else
                 {
                   /* it's a superbitmap layer */
                   CR_tmp->BitMap = L_tmp->SuperBitMap;
                   BltBitMap(L_tmp->rp->BitMap,
                             CR_tmp->bounds.MinX,
                             CR_tmp->bounds.MinY,
                             CR_tmp->BitMap,
                             CR_tmp->bounds.MinX - L_tmp->bounds.MinX + L_tmp->Scroll_X,
                             CR_tmp->bounds.MinY - L_tmp->bounds.MinY + L_tmp->Scroll_Y,
                             CR_tmp->bounds.MaxX - CR_tmp->bounds.MinX + 1,
                             CR_tmp->bounds.MaxY - CR_tmp->bounds.MinY + 1,
                             0x0c0,
                             0xff,
                             NULL);
                 }
               }
               else
               {
                 /*
                 ** Take the area of that ClipRect out of the DamageList
                 ** such that no mess happens on the screen.
                 */
                 struct Rectangle R = CR_tmp->bounds;
                 CR_tmp->bounds.MinX -= L_tmp->bounds.MinX;
                 CR_tmp->bounds.MinY -= L_tmp->bounds.MinY;
                 CR_tmp->bounds.MaxX -= L_tmp->bounds.MinX;
                 CR_tmp->bounds.MaxY -= L_tmp->bounds.MinY;
                 ClearRectRegion(L_tmp->DamageList, &R);
                 
               }
               /*
               ** Mark this ClipRect as hidden.
               */
               CR_tmp->lobs = layer_to_move;
             }
             
             CR_tmp = CR_tmp->Next;
           } /* while */
           
           /* Whatever can be found in CR will be shown now. */

           if (0 == (layer_to_move->Flags & LAYERSIMPLE))
           {
             if (0 == (layer_to_move->Flags & LAYERSUPER))
             {
               /* it's a smart layer */
               BltBitMap(CR->BitMap,
                         ALIGN_OFFSET(CR->bounds.MinX),
                         0,
                         layer_to_move->rp->BitMap,
                         CR->bounds.MinX,
                         CR->bounds.MinY,
                         CR->bounds.MaxX - CR->bounds.MinX + 1,
                         CR->bounds.MaxY - CR->bounds.MinY + 1,
                         0x0c0,
                         0xff,
                         NULL);
               FreeBitMap(CR->BitMap);
             }
             else
             {
               /* it's a superbitmap layer */
               BltBitMap(CR->BitMap,
                         CR->bounds.MinX - layer_to_move->bounds.MinX +
                                           layer_to_move->Scroll_X,
                         CR->bounds.MinY - layer_to_move->bounds.MinY +
                                           layer_to_move->Scroll_Y,
                         layer_to_move->rp->BitMap,
                         CR->bounds.MinX,
                         CR->bounds.MinY,
                         CR->bounds.MaxX - CR->bounds.MinX + 1,
                         CR->bounds.MaxY - CR->bounds.MinY + 1,
                         0x0c0,
                         0xff,
                         NULL);
             }
           }
           else
           {
             /* it's a simple layer*/
             struct Rectangle Rect = CR->bounds;
             Rect.MinX -= layer_to_move->bounds.MinX; 
             Rect.MinY -= layer_to_move->bounds.MinY; 
             Rect.MaxX -= layer_to_move->bounds.MinX; 
             Rect.MaxY -= layer_to_move->bounds.MinY; 
             
             OrRectRegion(layer_to_move->DamageList, &Rect);
             layer_to_move->Flags |= LAYERREFRESH;
             
             _CallLayerHook(layer_to_move->BackFill,
                            layer_to_move->rp,
                            layer_to_move,
                            &CR->bounds,
                            CR->bounds.MinX,
                            CR->bounds.MinY);
           }

           CR    ->BitMap = NULL; 
           CR    ->lobs   = NULL;

           /* 
              Now I have to change all the lobs entries for this area
              in the layers that are further behind. What a pain.
            */
           while (NULL != L_tmp->back)
	   {
             L_tmp = internal_WhichLayer(L_tmp->back,
                                         CR->bounds.MinX,
                                         CR->bounds.MinY);
             if (NULL == L_tmp)
               break;
             CR_tmp = L_tmp->ClipRect;
             while (NULL != CR_tmp)
             {
               if (!(CR_tmp->bounds.MinX > CR->bounds.MaxX ||
                     CR_tmp->bounds.MinY > CR->bounds.MaxY || 
                     CR_tmp->bounds.MaxX < CR->bounds.MinX ||
                     CR_tmp->bounds.MaxY < CR->bounds.MinY))
                 CR_tmp -> lobs = layer_to_move;
               CR_tmp = CR_tmp->Next;
             }
	   } /* while */
	 } /* if (...priority > .. priority) */
       } /* if */
       CR = CR -> Next;
     }
  }
  else
  {
    while (NULL != CR)
    {
      /* Has this ClipRect been visible before? */
      if (NULL == CR->lobs)
      {
        /*
           It was visible. Should it be hidden now?
         */
        ULONG area_vis, area_hid;
        struct Layer * _L;
        struct ClipRect * _CR;
        
        _L = WhichLayer(LI, CR->bounds.MinX , CR->bounds.MinY);
        
        if (_L != layer_to_move )
	{
	  struct Layer * L_tmp = _L;
	  /*
	  ** The size of the visible ClipRect
	  */
	  area_vis = (CR->bounds.MaxX - CR->bounds.MinX + 1) *
	             (CR->bounds.MaxY - CR->bounds.MinY + 1);
	  /*
	  ** Search for the first ClipRect in the Layer _L that
	  ** somehow overlaps with this cliprect CR. That
	  ** ClipRect and possibly some more have to be made
	  ** visible.
	  */
	  area_hid = 0;
	  
	  _CR = _L->ClipRect;
	  while (NULL != _CR)
	  {
	    if (layer_to_move == _CR->lobs &&
	        !(_CR->bounds.MinX > CR->bounds.MaxX ||
	          _CR->bounds.MinY > CR->bounds.MaxY ||
	          _CR->bounds.MaxX < CR->bounds.MinX ||
	          _CR->bounds.MaxY < CR->bounds.MinY))
	    {
	      area_hid = (_CR->bounds.MaxX - _CR->bounds.MinX + 1) *
	                 (_CR->bounds.MaxY - _CR->bounds.MinY + 1);
	      break;
	    }
	    _CR = _CR->Next;
	  } 
	  
	  /*
	  ** It is possible that multiple of the layer L's ClipRect
	  ** have to become hidden to male the ClipRect of layer _L
	  ** visible. CR is one of them.
	  ** But it is possible that multiple of the layer _L's
	  ** ClipRects have to become visible to make the ClipRect 
	  ** of layer L visible. _CR is one of them
	  */
	  
	  if (area_vis >= area_hid)
	  {
	    /*
	    ** More than one hidden ClipRect must be made visible
	    */
	    /*
	    ** ClipRect(s) [_L} is/are now visible and CR [layer_to_move]
	    ** is now hidden.
	    */
	    
	    /*
	    ** There are several cases now:
	    ** The CR  to be hidden can be a simple layer,
	    **                               superbitmap layer
	    **                               smart layer
	    ** The _CR(s) to become visible can be a simple layer(s),
	    **                                       superbitmap layer(s) or
	    **                                       smart layer(s)
	    */
	    
	    if (0 == (layer_to_move->Flags & LAYERSIMPLE))
	    {
	      /* the part to be hidden has to go into a bitmap */
	      if (0 == (layer_to_move->Flags & LAYERSUPER))
	      {
	        CR->BitMap =
	            AllocBitMap(CR->bounds.MaxX - CR->bounds.MinX + 1,
	                        CR->bounds.MaxY - CR->bounds.MinY + 1,
	                        layer_to_move->rp->BitMap->Depth,
	                        0,
	                        layer_to_move->rp->BitMap);
	                        
	        BltBitMap(layer_to_move->rp->BitMap,
	                  CR->bounds.MinX,
	                  CR->bounds.MinY,
	                  CR->BitMap,
	                  ALIGN_OFFSET(CR->bounds.MinX),
	                  0,
	                  CR->bounds.MaxX - CR->bounds.MinX + 1,
	                  CR->bounds.MaxY - CR->bounds.MinY + 1,
	                  0x0c0,
	                  0xff,
	                  NULL);
	      }
	      else
	      {
	        /* a superbitmap layer */
	        CR->BitMap = layer_to_move->SuperBitMap;
	        BltBitMap(layer_to_move->rp->BitMap,
	                  CR->bounds.MinX,
	                  CR->bounds.MinY,
	                  CR->BitMap,
	                  CR->bounds.MinX - layer_to_move->bounds.MinX + layer_to_move->Scroll_X,
	                  CR->bounds.MinY - layer_to_move->bounds.MinY + layer_to_move->Scroll_Y,
	                  CR->bounds.MaxX - CR->bounds.MinX + 1,
	                  CR->bounds.MaxY - CR->bounds.MinY + 1,
	                  0x0c0,
	                  0xff,
	                  NULL);
	      }
	    }            
	    else
	    {
	      /*
	      ** The part to be hidden belongs to a simple layer.
	      ** I don't do anything here.
	      */
	    }
	      
	    CR->lobs = _L;
	      
	    while (NULL != _CR && 0 != area_vis)
	    {
	      /*
	      ** There might not just one ClipRect of the layer _L hidden. So
	      ** I have to check them all.
	      */
	      if (layer_to_move == _CR->lobs &&
                  !(_CR->bounds.MinX > CR->bounds.MaxX ||
	            _CR->bounds.MaxX < CR->bounds.MinX ||
	            _CR->bounds.MinY > CR->bounds.MaxY ||
	            _CR->bounds.MaxY < CR->bounds.MinY))
	      {
	        /*
	        ** Make this ClipRect visible
	        */
	        if (0 == (_L->Flags & LAYERSIMPLE))
	        {
	          if (0 == (_L->Flags & LAYERSUPER))
	          {
	            BltBitMap(_CR->BitMap,
	                      ALIGN_OFFSET(_CR->bounds.MinX),
	                      0,
	                      _L->rp->BitMap,
	                      _CR->bounds.MinX,
	                      _CR->bounds.MinY,
	                      _CR->bounds.MaxX - _CR->bounds.MinX + 1,
	                      _CR->bounds.MaxY - _CR->bounds.MinY + 1,
	                      0x0c0,
	                      0xff,
	                      NULL);
	            FreeBitMap(_CR->BitMap);
	          }
	          else
	          {
	            /*
	            ** The part to become visible belongs to a superbitmap layer
	            */
	            BltBitMap(_L->SuperBitMap,
	                      _CR->bounds.MinX - _L->bounds.MinX + _L->Scroll_X,
	                      _CR->bounds.MinY - _L->bounds.MinY + _L->Scroll_Y,
	                      _L->rp->BitMap,
	                      _CR->bounds.MinX,
	                      _CR->bounds.MinY,
	                      _CR->bounds.MaxX - _CR->bounds.MinX + 1,
	                      _CR->bounds.MaxY - _CR->bounds.MinY + 1,
	                      0x0c0,
	                      0xff,
	                      NULL);
	          }
	        }
	        else
	        {
	        }
	          
	        /*
	        ** Mark this ClipRect as visible
	        */
	        _CR -> lobs   = NULL;
                _CR -> BitMap = NULL;
	          
	        /* check whether the whole are has already been moved ... */
	        area_vis -= ((_CR->bounds.MaxX - _CR->bounds.MinX + 1) *
	                     (_CR->bounds.MaxY - _CR->bounds.MinY + 1));
	      }
	      else
	      {
	      }
	        
	      _CR = _CR->Next;
	    } /* while (NULL != _CR && 0 != area_vis) */

            if (0 != (_L->Flags & LAYERSIMPLE))
            {
              /*
              ** The part to become visible belongs to a simple later,
              ** I add that part to the damage list and clear that area.
              */
              /*
              ** The damagelist is relative to the window instead of the
              ** screen 
              */
              struct Rectangle R = CR->bounds;
              R.MinX -= _L->bounds.MinX;
              R.MinY -= _L->bounds.MinY;
              R.MaxX -= _L->bounds.MinX;
              R.MaxY -= _L->bounds.MinY;
              
              OrRectRegion(_L->DamageList, &R);
              _L->Flags |= LAYERREFRESH;
              
              _CallLayerHook(_L->BackFill,
                             _L->rp,
                             _L,
                             &CR->bounds,
                             CR->bounds.MinX,
                             CR->bounds.MinY);
            }

            if (0 != (layer_to_move->Flags & LAYERSIMPLE))
            {
              /*
              ** If L is a simple layer then clear that part from
              ** the damagelist such that no mess happens on the screen.
              */
              struct Rectangle R = CR->bounds;
              R.MinX -= _L->bounds.MinX;
              R.MinY -= _L->bounds.MinY;
              R.MaxX -= _L->bounds.MinX;
              R.MaxY -= _L->bounds.MinY;
              ClearRectRegion(layer_to_move->DamageList, &R);
            }
            
            /*
            ** Now I have to change all the lobs-entries in the layers
            ** behind the layer that became visible (_L) so they are 
            ** pointing to the correct layer
            */
            
            while (NULL != L_tmp -> back)
            {
              L_tmp = internal_WhichLayer(L_tmp->back,
                                          CR->bounds.MinX,
                                          CR->bounds.MinY);
              if (NULL == L_tmp)
                break;
              
              _CR = L_tmp -> ClipRect;
              while (NULL != _CR)
              {
                if (!(CR->bounds.MinX > _CR->bounds.MaxX ||
                      CR->bounds.MinY > _CR->bounds.MaxY ||
                      CR->bounds.MaxX < _CR->bounds.MinX ||
                      CR->bounds.MaxY < _CR->bounds.MinY))
                  _CR -> lobs = _L;
                  
                _CR = _CR->Next;
              }
            } /* while */
	  } 
	  else
	  {
	    struct ClipRect * CR2 = CR;
	    /*
	    ** The ClipRect to become visible is bigger than the one 
	    ** that is visible right now -> More than one ClipRect
	    ** needs to be backed up.
	    */
	    
	    if (NULL == _CR)
	    {
	      kprintf("%s: Error!!\n",__FUNCTION__);
	      break;
	    }
	    
	    /*
	    ** First backup all relevant ClipRects and set their lobs entries
	    ** to the new layer
	    */
	    
	    while (NULL != CR2)
	    {
	      if (NULL == CR2->lobs &&
	          !(CR2->bounds.MinX > _CR->bounds.MaxX ||
	            CR2->bounds.MinY > _CR->bounds.MaxY ||
	            CR2->bounds.MaxX < _CR->bounds.MinX ||
	            CR2->bounds.MaxY < _CR->bounds.MinY))
	      {
	        if (0 == (layer_to_move -> Flags & LAYERSIMPLE))
	        {
	          /*
	          ** the part to be hidden has to go into a bitmap 
	          */
	          if (0 == (layer_to_move -> Flags & LAYERSUPER))
	          {
	            CR2->BitMap =
	                 AllocBitMap(CR2->bounds.MaxX - CR2->bounds.MinX + 1,
	                             CR2->bounds.MaxY - CR2->bounds.MinY + 1,
	                             layer_to_move->rp->BitMap->Depth,
	                             0,
	                             layer_to_move->rp->BitMap);
	                             
	            BltBitMap(layer_to_move->rp->BitMap,
	                      CR2->bounds.MinX,
	                      CR2->bounds.MinY,
	                      CR2->BitMap,
	                      ALIGN_OFFSET(CR2->bounds.MinX),
	                      0,
	                      CR2->bounds.MaxX - CR2->bounds.MinX + 1,
	                      CR2->bounds.MaxY - CR2->bounds.MinY + 1,
	                      0x0c0,
	                      0xff,
	                      NULL);
	          } 
	          else
	          {
	            /* a superbitmap layer */
	            CR2->BitMap = layer_to_move->SuperBitMap;
	            BltBitMap(layer_to_move->rp->BitMap,
	                      CR2->bounds.MinX,
	                      CR2->bounds.MinY,
	                      CR2->BitMap,
	                      CR2->bounds.MinX - layer_to_move->bounds.MinX + layer_to_move->Scroll_X,
	                      CR2->bounds.MinY - layer_to_move->bounds.MinY + layer_to_move->Scroll_Y,
	                      CR2->bounds.MaxX - CR2->bounds.MinX + 1,
	                      CR2->bounds.MaxY - CR2->bounds.MinY + 1,
	                      0x0c0,
	                      0xff,
	                      NULL);
	          }
	        }
	        else
	        {
	          struct Rectangle R = CR2->bounds;
	          R.MinX -= layer_to_move->bounds.MinX;
	          R.MinY -= layer_to_move->bounds.MinY;
	          R.MaxX -= layer_to_move->bounds.MinX;
	          R.MaxY -= layer_to_move->bounds.MinY;
	          ClearRectRegion(layer_to_move->DamageList, &R);
	        }
	        
	        CR2->lobs = _L;
	        
	        area_hid -= (CR2->bounds.MaxX - CR2->bounds.MinX + 1) *
	                    (CR2->bounds.MaxY - CR2->bounds.MinY + 1);
	        
	        if (0 == area_hid)
	          break;
	      } /* if */
	      CR2 = CR2->Next;
	    } /* while */
	    
	    /*
	    ** Now make the hidden part visible
	    */
	    
	    if (0 == (_L->Flags & LAYERSIMPLE))
	    {
	      if (0 == (_L->Flags & LAYERSUPER))
	      {
	        BltBitMap(_CR->BitMap,
	                  ALIGN_OFFSET(_CR->bounds.MinX),
	                  0,
	                  _L->rp->BitMap,
	                  _CR->bounds.MinX,
	                  _CR->bounds.MinY,
	                  _CR->bounds.MaxX - CR->bounds.MinX + 1,
	                  _CR->bounds.MaxY - CR->bounds.MinY + 1,
	                  0x0c0,
	                  0xff,
	                  NULL);
	        FreeBitMap(_CR->BitMap);
	        _CR->BitMap = NULL;
	      }
	      else
	      {
	        /* the part to become visible belongs to a superbitmap layer */
	        BltBitMap(_L->SuperBitMap,
	                  _CR->bounds.MinX - _L->bounds.MinX + _L->Scroll_X,
	                  _CR->bounds.MinY - _L->bounds.MinY + _L->Scroll_Y,
	                  _L->rp->BitMap,
	                  _CR->bounds.MinX,
	                  _CR->bounds.MinY,
	                  _CR->bounds.MaxX - _CR->bounds.MinX + 1,
	                  _CR->bounds.MaxY - _CR->bounds.MinY + 1,
	                  0x0c0,
	                  0xff,
	                  NULL);
	      }
	    }
	    else
	    {
	      /*
	      ** The part to become visible belongs to a simple layer;
	      ** I add that part to the damage list and clear the area. 
	      */
	      /*
	      ** The damage list is relative to the window instead to 
	      ** the screen!
	      */
	      struct Rectangle R = _CR->bounds;
	      R.MinX -= _L->bounds.MinX;
	      R.MinY -= _L->bounds.MinY;
	      R.MaxX -= _L->bounds.MinX;
	      R.MaxY -= _L->bounds.MinY;
	      OrRectRegion(_L->DamageList, &R);
	      _L->Flags |= LAYERREFRESH;
	      
	      _CallLayerHook(_L->BackFill,
	                     _L->rp,
	                     _L,
	                     &_CR->bounds,
	                     _CR->bounds.MinX,
	                     _CR->bounds.MinY);
	    }
	    
	    /*
	    ** Mark that part as visible
	    */
	    _CR -> lobs   = NULL;
	    _CR -> BitMap = NULL;
	    
	    /*
	    ** Now I have to change all lobs entries in the layers
	    ** behind the layer that became visible (_L) so they are
	    ** pointing to the correct layer.
	    */
	    
	    while (NULL != L_tmp->back)
	    {
	      L_tmp = internal_WhichLayer(L_tmp->back,
	                                  CR->bounds.MinX,
	                                  CR->bounds.MinY);
	      
	      if (NULL == L_tmp)
	        break;
	        
	      CR2 = L_tmp -> ClipRect;
	      while (NULL != CR2)
	      {
	        if (!(CR2->bounds.MinX > _CR->bounds.MaxX ||
	              CR2->bounds.MinY > _CR->bounds.MaxY ||
	              CR2->bounds.MaxX < _CR->bounds.MinX ||
	              CR2->bounds.MaxY < _CR->bounds.MinY))
	          CR2 -> lobs = _L;
	        CR2 = CR2->Next;
	      } /* while (all CLipRects) */
	    } /* while */
	  }
	} /* if */
      } /* if */	
    } /* while */
  } /* if .. else .. */

  CleanupLayers(LI);

  if (NULL != oldclipregion)
    InstallClipRegion(layer_to_move, oldclipregion);

  InstallClipRegionClipRects(LI);
 
  UnlockLayers(LI);

  return TRUE;
  AROS_LIBFUNC_EXIT
} /* MoveLayerInFrontOf */
