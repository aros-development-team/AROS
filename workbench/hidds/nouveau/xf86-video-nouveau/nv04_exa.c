/*
 * Copyright 2003 NVIDIA, Corporation
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

#include "nv_include.h"
#include "nv_rop.h"

#include "nv04_pushbuf.h"

static void 
NV04EXASetPattern(ScrnInfoPtr pScrn, CARD32 clr0, CARD32 clr1,
		  CARD32 pat0, CARD32 pat1)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *patt = pNv->NvImagePattern;

	BEGIN_RING(chan, patt, NV04_IMAGE_PATTERN_MONOCHROME_COLOR0, 4);
	OUT_RING  (chan, clr0);
	OUT_RING  (chan, clr1);
	OUT_RING  (chan, pat0);
	OUT_RING  (chan, pat1);
}

#if !defined(__AROS__)
static void 
NV04EXASetROP(ScrnInfoPtr pScrn, CARD32 alu, CARD32 planemask)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *rop = pNv->NvRop;
	
	if (planemask != ~0) {
		NV04EXASetPattern(pScrn, 0, planemask, ~0, ~0);
		if (pNv->currentRop != (alu + 32)) {
			BEGIN_RING(chan, rop, NV03_CONTEXT_ROP_ROP, 1);
			OUT_RING  (chan, NVROP[alu].copy_planemask);
			pNv->currentRop = alu + 32;
		}
	} else
	if (pNv->currentRop != alu) {
		if(pNv->currentRop >= 16)
			NV04EXASetPattern(pScrn, ~0, ~0, ~0, ~0);
		BEGIN_RING(chan, rop, NV03_CONTEXT_ROP_ROP, 1);
		OUT_RING  (chan, NVROP[alu].copy);
		pNv->currentRop = alu;
	}
}
#else
static void 
NV04EXASetROP(ScrnInfoPtr pScrn, CARD32 alu, CARD32 planemask)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *rop = pNv->NvRop;
	
	BEGIN_RING(chan, rop, NV03_CONTEXT_ROP_ROP, 1);
	OUT_RING  (chan, NVROP[alu].copy);
}
#endif

#if !defined(__AROS__)
static void
NV04EXAStateSolidResubmit(struct nouveau_channel *chan)
{
	ScrnInfoPtr pScrn = chan->user_private;
	NVPtr pNv = NVPTR(pScrn);

	NV04EXAPrepareSolid(pNv->pdpix, pNv->alu, pNv->planemask, pNv->fg_colour);
}
#endif

Bool
NV04EXAPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg)
{
#if !defined(__AROS__)
	ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
#else
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *surf2d = pNv->NvContextSurfaces;
	struct nouveau_grobj *rect = pNv->NvRectangle;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pPixmap);
	unsigned int fmt, pitch, fmt2 = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;

	if (MARK_RING(chan, 64, 2))
		return FALSE;

#if !defined(__AROS__)
	planemask |= ~0 << pPixmap->drawable.bitsPerPixel;
	if (planemask != ~0 || alu != GXcopy) {
		if (pPixmap->drawable.bitsPerPixel == 32)
			return FALSE;
#else
	if (alu != 0x03 /* DrawMode_Copy */) {
#endif
		BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
		OUT_RING  (chan, 1); /* ROP_AND */
		NV04EXASetROP(pScrn, alu, planemask);
	} else {
		BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
		OUT_RING  (chan, 3); /* SRCCOPY */
	}

	if (!NVAccelGetCtxSurf2DFormatFromPixmap(pPixmap, (int*)&fmt))
		return FALSE;
	pitch = exaGetPixmapPitch(pPixmap);

#if !defined(__AROS__)
	if (pPixmap->drawable.bitsPerPixel == 16) {
		if (pPixmap->drawable.depth == 16) {
			fmt2 = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;
		} else if (pPixmap->drawable.depth == 15) {
			fmt2 = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_X16A1R5G5B5;
		}
	}
#else
	if (pPixmap->depth == 16)
		fmt2 = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;
#endif

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
	OUT_RING (chan, fg);

#if !defined(__AROS__)
	pNv->pdpix = pPixmap;
	pNv->alu = alu;
	pNv->planemask = planemask;
	pNv->fg_colour = fg;
	chan->flush_notify = NV04EXAStateSolidResubmit;
#else
	chan->flush_notify = NULL;
#endif
	return TRUE;
}

void
NV04EXASolid (PixmapPtr pPixmap, int x1, int y1, int x2, int y2)
{
#if !defined(__AROS__)
	ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
#else
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *rect = pNv->NvRectangle;
	int width = x2-x1;
	int height = y2-y1;

	BEGIN_RING(chan, rect,
		   NV04_GDI_RECTANGLE_TEXT_UNCLIPPED_RECTANGLE_POINT(0), 2);
	OUT_RING  (chan, (x1 << 16) | y1);
	OUT_RING  (chan, (width << 16) | height);

#if !defined(__AROS__)
	if((width * height) >= 512)
#endif
		FIRE_RING (chan);
}

#if !defined(__AROS__)
void
NV04EXADoneSolid (PixmapPtr pPixmap)
{
	ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
	NVPtr pNv = NVPTR(pScrn);

	pNv->chan->flush_notify = NULL;
}

static void
NV04EXAStateCopyResubmit(struct nouveau_channel *chan)
{
	ScrnInfoPtr pScrn = chan->user_private;
	NVPtr pNv = NVPTR(pScrn);

	NV04EXAPrepareCopy(pNv->pspix, pNv->pdpix, 0, 0, pNv->alu,
			   pNv->planemask);
}
#endif

Bool
NV04EXAPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap, int dx, int dy,
		   int alu, Pixel planemask)
{
#if !defined(__AROS__)
	ScrnInfoPtr pScrn = xf86Screens[pSrcPixmap->drawable.pScreen->myNum];
#else
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *surf2d = pNv->NvContextSurfaces;
	struct nouveau_grobj *blit = pNv->NvImageBlit;
	struct nouveau_bo *src_bo = nouveau_pixmap_bo(pSrcPixmap);
	struct nouveau_bo *dst_bo = nouveau_pixmap_bo(pDstPixmap);
	int fmt;

#if !defined(__AROS__)
	if (pSrcPixmap->drawable.bitsPerPixel !=
	    pDstPixmap->drawable.bitsPerPixel)
#else
	if (pSrcPixmap->depth != pDstPixmap->depth)
#endif
		return FALSE;

	if (!NVAccelGetCtxSurf2DFormatFromPixmap(pDstPixmap, &fmt))
		return FALSE;

	if (MARK_RING(chan, 64, 2))
		return FALSE;

#if !defined(__AROS__)
	planemask |= ~0 << pDstPixmap->drawable.bitsPerPixel;
	if (planemask != ~0 || alu != GXcopy) {
		if (pDstPixmap->drawable.bitsPerPixel == 32) {
			MARK_UNDO(chan);
			return FALSE;
		}
#else
	if (alu != 0x03 /* DrawMode_Copy */) {
#endif

		BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_OPERATION, 1);
		OUT_RING  (chan, 1); /* ROP_AND */

		NV04EXASetROP(pScrn, alu, planemask);
	} else {
		BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_OPERATION, 1);
		OUT_RING  (chan, 3); /* SRCCOPY */
	}

	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, fmt);
	OUT_RING  (chan, (exaGetPixmapPitch(pDstPixmap) << 16) |
		   (exaGetPixmapPitch(pSrcPixmap)));
	if (OUT_RELOCl(chan, src_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD) ||
	    OUT_RELOCl(chan, dst_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}

#if !defined(__AROS__)
	pNv->pspix = pSrcPixmap;
	pNv->pdpix = pDstPixmap;
	pNv->alu = alu;
	pNv->planemask = planemask;
	chan->flush_notify = NV04EXAStateCopyResubmit;
#else
	chan->flush_notify = NULL;
#endif
	return TRUE;
}

void
NV04EXACopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY,
	    int width, int height)
{
#if !defined(__AROS__)
	ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
#else
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *blit = pNv->NvImageBlit;
#if !defined(__AROS__)
	int split_dstY = NOUVEAU_ALIGN(dstY + 1, 64);
	int split_height = split_dstY - dstY;

	if ((width * height) >= 200000 && pNv->pspix != pNv->pdpix &&
	    (dstY > srcY || dstX > srcX) && split_height < height) {
		/*
		 * KLUDGE - Split the destination rectangle in an
		 * upper misaligned half and a lower tile-aligned
		 * half, then get IMAGE_BLIT to blit the lower piece
		 * downwards (required for sync-to-vblank if the area
		 * to be blitted is large enough). The blob does a
		 * different (not nicer) trick to achieve the same
		 * effect.
		 */
		struct nouveau_grobj *surf2d = pNv->NvContextSurfaces;
		struct nouveau_bo *dst_bo = nouveau_pixmap_bo(pNv->pdpix);
		unsigned dst_pitch = exaGetPixmapPitch(pNv->pdpix);

		if (MARK_RING(chan, 10, 1))
			return;

		BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_POINT_IN, 3);
		OUT_RING  (chan, (srcY << 16) | srcX);
		OUT_RING  (chan, (dstY << 16) | dstX);
		OUT_RING  (chan, (split_height  << 16) | width);

		BEGIN_RING(chan, surf2d,
			   NV04_CONTEXT_SURFACES_2D_OFFSET_DESTIN, 1);
		OUT_RELOCl(chan, dst_bo, split_dstY * dst_pitch,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

		srcY += split_height;
		height -= split_height;
		dstY = 0;
	}
#endif

	BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_POINT_IN, 3);
	OUT_RING  (chan, (srcY << 16) | srcX);
	OUT_RING  (chan, (dstY << 16) | dstX);
	OUT_RING  (chan, (height  << 16) | width);

#if !defined(__AROS__)
	if((width * height) >= 512)
#endif
		FIRE_RING (chan);
}

#if !defined(__AROS__)
void
NV04EXADoneCopy(PixmapPtr pDstPixmap)
{
	ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
	NVPtr pNv = NVPTR(pScrn);

	pNv->chan->flush_notify = NULL;
}

static Bool
NV04EXAStateIFCSubmit(struct nouveau_channel *chan)
{
	ScrnInfoPtr pScrn = chan->user_private;
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_grobj *surf2d = pNv->NvContextSurfaces;
	struct nouveau_grobj *ifc = pNv->NvImageFromCpu;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pNv->pdpix);
	int surf_fmt;

	NVAccelGetCtxSurf2DFormatFromPixmap(pNv->pdpix, &surf_fmt);

	if (MARK_RING(chan, 64, 2))
		return FALSE;

	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, surf_fmt);
	OUT_RING  (chan, (exaGetPixmapPitch(pNv->pdpix) << 16) |
			  exaGetPixmapPitch(pNv->pdpix));
	if (OUT_RELOCl(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR)) {
		MARK_UNDO(chan);
		return FALSE;
	}
	BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_POINT, 3);
	OUT_RING  (chan, (pNv->point_y << 16) | pNv->point_x);
	OUT_RING  (chan, (pNv->height_out << 16) | pNv->width_out);
	OUT_RING  (chan, (pNv->height_in << 16) | pNv->width_in);

	return TRUE;
}

static void
NV04EXAStateIFCResubmit(struct nouveau_channel *chan)
{
	NV04EXAStateIFCSubmit(chan);
}

Bool
NV04EXAUploadIFC(ScrnInfoPtr pScrn, const char *src, int src_pitch,
		 PixmapPtr pDst, int x, int y, int w, int h, int cpp)
{
	NVPtr pNv = NVPTR(pScrn);
	ScreenPtr pScreen = pDst->drawable.pScreen;
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *clip = pNv->NvClipRectangle;
	struct nouveau_grobj *ifc = pNv->NvImageFromCpu;
	int line_len = w * cpp;
	int iw, id, surf_fmt, ifc_fmt;
	int padbytes;

	if (pNv->Architecture >= NV_ARCH_50)
		return FALSE;

	if (h > 1024)
		return FALSE;

	if (line_len<4)
		return FALSE;

	switch (cpp) {
	case 2: ifc_fmt = 1; break;
	case 4: ifc_fmt = 4; break;
	default:
		return FALSE;
	}

	if (!NVAccelGetCtxSurf2DFormatFromPixmap(pDst, &surf_fmt))
		return FALSE;

	/* Pad out input width to cover both COLORA() and COLORB() */
	iw  = (line_len + 7) & ~7;
	padbytes = iw - line_len;
	id  = iw / 4; /* line push size */
	iw /= cpp;

	/* Don't support lines longer than max push size yet.. */
	if (id > 1792)
		return FALSE;

	BEGIN_RING(chan, clip, NV01_CONTEXT_CLIP_RECTANGLE_POINT, 2);
	OUT_RING  (chan, (y << 16) | x);
	OUT_RING  (chan, (h << 16) | w);
	BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_OPERATION, 2);
	OUT_RING  (chan, NV01_IMAGE_FROM_CPU_OPERATION_SRCCOPY);
	OUT_RING  (chan, ifc_fmt);

	pNv->point_x = x;
	pNv->point_y = y;
	pNv->height_in = pNv->height_out = h;
	pNv->width_in = iw;
	pNv->width_out = w;
	pNv->pdpix = pDst;
	chan->flush_notify = NV04EXAStateIFCResubmit;
	if (!NV04EXAStateIFCSubmit(chan))
		return FALSE;

	if (padbytes)
		h--;
	while (h--) {
		/* send a line */
		BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_COLOR(0), id);
		OUT_RINGp (chan, src, id);

		src += src_pitch;
		pNv->point_y++;
	}
	if (padbytes) {
		char padding[8];
		int aux = (padbytes + 7) >> 2;
		BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_COLOR(0), id);
		OUT_RINGp (chan, src, id - aux);
		memcpy(padding, src + (id - aux) * 4, padbytes);
		OUT_RINGp (chan, padding, aux);
	}

	chan->flush_notify = NULL;

	if (pDst == pScreen->GetScreenPixmap(pScreen))
		FIRE_RING(chan);
	return TRUE;
}
#endif

/* AROS CODE */

VOID HIDDNouveauNV04SetPattern(struct CardData * carddata, LONG clr0, LONG clr1,
		  LONG pat0, LONG pat1)
{
    NV04EXASetPattern(carddata, clr0, clr1, pat0, pat1);
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNV04FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, LONG minX, LONG minY, LONG maxX,
    LONG maxY, ULONG drawmode, ULONG color)
{
    if (NV04EXAPrepareSolid(bmdata, drawmode, ~0, color))
    {
        NV04EXASolid(bmdata, minX, minY, maxX + 1, maxY + 1);
        return TRUE;
    }
    
    return FALSE;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNV04CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG drawmode)
{
    if (NV04EXAPrepareCopy(srcdata, destdata, 0, 0, drawmode, ~0))
    {
        NV04EXACopy(destdata, srcX, srcY, destX, destY, width, height);
        return TRUE;
    }

    return FALSE;
}
