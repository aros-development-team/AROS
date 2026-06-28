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

#include "hwdefs/nv_object.xml.h"
#include "hwdefs/nv_m2mf.xml.h"
#include "hwdefs/nv01_2d.xml.h"
#include "nv04_accel.h"

static void 
NV04EXASetPattern(NVPtr pNv, CARD32 clr0, CARD32 clr1, CARD32 pat0, CARD32 pat1)
{
	struct nouveau_pushbuf *push = pNv->pushbuf;

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvImagePattern->handle);
	BEGIN_NV04(push, NV01_PATT(MONOCHROME_COLOR(0)), 4);
	PUSH_DATA (push, clr0);
	PUSH_DATA (push, clr1);
	PUSH_DATA (push, pat0);
	PUSH_DATA (push, pat1);
}

static Bool
NV04EXASetROP(PixmapPtr ppix, int subc, int mthd, int alu, Pixel planemask)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(ppix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (ppix->drawable.bitsPerPixel < 32)
		planemask |= ~0 << ppix->drawable.bitsPerPixel;
	if (planemask != ~0 || alu != GXcopy) {
		if (ppix->drawable.bitsPerPixel == 32)
			return FALSE;
		if (planemask != ~0) {
			NV04EXASetPattern(pNv, 0, planemask, ~0, ~0);
			if (pNv->currentRop != (alu + 32)) {
				BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
				PUSH_DATA (push, pNv->NvRop->handle);
				BEGIN_NV04(push, NV01_ROP(ROP), 1);
				PUSH_DATA (push, NVROP[alu].copy_planemask);
				pNv->currentRop = alu + 32;
			}
		} else
		if (pNv->currentRop != alu) {
			if(pNv->currentRop >= 16)
				NV04EXASetPattern(pNv, ~0, ~0, ~0, ~0);
			BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
			PUSH_DATA (push, pNv->NvRop->handle);
			BEGIN_NV04(push, NV01_ROP(ROP), 1);
			PUSH_DATA (push, NVROP[alu].copy);
			pNv->currentRop = alu;
		}

		BEGIN_NV04(push, subc, mthd, 1);
		PUSH_DATA (push, 1); /* ROP_AND */
	} else {
		BEGIN_NV04(push, subc, mthd, 1);
		PUSH_DATA (push, 3); /* SRCCOPY */
	}

	return TRUE;
}

Bool
NV04EXAPrepareSolid(PixmapPtr ppix, int alu, Pixel planemask, Pixel fg)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(ppix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
	unsigned pitch = exaGetPixmapPitch(ppix);
	unsigned surf_fmt, rect_fmt;

	/* When SURFACE_FORMAT_A8R8G8B8 is used with GDI_RECTANGLE_TEXT, the 
	 * alpha channel gets forced to 0xFF for some reason.  We're using 
	 * SURFACE_FORMAT_Y32 as a workaround
	 */
	if (!NVAccelGetCtxSurf2DFormatFromPixmap(ppix, (int*)&surf_fmt))
		return FALSE;
	if (surf_fmt == NV04_SURFACE_2D_FORMAT_A8R8G8B8)
		surf_fmt = NV04_SURFACE_2D_FORMAT_Y32;

	rect_fmt = NV04_GDI_COLOR_FORMAT_A8R8G8B8;
	if (ppix->drawable.bitsPerPixel == 16) {
		if (ppix->drawable.depth == 16)
			rect_fmt = NV04_GDI_COLOR_FORMAT_A16R5G6B5;
		else
			rect_fmt = NV04_GDI_COLOR_FORMAT_X16A1R5G5B5;
	}

	if (!PUSH_SPACE(push, 64))
		return FALSE;
	PUSH_RESET(push);

	if (!NV04EXASetROP(ppix, NV04_RECT(OPERATION), alu, planemask))
		return FALSE;

	BEGIN_NV04(push, NV04_SF2D(FORMAT), 4);
	PUSH_DATA (push, surf_fmt);
	PUSH_DATA (push, (pitch << 16) | pitch);
	PUSH_MTHDl(push, NV04_SF2D(OFFSET_SOURCE), bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	PUSH_MTHDl(push, NV04_SF2D(OFFSET_DESTIN), bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_NV04(push, NV04_RECT(COLOR_FORMAT), 1);
	PUSH_DATA (push, rect_fmt);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		return FALSE;
	}

#if !defined(__AROS__)
	pNv->fg_colour = fg;
#endif
	return TRUE;
}

void
NV04EXASolid (PixmapPtr pPixmap, int x, int y, int x2, int y2, Pixel fg)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pPixmap->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	int w = x2 - x;
	int h = y2 - y;

	if (!PUSH_SPACE(push, 5))
		return;

	BEGIN_NV04(push, NV04_RECT(COLOR1_A), 1);
#if !defined(__AROS__)
	PUSH_DATA (push, pNv->fg_colour);
#else
	PUSH_DATA (push, fg);
#endif
	BEGIN_NV04(push, NV04_RECT(UNCLIPPED_RECTANGLE_POINT(0)), 2);
	PUSH_DATA (push, (x << 16) | y);
	PUSH_DATA (push, (w << 16) | h);
#if !defined(__AROS__)
	if ((w * h) >= 512)
#endif
		PUSH_KICK(push);
}

void
NV04EXADoneSolid (PixmapPtr pPixmap)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pPixmap->drawable.pScreen);
	nouveau_pushbuf_bufctx(NVPTR(pScrn)->pushbuf, NULL);
}

Bool
NV04EXAPrepareCopy(PixmapPtr pspix, PixmapPtr pdpix, int dx, int dy,
		   int alu, Pixel planemask)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pspix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_bo *src_bo = nouveau_pixmap_bo(pspix);
	struct nouveau_bo *dst_bo = nouveau_pixmap_bo(pdpix);
	int surf_fmt;

	if (pspix->drawable.bitsPerPixel != pdpix->drawable.bitsPerPixel)
		return FALSE;

	if (!NVAccelGetCtxSurf2DFormatFromPixmap(pdpix, &surf_fmt))
		return FALSE;

	if (!PUSH_SPACE(push, 64))
		return FALSE;
	PUSH_RESET(push);

	if (!NV04EXASetROP(pdpix, NV01_BLIT(OPERATION), alu, planemask))
		return FALSE;

	BEGIN_NV04(push, NV04_SF2D(FORMAT), 4);
	PUSH_DATA (push, surf_fmt);
	PUSH_DATA (push, (exaGetPixmapPitch(pdpix) << 16) |
			  exaGetPixmapPitch(pspix));
	PUSH_MTHDl(push, NV04_SF2D(OFFSET_SOURCE), src_bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	PUSH_MTHDl(push, NV04_SF2D(OFFSET_DESTIN), dst_bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		return FALSE;
	}

#if !defined(__AROS__)
	pNv->pspix = pspix;
	pNv->pmpix = NULL;
	pNv->pdpix = pdpix;
#endif
	return TRUE;
}

void
NV04EXACopy(PixmapPtr pdpix, int srcX, int srcY, int dstX, int dstY,
	    int width, int height)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pdpix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	int split_dstY = NOUVEAU_ALIGN(dstY + 1, 64);
	int split_height = split_dstY - dstY;

	if (nouveau_pushbuf_space(push, 16, 2, 0))
		return;

#if !defined(__AROS__)
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
		struct nouveau_bo *dst_bo = nouveau_pixmap_bo(pdpix);
		unsigned dst_pitch = exaGetPixmapPitch(pdpix);

		BEGIN_NV04(push, NV01_BLIT(POINT_IN), 3);
		PUSH_DATA (push, (srcY << 16) | srcX);
		PUSH_DATA (push, (dstY << 16) | dstX);
		PUSH_DATA (push, (split_height  << 16) | width);
		BEGIN_NV04(push, NV04_SF2D(OFFSET_DESTIN), 1);
		PUSH_RELOC(push, dst_bo, split_dstY * dst_pitch,
				 NOUVEAU_BO_LOW, 0, 0);

		srcY += split_height;
		height -= split_height;
		dstY = 0;
		pNv->pmpix = pdpix;
	}
#endif

	BEGIN_NV04(push, NV01_BLIT(POINT_IN), 3);
	PUSH_DATA (push, (srcY << 16) | srcX);
	PUSH_DATA (push, (dstY << 16) | dstX);
	PUSH_DATA (push, (height  << 16) | width);

#if !defined(__AROS__)
	if (pNv->pmpix) {
		struct nouveau_bo *dst_bo = nouveau_pixmap_bo(pdpix);

		BEGIN_NV04(push, NV04_SF2D(OFFSET_DESTIN), 1);
		PUSH_RELOC(push, dst_bo, 0, NOUVEAU_BO_LOW, 0, 0);
		pNv->pmpix = NULL;
	}
#endif

#if !defined(__AROS__)
	if ((width * height) >= 512)
#endif
		PUSH_KICK(push);
}

#if !defined(__AROS__)
void
NV04EXADoneCopy(PixmapPtr pdpix)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pdpix->drawable.pScreen);
	nouveau_pushbuf_bufctx(NVPTR(pScrn)->pushbuf, NULL);
}

Bool
NV04EXAUploadIFC(ScrnInfoPtr pScrn, const char *src, int src_pitch,
		 PixmapPtr pdpix, int x, int y, int w, int h, int cpp)
{
	NVPtr pNv = NVPTR(pScrn);
	ScreenPtr pScreen = pdpix->drawable.pScreen;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pdpix);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	int line_len = w * cpp;
	int surf_fmt, ifc_fmt;
	int iw, id, py, ph;
	int padbytes;
	Bool ret = FALSE;

	if (pNv->Architecture >= NV_TESLA)
		return FALSE;

	if (h > 1024)
		return FALSE;

	if (line_len < 4)
		return FALSE;

	switch (cpp) {
	case 2: ifc_fmt = 1; break;
	case 4: ifc_fmt = 4; break;
	default:
		return FALSE;
	}

	if (!NVAccelGetCtxSurf2DFormatFromPixmap(pdpix, &surf_fmt))
		return FALSE;

	/* Pad out input width to cover both COLORA() and COLORB() */
	iw  = (line_len + 7) & ~7;
	padbytes = iw - line_len;
	id  = iw / 4; /* line push size */
	iw /= cpp;

	/* Don't support lines longer than max push size yet.. */
	if (id > 1792)
		return FALSE;

	if (!PUSH_SPACE(push, 16))
		return FALSE;
	PUSH_RESET(push);

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvClipRectangle->handle);
	BEGIN_NV04(push, NV01_CLIP(POINT), 2);
	PUSH_DATA (push, (y << 16) | x);
	PUSH_DATA (push, (h << 16) | w);

	BEGIN_NV04(push, NV04_SF2D(FORMAT), 4);
	PUSH_DATA (push, surf_fmt);
	PUSH_DATA (push, (exaGetPixmapPitch(pdpix) << 16) |
			  exaGetPixmapPitch(pdpix));
	PUSH_MTHDl(push, NV04_SF2D(OFFSET_SOURCE), bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	PUSH_MTHDl(push, NV04_SF2D(OFFSET_DESTIN), bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push))
		goto out;

	py = y;
	ph = h;
	while (ph--) {
		if (PUSH_AVAIL(push) < id + 1 || (py == y)) {
			if (!PUSH_SPACE(push, id + 8))
				goto out;
			BEGIN_NV04(push, NV01_IFC(OPERATION), 2);
			PUSH_DATA (push, NV01_IFC_OPERATION_SRCCOPY);
			PUSH_DATA (push, ifc_fmt);
			BEGIN_NV04(push, NV01_IFC(POINT), 3);
			PUSH_DATA (push, (py << 16) | x);
			PUSH_DATA (push, (h << 16) | w);
			PUSH_DATA (push, (h << 16) | iw);
		}

		/* send a line */
		if (ph > 0 || !padbytes) {
			BEGIN_NV04(push, NV01_IFC(COLOR(0)), id);
			PUSH_DATAp(push, src, id);
		} else {
			char padding[8];
			int aux = (padbytes + 7) >> 2;
			memcpy(padding, src + (id - aux) * 4, padbytes);
			BEGIN_NV04(push, NV01_IFC(COLOR(0)), id);
			PUSH_DATAp(push, src, id - aux);
			PUSH_DATAp(push, padding, aux);
		}

		src += src_pitch;
		py++;
	}

	ret = TRUE;
out:
	nouveau_pushbuf_bufctx(push, NULL);
	if (pdpix == pScreen->GetScreenPixmap(pScreen))
		PUSH_KICK(push);
	return ret;
}
#endif

Bool
NV04EXARectM2MF(NVPtr pNv, int w, int h, int cpp,
		struct nouveau_bo *src, uint32_t src_off, int src_dom,
		int src_pitch, int src_h, int src_x, int src_y,
		struct nouveau_bo *dst, uint32_t dst_off, int dst_dom,
		int dst_pitch, int dst_h, int dst_x, int dst_y)
{
	struct nv04_fifo *fifo = pNv->channel->data;
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_pushbuf_refn refs[] = {
		{ src, src_dom | NOUVEAU_BO_RD },
		{ dst, dst_dom | NOUVEAU_BO_WR },
	};

	src_off += src_y * src_pitch + src_x * cpp;
	dst_off += dst_y * dst_pitch + dst_x * cpp;

	while (h) {
		int line_count = h;
		if (line_count > 2047)
			line_count = 2047;
		h -= line_count;

		if (nouveau_pushbuf_space(push, 16, 4, 0) ||
		    nouveau_pushbuf_refn (push, refs, 2))
			return FALSE;

		BEGIN_NV04(push, NV03_M2MF(DMA_BUFFER_IN), 2);
		PUSH_RELOC(push, src, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
		PUSH_RELOC(push, dst, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
		BEGIN_NV04(push, NV03_M2MF(OFFSET_IN), 8);
		PUSH_RELOC(push, src, src_off, NOUVEAU_BO_LOW, 0, 0);
		PUSH_RELOC(push, dst, dst_off, NOUVEAU_BO_LOW, 0, 0);
		PUSH_DATA (push, src_pitch);
		PUSH_DATA (push, dst_pitch);
		PUSH_DATA (push, w * cpp);
		PUSH_DATA (push, line_count);
		PUSH_DATA (push, 0x00000101);
		PUSH_DATA (push, 0x00000000);
		BEGIN_NV04(push, NV04_GRAPH(M2MF, NOP), 1);
		PUSH_DATA (push, 0x00000000);
		BEGIN_NV04(push, NV03_M2MF(OFFSET_OUT), 1);
		PUSH_DATA (push, 0x00000000);

		src_off += src_pitch * line_count;
		dst_off += dst_pitch * line_count;
	}

	return TRUE;
}

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
        NV04EXASolid(bmdata, minX, minY, maxX + 1, maxY + 1, color);
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
