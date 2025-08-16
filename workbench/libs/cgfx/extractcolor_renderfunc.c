/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <clib/macros.h>
#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

ULONG extcol_render(struct extcol_render_data *ecrd, LONG x_src, LONG y_src,
                           OOP_Object *bm_obj, OOP_Object *dst_gc,
                           struct Rectangle *rect, struct GfxBase *GfxBase)
{
    /* Get the info from the hidd */
    struct IntCGFXBase *CyberGfxBase = ecrd->CyberGfxBase;
    struct BitMap      *bm     = ecrd->destbm;
    HIDDT_Pixel        *pixbuf = CyberGfxBase->pixel_buf;
    LONG   x_dest = rect->MinX;
    LONG   y_dest = rect->MinY;
    ULONG  xsize  = rect->MaxX - rect->MinX + 1;
    ULONG  ysize  = rect->MaxY - rect->MinY + 1;
    LONG   pixels_left_to_process = xsize * ysize;
    ULONG  next_x = 0;
    ULONG  next_y = 0;
    ULONG  current_x, current_y, tocopy_w, tocopy_h;

    LOCK_PIXBUF

    while (pixels_left_to_process)
    {
        LONG srcx, srcy, dstx, dsty;
        LONG x, y;

        current_x = next_x;
        current_y = next_y;

        if (NUMPIX < xsize)
        {
           /* buffer can't hold a single horizontal line, and must
              divide each line into copies */
            tocopy_w = xsize - current_x;
            if (tocopy_w > NUMPIX)
            {
                /* Not quite finished with current horizontal pixel line */
                tocopy_w = NUMPIX;
                next_x += NUMPIX;
            }
            else
            {   /* Start at a new line */
            
                next_x = 0;
                next_y ++;
            }
            tocopy_h = 1;
        }
        else
        {
            tocopy_h = MIN(NUMPIX / xsize, ysize - current_y);
            tocopy_w = xsize;

            next_x = 0;
            next_y += tocopy_h;
        }

        srcx = x_src  + current_x;
        srcy = y_src  + current_y;
        dstx = x_dest + current_x;
        dsty = y_dest + current_y;

        /* Get some more pixels from the HIDD */
        HIDD_BM_GetImage(bm_obj, (UBYTE *)pixbuf, tocopy_w,
                         srcx, srcy, tocopy_w, tocopy_h, vHidd_StdPixFmt_Native32);

        /*  Write pixels to the destination */
        for (y = 0; y < tocopy_h; y ++)
        {
            for (x = 0; x < tocopy_w; x ++)
            {
                if (*pixbuf ++ == ecrd->pixel)
                {
                    ULONG i;

                    /* Set the according bit in the bitmap */
                    for (i = 0; i < bm->Depth; i++)
                    {
                        UBYTE *plane = bm->Planes[i];

                        if (NULL != plane)
                        {
                            UBYTE mask = XCOORD_TO_MASK(x + dstx);

                            plane += COORD_TO_BYTEIDX(x + dstx, y + dsty, bm->BytesPerRow);
                            /* Set the pixel */
                            *plane |= mask;
                        } /* if (plane allocated) */
                    } /* for (plane) */
                } /* if (color match) */
            } /* for (x) */
        } /* for (y) */

        pixels_left_to_process -= (tocopy_w * tocopy_h);
    }

    ULOCK_PIXBUF

    return 1;
}
