/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/layers.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include "layers_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH3(struct Layer *, WhichLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(LONG               , x,  D0),
	AROS_LHA(LONG               , y,  D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 22, Layers)

/*  FUNCTION
        Determines in which layer the point (x,y) is to be found.
        Starts with the frontmost layer. 

    INPUTS
        li - pointer to Layers_Info structure
        x  - x-coordinate
        y  - y-coordinate

    RESULT

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

  struct Layer *l;
  struct Region * tmpr = NULL;

  D(bug("WhichLayer(li @ $%lx, x %ld, y %ld)\n", li, x, y));
  
  LockLayers(li);

  for(l = li->top_layer; l != NULL; l = l->back)
    if(IS_VISIBLE(l) &&
       x >= l->shape->bounds.MinX && x <= l->shape->bounds.MaxX &&
       y >= l->shape->bounds.MinY && y <= l->shape->bounds.MaxY)
    {
       struct RegionRectangle * rr;
       int found = FALSE;
       int _x, _y;
      
       /* 
        * Must make a copy of the shape of the layer and
        * cut it down to the potentially visible part
        */ 
       tmpr = CopyRegion(l->shape);
       
       if (tmpr)
       {
         if (!AndRegionRegion(l->parent->shape, tmpr))
         {
           DisposeRegion(tmpr);
           tmpr = NULL;
           return NULL;
         }
       }
       _x = x - tmpr->bounds.MinX;
       _y = y - tmpr->bounds.MinY;
       rr = tmpr->RegionRectangle;
       
       /*
        * If it is just a square the region is empty.
        */
       if (NULL == rr)
         break;
       while (rr)
       {
         if (_x >= rr->bounds.MinX && _x <= rr->bounds.MaxX &&
             _y >= rr->bounds.MinY && _y <= rr->bounds.MaxY)
         {
           found = TRUE;
           break;
         }
         rr = rr->Next;
       }
       
       if (TRUE == found)
         break;
         
       DisposeRegion(tmpr);
       tmpr = NULL;
    }


  UnlockLayers(li);

  if (tmpr)
    DisposeRegion(tmpr);

  return l;

  AROS_LIBFUNC_EXIT
} /* WhichLayer */
