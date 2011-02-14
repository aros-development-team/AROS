/*
 * Copyright 2003 NVIDIA, Corporation
 * Copyright 2010 The AROS Development Team. All rights reserved.
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
#include "nv_rop.h"
#include "nouveau_class.h"

VOID HIDDNouveauNV04SetPattern(struct CardData * carddata, ULONG clr0, ULONG clr1,
		  ULONG pat0, ULONG pat1)
{
    struct nouveau_channel *chan = carddata->chan;
    struct nouveau_grobj *patt = carddata->NvImagePattern;

    BEGIN_RING(chan, patt, NV04_IMAGE_PATTERN_MONOCHROME_COLOR0, 4);
    OUT_RING  (chan, clr0);
    OUT_RING  (chan, clr1);
    OUT_RING  (chan, pat0);
    OUT_RING  (chan, pat1);
}

static VOID HIDDNouveauNV04SetROP(struct CardData * carddata, ULONG drawmode)/*, CARD32 planemask)*/
{
    struct nouveau_channel *chan = carddata->chan;
    struct nouveau_grobj *rop = carddata->NvRop;

    /* TODO: Understand what planemask is used for */
//    if (planemask != ~0) {
//        NV04EXASetPattern(pScrn, 0, planemask, ~0, ~0);
//        if (pNv->currentRop != (alu + 32)) {
//            BEGIN_RING(chan, rop, NV03_CONTEXT_ROP_ROP, 1);
//            OUT_RING  (chan, NVROP[alu].copy_planemask);
//            pNv->currentRop = alu + 32;
//        }
//    } else
//    if (pNv->currentRop != alu) {
//FIXME AROS currently does not have drawmodes above 15
//        if(pNv->currentRop >= 16)
//            HIDDNouveauNV04SetPattern(carddata, ~0, ~0, ~0, ~0);
        BEGIN_RING(chan, rop, NV03_CONTEXT_ROP_ROP, 1);
        OUT_RING  (chan, NVROP[drawmode].copy);
//        pNv->currentRop = alu;
//    }
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNV04CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    ULONG srcX, ULONG srcY, ULONG destX, ULONG destY, ULONG width, ULONG height,
    ULONG drawmode)
{
    struct nouveau_channel * chan = carddata->chan;
    struct nouveau_bo * src_bo = srcdata->bo;
    struct nouveau_bo * dst_bo = destdata->bo;
    struct nouveau_grobj *surf2d = carddata->NvContextSurfaces;
    struct nouveau_grobj *blit = carddata->NvImageBlit;
    LONG fmt;

    if (srcdata->bytesperpixel != destdata->bytesperpixel)
        return FALSE;

    if (!NVAccelGetCtxSurf2DFormatFromPixmap(destdata, &fmt))
        return FALSE;
    
    /* Prepare copy */
    if (MARK_RING(chan, 64, 2))
        return FALSE;

    /* TODO: Understand what planemask is used for */
//  	planemask |= ~0 << pDstPixmap->drawable.bitsPerPixel;
//	if (planemask != ~0 || alu != GXcopy) {
//		if (pDstPixmap->drawable.bitsPerPixel == 32) {
//			MARK_UNDO(chan);
//			return FALSE;
//		}
    if (drawmode != 0x03 /* DrawMode_Copy */) {
        BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_OPERATION, 1);
        OUT_RING  (chan, 1); /* ROP_AND */

        HIDDNouveauNV04SetROP(carddata, drawmode);
    } else {
        BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_OPERATION, 1);
        OUT_RING  (chan, 3); /* SRCCOPY */
    }

    BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
    OUT_RING  (chan, fmt);
    OUT_RING  (chan, destdata->pitch << 16 | srcdata->pitch );
    if (OUT_RELOCl(chan, src_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD) ||
        OUT_RELOCl(chan, dst_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
        MARK_UNDO(chan);
        return FALSE;
    }

    /* TODO: State resubmit preparation. What does it do? */

    /* Execute copy */
    BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_POINT_IN, 3);
    OUT_RING  (chan, (srcY << 16) | srcX);
    OUT_RING  (chan, (destY << 16) | destX);
    OUT_RING  (chan, (height  << 16) | width);

    FIRE_RING (chan);

    return TRUE;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNV04FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, ULONG minX, ULONG minY, ULONG maxX,
    ULONG maxY, ULONG drawmode, ULONG color)
{
    struct nouveau_channel *chan = carddata->chan;
    struct nouveau_grobj *surf2d = carddata->NvContextSurfaces;
    struct nouveau_grobj *rect = carddata->NvRectangle;
    struct nouveau_bo *bo = bmdata->bo;
    LONG fmt;
    ULONG pitch, fmt2 = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;
	LONG width = maxX - minX + 1;
	LONG height = maxY - minY + 1;

    /* Prepare solid fill */
    if (MARK_RING(chan, 64, 2))
        return FALSE;

    /* TODO: Understand what planemask is used for */
//    planemask |= ~0 << pPixmap->drawable.bitsPerPixel;
//    if (planemask != ~0 || alu != GXcopy) {
//        if (pPixmap->drawable.bitsPerPixel == 32)
//            return FALSE;
    if (drawmode != 0x03 /* DrawMode_Copy */) {
        BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
        OUT_RING  (chan, 1); /* ROP_AND */
        HIDDNouveauNV04SetROP(carddata, drawmode);
    } else {
        BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
        OUT_RING  (chan, 3); /* SRCCOPY */
    }

    if (!NVAccelGetCtxSurf2DFormatFromPixmap(bmdata, &fmt))
        return FALSE;
    pitch = bmdata->pitch;

    if (bmdata->bytesperpixel * 8 == 16) {
        if (bmdata->depth == 16) {
            fmt2 = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;
        } else if (bmdata->depth == 15) {
            fmt2 = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_X16A1R5G5B5;
        }
    }

    /* When SURFACE_FORMAT_A8R8G8B8 is used with GDI_RECTANGLE_TEXT, the 
    * alpha channel gets forced to 0xFF for some reason.  We're using 
    * SURFACE_FORMAT_Y32 as a workaround
    */
    if (fmt == NV04_CONTEXT_SURFACES_2D_FORMAT_A8R8G8B8)
        fmt = NV04_CONTEXT_SURFACES_2D_FORMAT_Y32;

    BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
    OUT_RING  (chan, fmt);
    OUT_RING  (chan, (pitch << 16) | pitch);
    if (OUT_RELOCl(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
        OUT_RELOCl(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
        MARK_UNDO(chan);
        return FALSE;
    }

    BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT, 1);
    OUT_RING  (chan, fmt2);
    BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR1_A, 1);
    OUT_RING (chan, color);
	
    /* TODO: State resubmit preparation. What does it do? */
	
	/* Execute solid fill */
	BEGIN_RING(chan, rect,
		   NV04_GDI_RECTANGLE_TEXT_UNCLIPPED_RECTANGLE_POINT(0), 2);
	OUT_RING  (chan, (minX << 16) | minY);
	OUT_RING  (chan, (width << 16) | height);

    FIRE_RING (chan);

    return TRUE;
}
