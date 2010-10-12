/*
 * Copyright 2009 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nouveau_intern.h"
#include "nouveau/nouveau_class.h"

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes lock on GART object is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HiddNouveauNVAccelUploadM2MF(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, UBYTE * pixels, ULONG x, ULONG y, 
    ULONG width, ULONG height, ULONG srcpitch)
{
    struct nouveau_channel *chan = carddata->chan;
    struct nouveau_grobj *m2mf = carddata->NvMemFormat;
    struct nouveau_bo *bo = bmdata->bo;
    unsigned cpp = bmdata->bytesperpixel;
    unsigned line_len = width * cpp;
    unsigned dst_offset = 0, dst_pitch = 0, linear = 0;
    char *src = (char *)pixels;

//    if (!nv50_style_tiled_pixmap(pdpix)) {
        linear     = 1;
        dst_pitch  = bmdata->pitch;
        dst_offset += (y * dst_pitch) + (x * cpp);
//    }

    while (height) {
        int line_count, i;
        char *dst;

        /* Determine max amount of data we can DMA at once */
        if (height * line_len <= carddata->GART->size) {
            line_count = height;
        } else {
            line_count = carddata->GART->size / line_len;
            if (line_count > height)
                line_count = height;
        }

        /* HW limitations */
        if (line_count > 2047)
            line_count = 2047;

        /* Upload to GART */
        if (nouveau_bo_map(carddata->GART, NOUVEAU_BO_WR))
            return FALSE;
        dst = carddata->GART->map;
        if (srcpitch == line_len) {
            memcpy(dst, src, srcpitch * line_count);
            src += srcpitch * line_count;
        } else {
            for (i = 0; i < line_count; i++) {
                memcpy(dst, src, line_len);
                src += srcpitch;
                dst += line_len;
            }
        }
        nouveau_bo_unmap(carddata->GART);

        if (MARK_RING(chan, 32, 6))
            return FALSE;

        BEGIN_RING(chan, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_IN, 2);
        if (OUT_RELOCo(chan, carddata->GART, NOUVEAU_BO_GART |
                   NOUVEAU_BO_RD) ||
            OUT_RELOCo(chan, bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
                   NOUVEAU_BO_WR)) {
            MARK_UNDO(chan);
            return FALSE;
        }

        if (carddata->architecture >= NV_ARCH_50) {
            BEGIN_RING(chan, m2mf, NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_IN, 1);
            OUT_RING  (chan, 1);

            if (!linear) {
                BEGIN_RING(chan, m2mf, NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_OUT, 7);
                OUT_RING  (chan, 0);
                OUT_RING  (chan, bo->tile_mode << 4);
                OUT_RING  (chan, bmdata->width * cpp);
                OUT_RING  (chan, bmdata->height);
                OUT_RING  (chan, 1);
                OUT_RING  (chan, 0);
                OUT_RING  (chan, (y << 16) | (x * cpp));
            } else {
                BEGIN_RING(chan, m2mf, NV50_MEMORY_TO_MEMORY_FORMAT_LINEAR_OUT, 1);
                OUT_RING  (chan, 1);
            }

            BEGIN_RING(chan, m2mf, NV50_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN_HIGH, 2);
            if (OUT_RELOCh(chan, carddata->GART, 0, NOUVEAU_BO_GART |
                       NOUVEAU_BO_RD) ||
                OUT_RELOCh(chan, bo, dst_offset, NOUVEAU_BO_VRAM |
                       NOUVEAU_BO_GART | NOUVEAU_BO_WR)) {
                MARK_UNDO(chan);
                return FALSE;
            }
        }

        /* DMA to VRAM */
        BEGIN_RING(chan, m2mf,
               NV04_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN, 8);
        if (OUT_RELOCl(chan, carddata->GART, 0, NOUVEAU_BO_GART |
                   NOUVEAU_BO_RD) ||
            OUT_RELOCl(chan, bo, dst_offset, NOUVEAU_BO_VRAM |
                   NOUVEAU_BO_GART | NOUVEAU_BO_WR)) {
            MARK_UNDO(chan);
            return FALSE;
        }
        OUT_RING  (chan, line_len);
        OUT_RING  (chan, dst_pitch);
        OUT_RING  (chan, line_len);
        OUT_RING  (chan, line_count);
        OUT_RING  (chan, (1<<8)|1);
        OUT_RING  (chan, 0);
        FIRE_RING (chan);

        if (linear)
            dst_offset += line_count * dst_pitch;
        height -= line_count;
        y += line_count;
    }

    return TRUE;
}

