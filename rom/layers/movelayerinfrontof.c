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
kprintf("Moving a layer further to the front\n");
*/
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
kprintf("Found a CR that is hidden! %d vs. %d\n",CR->lobs->priority,
  other_layer->priority);
kprintf("This ClipRect: MinX: %d, MaxX: %d, MinY: %d, MaxY %d\n",
   CR->bounds.MinX,CR->bounds.MaxX,CR->bounds.MinY,CR->bounds.MaxY);
*/
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
              Now I have to look for the ClipRect in L_tmp that
              is now hidden.
	    */
           CR_tmp = internal_WhichClipRect(L_tmp,
                                           CR->bounds.MinX,
                                           CR->bounds.MinY);
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
                         CR_tmp->bounds.MinX & 0x0f,
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
             /* It's a simple layer. I don't do anything here */
           }

           /* Whatever can be found in CR will be shown now. */

           if (0 == (layer_to_move->Flags & LAYERSIMPLE))
           {
             if (0 == (layer_to_move->Flags & LAYERSUPER))
             {
               /* it's a smart layer */
               BltBitMap(CR->BitMap,
                         CR->bounds.MinX & 0x0f,
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
             BltBitMap(layer_to_move->rp->BitMap,
                       0,
                       0,
                       layer_to_move->rp->BitMap,
                       CR->bounds.MinX,
                       CR->bounds.MinY,
                       CR->bounds.MaxX - CR->bounds.MinX + 1,
                       CR->bounds.MaxY - CR->bounds.MinY + 1,
                       0x0,
                       0xff,
                       NULL);
           }

if (NULL == CR_tmp)
  kprintf("Error!\n");

           CR    ->BitMap = NULL; 

           CR_tmp->lobs   = layer_to_move;
           CR    ->lobs   = NULL;
           /* 
              Now I have to change all the lobs entries for this area
              in the layers that are further behind. What a pain.
            */
           while (NULL != L_tmp->back)
	   {
/*
kprintf("     ClipRect: MinX: %d, MaxX: %d, MinY: %d, MaxY %d\n",
   CR_tmp->bounds.MinX,CR_tmp->bounds.MaxX,
   CR_tmp->bounds.MinY,CR_tmp->bounds.MaxY);
*/ 
             L_tmp = internal_WhichLayer(L_tmp->back,
                                         CR->bounds.MinX,
                                         CR->bounds.MinY);
             if (NULL == L_tmp)
               break;
             CR_tmp = internal_WhichClipRect(L_tmp,
                                             CR->bounds.MinX,
                                             CR->bounds.MinY);
             CR_tmp -> lobs = layer_to_move;
	   } /* while */
	 } /* if (...priority > .. priority) */
       } /* if */
       CR = CR -> Next;
     }
  }
  else
  {
    /*
       The layer_to_move goes further in the back, which
       means that unvisible parts stay unvisible and some
       visible parts might get hidden.
       I have to visit all the ClipRects of the layer_to_move...
     */
    while (NULL != CR)
    {
      /* Has this ClipRect been visible before? */
      if (NULL == CR->lobs)
      {
        /* 
           It was visible. Should it be hidden now? 
         */
        struct Layer * L_tmp = 
                     WhichLayer(LI, CR->bounds.MinX , CR->bounds.MinY);
        if (L_tmp != layer_to_move )
	{
          /* 
             Another layer is in front of the layer_to_move layer in this 
             area.
             All that needs to be done now is to backup the area that is
             displayed into the bitmap of a ClipRect and display the
             area of the layer L_tmp that was hidden before.
             L_tmp is visible in that area now.
           */
          struct ClipRect * CR_tmp, * CR2;
          struct Layer * L_top = L_tmp; /* need a backup */
          BOOL equal_rects;
          CR_tmp = L_tmp->ClipRect;

          while (!(CR_tmp->bounds.MinX >= CR->bounds.MinX &&
                   CR_tmp->bounds.MinY >= CR->bounds.MinY && 
                   CR_tmp->bounds.MaxX <= CR->bounds.MaxX &&
                   CR_tmp->bounds.MaxY <= CR->bounds.MaxY))
          {
            CR_tmp = CR_tmp->Next;
            
if (NULL == CR_tmp)
  kprintf("mlio: Fatal error!\n");
/*
  This fatal error occurs if the visible CR is smaller
  than one in the list of layer L_tmp. The question just is
  whether this case is possible at all???
 */
          }


          /*
             CR_tmp contains the bitmap to become visible next.
             The problem is that this ClipRect might have a different size
             as the cliprect that is shown right not. Therefore I have to go 
             through the list of ClipRects that are invisible now and
             fit into CR and make those visible.
             So what I will have to do now is the following. I will have
             to go through all ClipRects of the currently invisible 
             Layer and check whether they fit into the ClipRect CR_tmp.
             If so, I will take them out of the list and make them
             visible. CR_tmp will finally hold the size of the ClipRect.
	   */

          if (CR->bounds.MinX != CR_tmp->bounds.MinX ||
              CR->bounds.MaxX != CR_tmp->bounds.MaxX ||
              CR->bounds.MinY != CR_tmp->bounds.MinY ||
              CR->bounds.MaxY != CR_tmp->bounds.MaxY )
          {
            equal_rects = FALSE;
          }
          else
          {
            equal_rects = TRUE;
          }

           /*
             Get a bitmap of the size of the still visible bitmap area
            */
           /*
             And backup the data from the displayed bitmap to a bitmap,
             if its a smart or superbitmap layer
	    */

          if (0 == (layer_to_move->Flags & LAYERSIMPLE))
          { 
            if (0 == (layer_to_move-> Flags & LAYERSUPER))
	    {
              /* no SuperBitMap */
              CR->BitMap = 
                    AllocBitMap(CR->bounds.MaxX - CR->bounds.MinX + 1 + 16,
                                CR->bounds.MaxY - CR->bounds.MinY + 1,
                                GetBitMapAttr(CR_tmp->BitMap, BMA_DEPTH),
                                0,
                                NULL /* !!! */ );

              BltBitMap(layer_to_move->rp->BitMap,
                        CR->bounds.MinX,
                        CR->bounds.MinY,
                        CR->BitMap,
                        CR->bounds.MinX & 0x0f,
                        0,
                        CR->bounds.MaxX - CR->bounds.MinX + 1,
                        CR->bounds.MaxY - CR->bounds.MinY + 1,
                        0x0c0,
                        0xff,
                        NULL);
 
            }
            else
	    {
              /* with SuperBitMap */
              CR->BitMap = layer_to_move->SuperBitMap;
              BltBitMap(layer_to_move->rp->BitMap,
                        CR->bounds.MinX,
                        CR->bounds.MinY,
                        layer_to_move->SuperBitMap,
                        CR->bounds.MinX - layer_to_move->bounds.MinX +
                                          layer_to_move->Scroll_X,
                        CR->bounds.MinY - layer_to_move->bounds.MinY +
                                          layer_to_move->Scroll_Y,
                        CR->bounds.MaxX - CR->bounds.MinX + 1,
                        CR->bounds.MaxY - CR->bounds.MinY + 1,
                        0x0c0,
                        0xff,
                        NULL);
	    }
          }
          else
          {
            /* nothing to do for simple layers.*/
          }
          
           if (0 == (L_tmp->Flags & LAYERSIMPLE))
           {
             if (0 == (L_tmp->Flags & LAYERSUPER))
             {
               /*
                  Now display all the previously hidden parts that fit into 
                  the ClipRect that was shown before. I start out with the
                  first one, which might also be the last one.
	        */

               BltBitMap(CR_tmp->BitMap,
                         CR_tmp->bounds.MinX & 0x0f,
                         0,
                         layer_to_move->rp->BitMap,
                         CR_tmp->bounds.MinX,
                         CR_tmp->bounds.MinY,
                         CR_tmp->bounds.MaxX - CR_tmp->bounds.MinX + 1,
                         CR_tmp->bounds.MaxY - CR_tmp->bounds.MinY + 1,
                         0x0c0,
                         0xff,
                         NULL);
               FreeBitMap(CR_tmp->BitMap);
	     }
             else
             {
               BltBitMap(L_tmp->SuperBitMap,
                         CR_tmp->bounds.MinX - L_tmp->bounds.MinX +
                                               L_tmp->Scroll_X,
                         CR_tmp->bounds.MinY - L_tmp->bounds.MinY +
                                               L_tmp->Scroll_Y,
                         layer_to_move->rp->BitMap,
                         CR_tmp->bounds.MinX,
                         CR_tmp->bounds.MinY,
                         CR_tmp->bounds.MaxX - CR_tmp->bounds.MinX + 1,
                         CR_tmp->bounds.MaxY - CR_tmp->bounds.MinY + 1,
                         0x0c0,
                         0xff,
                         NULL);
	     }
           }
           else
           {
             /* it's a simple layer. */
             struct Rectangle Rect = CR->bounds;
             Rect.MinX -= layer_to_move->bounds.MinX;
             Rect.MinY -= layer_to_move->bounds.MinY;
             Rect.MaxX -= layer_to_move->bounds.MinX;
             Rect.MaxY -= layer_to_move->bounds.MinY;
             OrRectRegion(layer_to_move->DamageList, &Rect);
             layer_to_move->Flags |= LAYERREFRESH;
             /* clear the whole area */
             BltBitMap(layer_to_move->rp->BitMap,
                       0,
                       0,
                       layer_to_move->rp->BitMap,
                       CR->bounds.MinX,
                       CR->bounds.MinY,
                       CR->bounds.MaxX - CR->bounds.MinX + 1,
                       CR->bounds.MaxY - CR->bounds.MinY + 1,
                       0x0c0,
                       0xff,
                       NULL);
           }
           /* 
              If the hidden ClipRect had the same size as the
              visible one I don't have to make other Cliprects
              visible anymore 
            */
           /*
                CR_tmp has become visible
            */
           CR_tmp->lobs   = NULL;
           CR_tmp->BitMap = NULL;

           CR -> lobs = L_tmp;

           if (FALSE == equal_rects)
	   {
             /* I am not going to free the first one that fitted. It
                will stay in the list. The other ones I will display
                and then free.
              */
             CR_tmp->bounds = CR->bounds;

             while (NULL != CR_tmp -> Next)
	     {
               struct ClipRect * CR_del = CR_tmp -> Next;
               if (CR_del->bounds.MinX >= CR->bounds.MinX &&
                   CR_del->bounds.MinY >= CR->bounds.MinY &&
                   CR_del->bounds.MaxX <= CR->bounds.MaxX &&
                   CR_del->bounds.MaxY <= CR->bounds.MaxY)
	       {
                 /* Take this ClipRect out of the list */
                 CR_tmp -> Next = CR_del -> Next;

                 if (0 == (L_tmp->Flags & LAYERSIMPLE))
                 {
                   if (0 == (L_tmp->Flags & LAYERSUPER))
		   {
                     BltBitMap(CR_del->BitMap,
                               CR_del->bounds.MinX & 0x0f,
                               0,
                               layer_to_move->rp->BitMap,
                               CR_del->bounds.MinX,
                               CR_del->bounds.MinY,
                               CR_del->bounds.MaxX - CR_del->bounds.MinX + 1,
                               CR_del->bounds.MaxY - CR_del->bounds.MinY + 1,
                               0x0c0,
                               0xff,
                               NULL);
                     FreeBitMap(CR_del -> BitMap);
                   }
                   else
		   {
                     BltBitMap(CR_del->BitMap,
                               CR_del->bounds.MinX - L_tmp->bounds.MinX +
                                                     L_tmp->Scroll_X,
                               CR_del->bounds.MinY - L_tmp->bounds.MinY +
                                                     L_tmp->Scroll_Y,
                               layer_to_move->rp->BitMap,
                               CR_del->bounds.MinX,
                               CR_del->bounds.MinY,
                               CR_del->bounds.MaxX - CR_del->bounds.MinX + 1,
                               CR_del->bounds.MaxY - CR_del->bounds.MinY + 1,
                               0x0c0,
                               0xff,
                               NULL);
		   }
		 }
		 else
		 {
		   /* a simple bitmap. It's already take care of above... */
		 }

                 _FreeClipRect(CR_del, L_tmp);
                 /* 
                    Do not procede to the Next ClipRect as we
                    changed the next entry by deleting CR_del from the list!!! 
                  */
               } /* if */
               else
               {
                 CR_tmp = CR_tmp -> Next;
	       }
	     } /* while */
	   } /* if () */

           /*
              One more thing to do:
              I have to search ALL layers' cliprects that are behind the
              now visible layer in that area and change the lobs entry
              as these parts are now hidden by another layer.
            */
           while (NULL != L_tmp->back)
           {
             L_tmp = internal_WhichLayer(L_tmp->back,
                                         CR->bounds.MinX,
                                         CR->bounds.MinY);
             /*
                 L_tmp points to a layer that is farther behind the
                 now visible part and has an area that is behind the
                 now visible part.
              */
             if (NULL == L_tmp)
               break;

             if (L_tmp != layer_to_move)
	     {
               CR2 = L_tmp->ClipRect;
               while (NULL != CR2)
               {
		 
                 if (CR2->bounds.MinX >= CR->bounds.MinX &&
                     CR2->bounds.MinY >= CR->bounds.MinY &&
                     CR2->bounds.MaxX <= CR->bounds.MaxX &&
                     CR2->bounds.MaxY <= CR->bounds.MaxY)
                   CR2 -> lobs = L_top;
		   
                 CR2 = CR2 -> Next;
	       } /* while */
	     } /* if */
           } /* while */
        } /* if (L_tmp != ...) */
      } /* if */
      CR = CR->Next;
    } /* while (NULL != CR) */
  }

  CleanupLayers(LI);
 
  UnlockLayers(LI);

  return TRUE;
  AROS_LIBFUNC_EXIT
} /* MoveLayerInFrontOf */
