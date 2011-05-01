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

static BOOL HIDDNouveauNV502DSurfaceFormat(struct HIDDNouveauBitMapData * bmdata, 
    uint32_t *fmt)
{
	switch (bmdata->bytesperpixel * 8) 
	{
	case 8 : *fmt = NV50_2D_SRC_FORMAT_R8_UNORM; break;
	case 15: *fmt = NV50_2D_SRC_FORMAT_X1R5G5B5_UNORM; break;
	case 16: *fmt = NV50_2D_SRC_FORMAT_R5G6B5_UNORM; break;
	case 24: *fmt = NV50_2D_SRC_FORMAT_X8R8G8B8_UNORM; break;
	case 30: *fmt = NV50_2D_SRC_FORMAT_A2B10G10R10_UNORM; break;
//FIXME	case 32: *fmt = NV50_2D_SRC_FORMAT_A8R8G8B8_UNORM; break;
	case 32: *fmt = NV50_2D_SRC_FORMAT_X8R8G8B8_UNORM; break;
	default:
		 return FALSE;
	}

	return TRUE;
}

static VOID HIDDNouveauNV50SetClip(struct CardData * carddata, LONG x, LONG y, 
    LONG w, LONG h)
{
    struct nouveau_channel * chan = carddata->chan;
    struct nouveau_grobj *eng2d = carddata->Nv2D;

	BEGIN_RING(chan, eng2d, NV50_2D_CLIP_X, 4);
	OUT_RING  (chan, x);
	OUT_RING  (chan, y);
	OUT_RING  (chan, w);
	OUT_RING  (chan, h);
}

static BOOL HIDDNouveauNV50AcquireSurface2D(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, BOOL issrc)
{
    struct nouveau_channel * chan = carddata->chan;
    struct nouveau_grobj *eng2d = carddata->Nv2D;
    struct nouveau_bo *bo = bmdata->bo;
    int mthd = issrc ? NV50_2D_SRC_FORMAT : NV50_2D_DST_FORMAT;
    uint32_t fmt, bo_flags;

    if (!HIDDNouveauNV502DSurfaceFormat(bmdata, &fmt))
        return FALSE;

    bo_flags  = NOUVEAU_BO_VRAM;
    bo_flags |= issrc ? NOUVEAU_BO_RD : NOUVEAU_BO_WR;

    if(!bo->tile_flags) {
        BEGIN_RING(chan, eng2d, mthd, 2);
        OUT_RING  (chan, fmt);
        OUT_RING  (chan, 1);
        BEGIN_RING(chan, eng2d, mthd + 0x14, 1);
        OUT_RING  (chan, (uint32_t)bmdata->pitch);
    } else {
        BEGIN_RING(chan, eng2d, mthd, 5);
        OUT_RING  (chan, fmt);
        OUT_RING  (chan, 0);
        OUT_RING  (chan, bo->tile_mode << 4);
        OUT_RING  (chan, 1);
        OUT_RING  (chan, 0);
    }

    BEGIN_RING(chan, eng2d, mthd + 0x18, 4);
    OUT_RING  (chan, bmdata->width);
    OUT_RING  (chan, bmdata->height);
    if (OUT_RELOCh(chan, bo, 0, bo_flags) ||
        OUT_RELOCl(chan, bo, 0, bo_flags))
    return FALSE;

    if (!issrc)
        HIDDNouveauNV50SetClip(carddata, 0, 0, bmdata->width, bmdata->height);

    return TRUE;
}

VOID HIDDNouveauNV50SetPattern(struct CardData * carddata, LONG col0, 
    LONG col1, LONG pat0, LONG pat1)
{
    struct nouveau_channel * chan = carddata->chan;
    struct nouveau_grobj *eng2d = carddata->Nv2D;

    BEGIN_RING(chan, eng2d, NV50_2D_PATTERN_COLOR(0), 4);
    OUT_RING  (chan, col0);
    OUT_RING  (chan, col1);
    OUT_RING  (chan, pat0);
    OUT_RING  (chan, pat1);
}

static VOID HIDDNouveauNV50SetROP(struct CardData * carddata, ULONG drawmode,
    struct HIDDNouveauBitMapData * bmdata) /*, Pixel planemask) */
{
    struct nouveau_channel * chan = carddata->chan;
    struct nouveau_grobj *eng2d = carddata->Nv2D;
    LONG rop;

//    if (planemask != ~0)
//        rop = NVROP[alu].copy_planemask;
//    else
//        rop = NVROP[alu].copy;
    rop = NVROP[drawmode].copy;

    BEGIN_RING(chan, eng2d, NV50_2D_OPERATION, 1);
//    if (alu == GXcopy && EXA_PM_IS_SOLID(&pdpix->drawable, planemask)) {
    if (drawmode == 0x03 /* DrawMode_Copy */) {
        OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY);
        return;
    } else {
        OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY_PREMULT);
    }

    BEGIN_RING(chan, eng2d, NV50_2D_PATTERN_FORMAT, 2);
    //	switch (pdpix->drawable.bitsPerPixel) {
    switch (bmdata->bytesperpixel * 8) { /* TODO: maybe use bmdata->depth? */
        case  8: OUT_RING  (chan, 3); break;
        case 15: OUT_RING  (chan, 1); break;
        case 16: OUT_RING  (chan, 0); break;
        case 24:
        case 32:
        default:
            OUT_RING  (chan, 2);
        break;
    }
    OUT_RING  (chan, 1);

    /* There are 16 alu's.
    * 0-15: copy
    * 16-31: copy_planemask
    */

//    if (!EXA_PM_IS_SOLID(&pdpix->drawable, planemask)) {
//        alu += 16;
//        NV50EXASetPattern(pdpix, 0, planemask, ~0, ~0);
//    } else {
//FIXME AROS currently does not have drawmodes above 15
//        if (pNv->currentRop > 15)
//        HIDDNouveauNV50SetPattern(carddata, ~0, ~0, ~0, ~0);
//    }

//    if (pNv->currentRop != alu) {
        BEGIN_RING(chan, eng2d, NV50_2D_ROP, 1);
        OUT_RING  (chan, rop);
//        pNv->currentRop = alu;
//    }
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNV50CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    ULONG srcX, ULONG srcY, ULONG destX, ULONG destY, ULONG width, ULONG height,
    ULONG drawmode)
{
    struct nouveau_channel * chan = carddata->chan;
    struct nouveau_grobj *eng2d = carddata->Nv2D;

    if (srcdata->bytesperpixel != destdata->bytesperpixel)
        return FALSE;

    /* Prepare copy */
    if (MARK_RING(chan, 64, 4))
        return FALSE;
    
    if (!HIDDNouveauNV50AcquireSurface2D(carddata, srcdata, TRUE))
    {
        MARK_UNDO(chan);
        return FALSE;
    }
    
    if (!HIDDNouveauNV50AcquireSurface2D(carddata, destdata, FALSE))
    {
        MARK_UNDO(chan);
        return FALSE;
    }
    
    /* Set raster operation */
    HIDDNouveauNV50SetROP(carddata, drawmode, destdata);
    
    /* TODO: State resubmit preparation. What does it do? */
    
    /* Execute copy */
    WAIT_RING (chan, 17);
    BEGIN_RING(chan, eng2d, 0x0110, 1);
    OUT_RING  (chan, 0);
    BEGIN_RING(chan, eng2d, 0x088c, 1);
    OUT_RING  (chan, 0);
    BEGIN_RING(chan, eng2d, NV50_2D_BLIT_DST_X, 12);
    OUT_RING  (chan, destX);
    OUT_RING  (chan, destY);
    OUT_RING  (chan, width);
    OUT_RING  (chan, height);
    OUT_RING  (chan, 0);
    OUT_RING  (chan, 1);
    OUT_RING  (chan, 0);
    OUT_RING  (chan, 1);
    OUT_RING  (chan, 0);
    OUT_RING  (chan, srcX);
    OUT_RING  (chan, 0);
    OUT_RING  (chan, srcY);

    FIRE_RING (chan);
    
    return TRUE;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNV50FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, ULONG minX, ULONG minY, ULONG maxX,
    ULONG maxY, ULONG drawmode, ULONG color)
{
    struct nouveau_channel * chan = carddata->chan;
    struct nouveau_grobj *eng2d = carddata->Nv2D;
    uint32_t fmt;

    /* Prepare solid fill */
    if (!HIDDNouveauNV502DSurfaceFormat(bmdata, &fmt))
        return FALSE;

    if (MARK_RING(chan, 64, 4))
        return FALSE;

    if (!HIDDNouveauNV50AcquireSurface2D(carddata, bmdata, FALSE)) 
    {
        MARK_UNDO(chan);
        return FALSE;
    }

    /* Set raster operation */
    HIDDNouveauNV50SetROP(carddata, drawmode, bmdata);

    BEGIN_RING(chan, eng2d, NV50_2D_DRAW_SHAPE, 3);
    OUT_RING  (chan, NV50_2D_DRAW_SHAPE_RECTANGLES);
    OUT_RING  (chan, fmt);
    OUT_RING  (chan, color);

    /* TODO: State resubmit preparation. What does it do? */

    /* Execute solid fill */
    WAIT_RING (chan, 5);
    BEGIN_RING(chan, eng2d, NV50_2D_DRAW_POINT32_X(0), 4);
    OUT_RING  (chan, minX);
    OUT_RING  (chan, minY);
    OUT_RING  (chan, maxX + 1);
    OUT_RING  (chan, maxY + 1);

    FIRE_RING (chan);

    return TRUE;
}
