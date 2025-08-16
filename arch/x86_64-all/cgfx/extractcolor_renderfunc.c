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

#include <emmintrin.h>

ULONG extcol_render(struct extcol_render_data *ecrd, LONG x_src, LONG y_src,
                           OOP_Object *bm_obj, OOP_Object *dst_gc,
                           struct Rectangle *rect, struct GfxBase *GfxBase)
{
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
            tocopy_w = xsize - current_x;
            if (tocopy_w > NUMPIX)
            {
                tocopy_w = NUMPIX;
                next_x += NUMPIX;
            }
            else
            {
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

        /* Fetch pixel block */
        HIDD_BM_GetImage(bm_obj, (UBYTE *)pixbuf, tocopy_w,
                         srcx, srcy, tocopy_w, tocopy_h, vHidd_StdPixFmt_Native32);

        /* Prepare SSE broadcast of the target pixel */
        __m128i cmpval = _mm_set1_epi32(ecrd->pixel);

        for (y = 0; y < tocopy_h; y++)
        {
            LONG x_aligned = 0;
            LONG width = tocopy_w;

            /* Process 4 pixels per iteration */
            for (; x_aligned + 3 < width; x_aligned += 4)
            {
                __m128i px = _mm_loadu_si128((__m128i const*)(pixbuf + x_aligned));
                __m128i eq = _mm_cmpeq_epi32(px, cmpval);
                int mask   = _mm_movemask_ps(_mm_castsi128_ps(eq));

                if (mask)
                {
                    /* At least one pixel matched */
                    for (int i = 0; i < 4; i++)
                    {
                        if (mask & (1 << i))
                        {
                            ULONG px_x = dstx + x_aligned + i;
                            ULONG px_y = dsty + y;

                            for (ULONG planeIdx = 0; planeIdx < bm->Depth; planeIdx++)
                            {
                                UBYTE *plane = bm->Planes[planeIdx];
                                if (plane)
                                {
                                    UBYTE maskb = XCOORD_TO_MASK(px_x);
                                    plane += COORD_TO_BYTEIDX(px_x, px_y, bm->BytesPerRow);
                                    *plane |= maskb;
                                }
                            }
                        }
                    }
                }
            }

            /* Process remainder pixels */
            for (; x_aligned < width; x_aligned++)
            {
                if (pixbuf[x_aligned] == ecrd->pixel)
                {
                    ULONG px_x = dstx + x_aligned;
                    ULONG px_y = dsty + y;

                    for (ULONG planeIdx = 0; planeIdx < bm->Depth; planeIdx++)
                    {
                        UBYTE *plane = bm->Planes[planeIdx];
                        if (plane)
                        {
                            UBYTE maskb = XCOORD_TO_MASK(px_x);
                            plane += COORD_TO_BYTEIDX(px_x, px_y, bm->BytesPerRow);
                            *plane |= maskb;
                        }
                    }
                }
            }

            pixbuf += tocopy_w;
        }

        pixels_left_to_process -= (tocopy_w * tocopy_h);
    }

    ULOCK_PIXBUF
    return 1;
}
