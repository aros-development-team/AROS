/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free the memory occupied by a BitMap.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/gfx.h>
#include <proto/graphics.h>

        AROS_LH1(void, FreeBitMap,

/*  SYNOPSIS */
        AROS_LHA(struct BitMap *, bm, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 154, Graphics)

/*  FUNCTION
        Returns the memory occupied by the BitMap to the system.

    INPUTS
        bm - The result of AllocBitMap(). Must be non-NULL.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocBitMap(), AllocRaster(), FreeRaster()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (!bm) return;
    
    ASSERT_VALID_PTR(bm);

    if (IS_HIDD_BM(bm))
    {
        OOP_Object *bmobj = HIDD_BM_OBJ(bm);
        
        D(bug("%s: Free HIDD bitmap %p (obj %p)\n", __func__, bm, bmobj));

        if (HIDD_BM_FLAGS(bm) & HIDD_BMF_SHARED_PIXTAB)
        {
            /* NULL colormap otherwise bitmap killing also kills
               the colormap object of the bitmap object
               from which we shared it = to which it belongs */
            if (bmobj)
                HIDD_BM_SetColorMap(bmobj, NULL);
        }
        else if (HIDD_BM_FLAGS(bm) & HIDD_BMF_SCREEN_BITMAP) // (bm->Flags & BMF_DISPLAYABLE)
        {
            FreeVec(HIDD_BM_PIXTAB(bm));
        }

        if (bmobj)
            HIDD_Gfx_DisposeBitMap(HIDD_BM_DRVDATA(bm)->gfxhidd, bmobj);

        FreeMem(bm, sizeof (struct BitMap) + sizeof(PLANEPTR) * HIDD_BM_EXTRAPLANES);
    }
    else
    {
        ULONG plane;
        ULONG width;

        D(bug("%s: Free plain bitmap %p\n", __func__, bm));

        width = bm->BytesPerRow * 8;

        for (plane=0; plane < bm->Depth; plane ++)
        {
            /* Take care of two special cases: plane pointers containing NULL or -1
             * are supported by BltBitMap() as all 0's and all 1's planes
             */
            if (bm->Planes[plane] && bm->Planes[plane] != (PLANEPTR)-1)
            {
                ASSERT_VALID_PTR(bm->Planes[plane]);
                FreeRaster (bm->Planes[plane], width, bm->Rows);
            }
        }

        FreeMem (bm, sizeof(struct BitMap) +
                     ((bm->Depth > 8) ? (bm->Depth - 8) * sizeof(PLANEPTR) : 0));
    }

    AROS_LIBFUNC_EXIT
} /* FreeBitMap */
