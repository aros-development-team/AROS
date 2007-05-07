/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CopySBitMap()
    Lang: english
*/
#include <graphics/layers.h>
#include <graphics/clip.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, CopySBitMap,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 75, Graphics)

/*  FUNCTION
        If the layer has a superbitmap all the parts that are visible will
        be refreshed with what is in the superbitmap. This function should
        be called after the SuperBitMap of the layer was synchronized with
        SyncSBitMap() and manipulated. 

    INPUTS
        l  - pointer to superbitmapped layer 

    RESULT
        The layer will show the true contents of the superbitmap that is
        attached to it

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SyncSBitMap()
 
    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  struct ClipRect * CR = l->ClipRect;

  if (NULL == l->SuperBitMap || (l->Flags & LAYERSUPER) == 0)
    return;

  while (NULL != CR)
  {
    /* a cliprect of a superbitmapped layer is visible if lobs==NULL,
       only these I have to copy into the rastport's bitmap */
    if (NULL == CR->lobs)
    {
      /* I have to backup this part into the SuperBitMap! I find the
         data in the bitmap of the rastport of this layer */
      BltBitMap(l->SuperBitMap,
                CR->bounds.MinX - l->bounds.MinX - l->Scroll_X,
                CR->bounds.MinY - l->bounds.MinY - l->Scroll_Y,
                l->rp->BitMap,
                CR->bounds.MinX,
                CR->bounds.MinY,
                CR->bounds.MaxX - CR->bounds.MinX + 1,
                CR->bounds.MaxY - CR->bounds.MinY + 1,
                0x0c0,
                0xff,
                NULL
            );
    }
    CR = CR->Next;
  }

  AROS_LIBFUNC_EXIT
} /* CopySBitMap */
