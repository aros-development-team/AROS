/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/rastport.h>

#include "layers_intern.h"

void CreateClipRectsBehindLayer(struct Layer * L)
{
  struct Layer * L_behind = L->back;
  struct BitMap * bm = L->rp->BitMap;
  LONG x0 = L->bounds.MinX;
  LONG y0 = L->bounds.MinY;
  LONG x1 = L->bounds.MaxX;
  LONG y1 = L->bounds.MaxY;
  struct ClipRect *(* FunctionArray[])() = {
                	(void *)&Case_0,
                 	(void *)&Case_1,
                 	(void *)&Case_2,
                 	(void *)&Case_3,
                 	(void *)&Case_4,
                 	(void *)&Case_5,
                 	(void *)&Case_6,
                 	(void *)&Case_7,
                 	(void *)&Case_8,
                 	(void *)&Case_9,
                 	(void *)&Case_10,
                 	(void *)&Case_11,
                 	(void *)&Case_12,
                 	(void *)&Case_13,
                 	(void *)&Case_14,
                 	(void *)&Case_15};


     /* 
       Now we have to examine all the layers behind this layer for overlapping
       cliprects. Overlapping cliprects mean, that the newly created layer L
       is overlaying a previously created layer or at least a part of it.
       Parts of layers that were hidden before by something do not
       have to be looked at.
     */

    while (NULL != L_behind)
    {
      /* 
         we can skip this whole layer if the new one does not 
         overlap it at all! 
       */
      if (x0 > L_behind->bounds.MaxX ||
          x1 < L_behind->bounds.MinX ||
          y0 > L_behind->bounds.MaxY ||
          y1 < L_behind->bounds.MinY    )
      {
        /* skip this layer */

        /* 
           Do not leave a mark in this layer saying that nothing
           has changed. We'll need this later.
        */
        L_behind -> reserved[0] = 0;
      }
      else /* examine this layer and its cliprects. */
      {
        struct ClipRect * CR_behind = L_behind -> ClipRect;

        /* 
           examine all cliprects of the behind-layer whether they are
           overlapped somehow by the new layer. Only treat those
           layers that were not previously hidden by some other layer. 
        */
        while (NULL != CR_behind)
        {
          /* 
             We can skip this cliprect if the new layer does not overlap
             it at all. This test is necessary for the tests further down.
           */
          if (x0 > CR_behind->bounds.MaxX ||
              x1 < CR_behind->bounds.MinX ||
              y0 > CR_behind->bounds.MaxY ||
              y1 < CR_behind->bounds.MinY    )
          {
            /* skip this cliprect */

          }
          else
          {
            /* hm, it can also only be partially overlapped */
            /* 
               This is the really difficult case. If the new layer partially
               overlaps a cliprect of a layer behind it this will cause the
               cliprect behind it to be split up into several other cliprects.
               Depending on how it is overlapping the cliprect the
               cliprect is split up into up to 5 cliprects.
               See further documentation for this in cliprectfunctions.c.
            */
            /* 
               Now I know that the new layer is overlapping the other
               cliprect somehow. If the other cliprect was overlapped
               before by some layer we may only change the lobs-entry,
               no bitmaps may be copied! (Wrong results otherwise!)
               The lobs entry will be pointing to the new layer.
              This is handled in case_0!! 
	     */
            int OverlapIndex = 0;

            if (x0 > CR_behind->bounds.MinX &&
                x0 < CR_behind->bounds.MaxX )
               OverlapIndex |= 8;

            if (y0 > CR_behind->bounds.MinY &&
                y0 < CR_behind->bounds.MaxY )
               OverlapIndex |= 4;

            if (x1 > CR_behind->bounds.MinX &&
                x1 < CR_behind->bounds.MaxX )
               OverlapIndex |= 2;

            if (y1 > CR_behind->bounds.MinY &&
                y1 < CR_behind->bounds.MaxY )
               OverlapIndex |= 1;

            /* 
               Leave a mark in thise layer saying that something
               has change. We'll need this later
             */
            L_behind->reserved[0] = 1;
            /* let's call the routine that treats that particular case 
               The chain of ClipRects now looks like this:
               CR_behind->A->B->...
            */
            CR_behind = (struct ClipRect *)
                FunctionArray[OverlapIndex](&L->bounds, CR_behind, bm, L);
              
            /* 
               CR_behind should point to the cliprect *BEFORE* the
               next ClipRect that existed before the call to the
               FunctionArray-Function.
               If the chain of cliprects now looks like this
               CR_behind(old one)->New1->New2->A->B
               CR_behind should point to New2 now.
            */
          }
          /* visit the next cliprect of this layer */
          CR_behind = CR_behind -> Next;
        } /* while */
      }

      /* visit the next one of the layers that is behind the newly created
         one, if there is still one. When leaving this loop, L->behind
         has to point to the layer that is farthest in the back */
      if (NULL == L_behind->back )
        break;
      else
        L_behind = L_behind->back;
    }

    /* ... and there is something else that we have to do:
       We have to visit all these rectangles that were created by the
       new layer and especially those rectangles that have a pointer
       in "layer->lobs" to the new layer. To do a proper splitting of all
       rectangles we have to take exactly these rectangles and split all
       other rectangles (with pointer to the new layer) of layers that are
       in front of these. But we won't split the topmost layer, though!
    */

    if (NULL != L_behind)
    {
      while (L != L_behind->front)
      {
        /* visit all ClipRects of the layer L_behind if this layer
           overlapped the newly created one at all (other ones are
           a wast of time)
         */
        struct ClipRect * CR_behind = L_behind -> ClipRect;
        /* check for the mark */
        if ( 0 != L_behind->reserved[0])
          while (NULL != CR_behind)
          {
            struct Layer * L_infront = L_behind -> front;

            /* does this ClipRect have the pointer in lobs to the newly
               created layer? */
            if (CR_behind -> lobs == L)
            {
              struct ClipRect * CR_infront = L_infront -> ClipRect;

              /* yes, it does have one. So we use this Cliprect to split
                 up other ClipRects of layers in front of this layer and
                 that also have a pointer to the newly created layer in
                 lobs. */
              while (NULL != CR_infront)
              {
                if (CR_infront -> lobs == L)
                {

                 /* we can skip this cliprect if the ClipRect CR_behind does not
                    overlap it at all. This test is necessary for the test further
                    down.
                  */
                  if (CR_behind->bounds.MinX > CR_infront->bounds.MaxX ||
                      CR_behind->bounds.MaxX < CR_infront->bounds.MinX ||
                      CR_behind->bounds.MinY > CR_infront->bounds.MaxY ||
                      CR_behind->bounds.MaxY < CR_infront->bounds.MinY    )
                  {
                     /* skip this cliprect */

                  }
                  else
                  {
                    /* hm, it can also only be partially overlapped */
                    /* This is the really difficult case. If the new layer partially
                       overlaps a cliprect of a layer behind it this will cause the
                       cliprect behind it to be split up into several other cliprects.
                       Depending on how it is overlapping the cliprect the
                       cliprect is split up into up to 5 cliprects.
                       See further documentation for this in cliprectfunctions.c.
                     */
                     int OverlapIndex = 0;

                     if (CR_behind->bounds.MinX > CR_infront->bounds.MinX &&
                         CR_behind->bounds.MinX < CR_infront->bounds.MaxX )
                       OverlapIndex |= 8;

                     if (CR_behind->bounds.MinY > CR_infront->bounds.MinY &&
                         CR_behind->bounds.MinY < CR_infront->bounds.MaxY )
                       OverlapIndex |= 4;

                     if (CR_behind->bounds.MaxX > CR_infront->bounds.MinX &&
                         CR_behind->bounds.MaxX < CR_infront->bounds.MaxX )
                       OverlapIndex |= 2;

                     if (CR_behind->bounds.MaxY > CR_infront->bounds.MinY &&
                         CR_behind->bounds.MaxY < CR_infront->bounds.MaxY )
                       OverlapIndex |= 1;

                     /* let's call the routine that treats that particular case
                        The chain of ClipRects now looks like this:
                        CR_behind->A->B->...
                      */
                     CR_infront = (struct ClipRect *)
                       FunctionArray[OverlapIndex]
                          (&CR_behind->bounds, CR_infront, bm, L);
                     /* CR_behind should point to the cliprect *BEFORE* the
                        next ClipRect that existed before the call to the
                        FunctionArray-Function.
                        If the chain of cliprects now looks like this
                        CR_behind(old one)->New1->New2->A->B
                        CR_behind should point to New2 now.
                     */
                  } /* if */
                }

                CR_infront = CR_infront -> Next;
              }
            }

            CR_behind = CR_behind -> Next;
          } /* while */
        L_behind = L_behind->front;
      } /* while */
    } /* if */

}
