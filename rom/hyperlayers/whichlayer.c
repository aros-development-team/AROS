/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

  D(bug("WhichLayer(li @ $%lx, x %ld, y %ld)\n", li, x, y));
  
  LockLayers(li);

  for(l = li->top_layer; l != NULL; l = l->back)
    if(IS_VISIBLE(l) &&
       x >= l->visibleshape->bounds.MinX && x <= l->visibleshape->bounds.MaxX &&
       y >= l->visibleshape->bounds.MinY && y <= l->visibleshape->bounds.MaxY)
    {
       struct RegionRectangle * rr;

       int found = FALSE;
       int _x, _y;

       _x = x - l->visibleshape->bounds.MinX;
       _y = y - l->visibleshape->bounds.MinY;
       rr = l->visibleshape->RegionRectangle;
       
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

    }


  UnlockLayers(li);

  return l;

  AROS_LIBFUNC_EXIT
} /* WhichLayer */
