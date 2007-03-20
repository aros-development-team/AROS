/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SyncSBitMap()
    Lang: english
*/
#include <graphics/layers.h>
#include <graphics/clip.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, SyncSBitMap,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 74, Graphics)

/*  FUNCTION
        If the layer has a superbitmap all the parts that are visible will
        be copied into the superbitmap. This is usually not done except when
        parts of a superbitmapped layer become hidden the visible parts are 
        stored into the superbitmap.

    INPUTS
        l  - pointer to superbitmapped layer 

    RESULT
        The superbitmap will be synchronized with the visible part. The
        superbitmap attached to the layer will be up-to-date with what's
        really in the layer.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CopySBitMap()
 
    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  struct ClipRect * CR = l->ClipRect;

  if (NULL == l->SuperBitMap || (l->Flags & LAYERSUPER) == 0)
    return;

  while (NULL != CR)
  {
    /* a cliprect of a superbitmapped layer is visible if lobs==NULL*/
    if (NULL == CR->lobs)
    {
      /* I have to backup this part into the SuperBitMap! I find the
         data in the bitmap of the rastport of this layer */
      BltBitMap(l->rp->BitMap,
                CR->bounds.MinX,
                CR->bounds.MinY,
                l->SuperBitMap,
                CR->bounds.MinX - l->bounds.MinX - l->Scroll_X,
                CR->bounds.MinY - l->bounds.MinY - l->Scroll_Y,
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
} /* SyncSBitMap */

