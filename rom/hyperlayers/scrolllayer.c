/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/clip.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <graphics/layers.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include "layers_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH4(void, ScrollLayer,

/*  SYNOPSIS */
	AROS_LHA(LONG          , dummy, A0),
	AROS_LHA(struct Layer *, l    , A1),
	AROS_LHA(LONG          , dx   , D0),
	AROS_LHA(LONG          , dy   , D1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 12, Layers)

/*  FUNCTION
        For superbitmapped layers this function work like this:.
        It updates the contents of the superbitmap with what is
        visible on the display, repositions the superbitmap
        and redraws the display.
        For non-superbitmapped layers, all subsequent (x,y) pairs 
        are adjusted by the scroll(x,y) value in the layer. If
        ScrollLayer(-10,-3) was called and (0,0) is drawn it will
        finally end up at coordinates (10, 3) in the superbitmap.

    INPUTS
        l  - pointer to layer
        dx - delta x to add to current x scroll value
        dy - delta y to add to current y scroll value

    RESULT
        

    NOTES

    EXAMPLE

    BUGS
      not tested

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  if (0 == dx && 0 == dy)
    return;

  LockLayer(0, l);

  /* if it's a superbitmapped layer */
  if ((l->Flags & LAYERSUPER) != 0)
  {
    /* store the visible "stuff" to the superbitmap */
    SyncSBitMap(l);
    
    l->Scroll_X -= dx;
    l->Scroll_Y -= dy;

    /* show what's in the superbitmap */
    CopySBitMap(l);
  }
  else
  {
    l->Scroll_X -= dx;
    l->Scroll_Y -= dy;
  }
  
  UnlockLayer(l);

  return;  

  AROS_LIBFUNC_EXIT
} /* ScrollLayer */
