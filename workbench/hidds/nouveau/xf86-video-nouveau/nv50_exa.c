/*
 * Copyright 2007 NVIDIA, Corporation
 * Copyright 2008 Ben Skeggs
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

#include "nv50_accel.h"

#define NV50EXA_LOCALS(p)                                                      \
	ScrnInfoPtr pScrn = xf86ScreenToScrn((p)->drawable.pScreen);         \
	NVPtr pNv = NVPTR(pScrn);                                              \
	struct nouveau_pushbuf *push = pNv->pushbuf; (void)push;

#define BF(f) NV50_BLEND_FACTOR_##f

struct nv50_blend_op {
	unsigned src_alpha;
	unsigned dst_alpha;
	unsigned src_blend;
	unsigned dst_blend;
};

static struct nv50_blend_op
NV50EXABlendOp[] = {
/* Clear       */ { 0, 0, BF(               ZERO), BF(               ZERO) },
/* Src         */ { 0, 0, BF(                ONE), BF(               ZERO) },
/* Dst         */ { 0, 0, BF(               ZERO), BF(                ONE) },
/* Over        */ { 1, 0, BF(                ONE), BF(ONE_MINUS_SRC_ALPHA) },
/* OverReverse */ { 0, 1, BF(ONE_MINUS_DST_ALPHA), BF(                ONE) },
/* In          */ { 0, 1, BF(          DST_ALPHA), BF(               ZERO) },
/* InReverse   */ { 1, 0, BF(               ZERO), BF(          SRC_ALPHA) },
/* Out         */ { 0, 1, BF(ONE_MINUS_DST_ALPHA), BF(               ZERO) },
/* OutReverse  */ { 1, 0, BF(               ZERO), BF(ONE_MINUS_SRC_ALPHA) },
/* Atop        */ { 1, 1, BF(          DST_ALPHA), BF(ONE_MINUS_SRC_ALPHA) },
/* AtopReverse */ { 1, 1, BF(ONE_MINUS_DST_ALPHA), BF(          SRC_ALPHA) },
/* Xor         */ { 1, 1, BF(ONE_MINUS_DST_ALPHA), BF(ONE_MINUS_SRC_ALPHA) },
/* Add         */ { 0, 0, BF(                ONE), BF(                ONE) },
};

static Bool
NV50EXA2DSurfaceFormat(PixmapPtr ppix, uint32_t *fmt)
{
	NV50EXA_LOCALS(ppix);

	switch (ppix->drawable.bitsPerPixel) {
	case 8 : *fmt = NV50_SURFACE_FORMAT_R8_UNORM; break;
	case 15: *fmt = NV50_SURFACE_FORMAT_BGR5_X1_UNORM; break;
	case 16: *fmt = NV50_SURFACE_FORMAT_B5G6R5_UNORM; break;
	case 24: *fmt = NV50_SURFACE_FORMAT_BGRX8_UNORM; break;
	case 30: *fmt = NV50_SURFACE_FORMAT_RGB10_A2_UNORM; break;
	case 32: *fmt = NV50_SURFACE_FORMAT_BGRA8_UNORM; break;
	default:
		 NOUVEAU_FALLBACK("Unknown surface format for bpp=%d\n",
				  ppix->drawable.bitsPerPixel);
		 return FALSE;
	}

	return TRUE;
}

static void NV50EXASetClip(PixmapPtr ppix, int x, int y, int w, int h)
{
	NV50EXA_LOCALS(ppix);

	BEGIN_NV04(push, NV50_2D(CLIP_X), 4);
	PUSH_DATA (push, x);
	PUSH_DATA (push, y);
	PUSH_DATA (push, w);
	PUSH_DATA (push, h);
}

static void
NV50EXAAcquireSurface2D(PixmapPtr ppix, int is_src, uint32_t fmt)
{
	NV50EXA_LOCALS(ppix);
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
#if !defined(__AROS__)
	struct nouveau_pixmap *nvpix = nouveau_pixmap(ppix);
#endif
	int mthd = is_src ? NV50_2D_SRC_FORMAT : NV50_2D_DST_FORMAT;
	uint32_t bo_flags;

#if !defined(__AROS__)
	bo_flags = nvpix->shared ? NOUVEAU_BO_GART : NOUVEAU_BO_VRAM;
#else
	bo_flags = NOUVEAU_BO_VRAM; /* AROS allocates all bitmaps in VRAM only */
#endif
	bo_flags |= is_src ? NOUVEAU_BO_RD : NOUVEAU_BO_WR;

	if (!nv50_style_tiled_pixmap(ppix)) {
		BEGIN_NV04(push, SUBC_2D(mthd), 2);
		PUSH_DATA (push, fmt);
		PUSH_DATA (push, 1);
		BEGIN_NV04(push, SUBC_2D(mthd + 0x14), 1);
		PUSH_DATA (push, (uint32_t)exaGetPixmapPitch(ppix));
	} else {
		BEGIN_NV04(push, SUBC_2D(mthd), 5);
		PUSH_DATA (push, fmt);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, bo->config.nv50.tile_mode);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, SUBC_2D(mthd + 0x18), 4);
	PUSH_DATA (push, ppix->drawable.width);
	PUSH_DATA (push, ppix->drawable.height);
	PUSH_DATA (push, bo->offset >> 32);
	PUSH_DATA (push, bo->offset);

	if (is_src == 0)
		NV50EXASetClip(ppix, 0, 0, ppix->drawable.width, ppix->drawable.height);

	PUSH_REFN (push, bo, bo_flags);
}

static void
NV50EXASetPattern(PixmapPtr pdpix, int col0, int col1, int pat0, int pat1)
{
	NV50EXA_LOCALS(pdpix);

	BEGIN_NV04(push, NV50_2D(PATTERN_COLOR(0)), 4);
	PUSH_DATA (push, col0);
	PUSH_DATA (push, col1);
	PUSH_DATA (push, pat0);
	PUSH_DATA (push, pat1);
}

static void
NV50EXASetROP(PixmapPtr pdpix, int alu, Pixel planemask)
{
	NV50EXA_LOCALS(pdpix);
	int rop;

	if (planemask != ~0)
		rop = NVROP[alu].copy_planemask;
	else
		rop = NVROP[alu].copy;

	BEGIN_NV04(push, NV50_2D(OPERATION), 1);
	if (alu == GXcopy && EXA_PM_IS_SOLID(&pdpix->drawable, planemask)) {
		PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY);
		return;
	} else {
		PUSH_DATA (push, NV50_2D_OPERATION_ROP);
	}

	BEGIN_NV04(push, NV50_2D(PATTERN_COLOR_FORMAT), 2);
	switch (pdpix->drawable.bitsPerPixel) {
		case  8: PUSH_DATA (push, 3); break;
		case 15: PUSH_DATA (push, 1); break;
		case 16: PUSH_DATA (push, 0); break;
		case 24:
		case 32:
		default:
			 PUSH_DATA (push, 2);
			 break;
	}
	PUSH_DATA (push, 1);

	/* There are 16 alu's.
	 * 0-15: copy
	 * 16-31: copy_planemask
	 */

	if (!EXA_PM_IS_SOLID(&pdpix->drawable, planemask)) {
		alu += 16;
		NV50EXASetPattern(pdpix, 0, planemask, ~0, ~0);
	} else {
		if (pNv->currentRop > 15)
			NV50EXASetPattern(pdpix, ~0, ~0, ~0, ~0);
	}

	if (pNv->currentRop != alu) {
		BEGIN_NV04(push, NV50_2D(ROP), 1);
		PUSH_DATA (push, rop);
		pNv->currentRop = alu;
	}
}

Bool
NV50EXAPrepareSolid(PixmapPtr pdpix, int alu, Pixel planemask, Pixel fg)
{
	NV50EXA_LOCALS(pdpix);
	uint32_t fmt;

	if (!NV50EXA2DSurfaceFormat(pdpix, &fmt))
		NOUVEAU_FALLBACK("rect format\n");

	if (!PUSH_SPACE(push, 64))
		NOUVEAU_FALLBACK("space\n");
	PUSH_RESET(push);

	NV50EXAAcquireSurface2D(pdpix, 0, fmt);
	NV50EXASetROP(pdpix, alu, planemask);

	BEGIN_NV04(push, NV50_2D(DRAW_SHAPE), 3);
	PUSH_DATA (push, NV50_2D_DRAW_SHAPE_RECTANGLES);
	PUSH_DATA (push, fmt);
	PUSH_DATA (push, fg);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		NOUVEAU_FALLBACK("validate\n");
	}

	return TRUE;
}

void
NV50EXASolid(PixmapPtr pdpix, int x1, int y1, int x2, int y2)
{
	NV50EXA_LOCALS(pdpix);

	if (!PUSH_SPACE(push, 8))
		return;

	BEGIN_NV04(push, NV50_2D(DRAW_POINT32_X(0)), 4);
	PUSH_DATA (push, x1);
	PUSH_DATA (push, y1);
	PUSH_DATA (push, x2);
	PUSH_DATA (push, y2);

	if ((x2 - x1) * (y2 - y1) >= 512)
		PUSH_KICK(push);
}

void
NV50EXADoneSolid(PixmapPtr pdpix)
{
	NV50EXA_LOCALS(pdpix);
	nouveau_pushbuf_bufctx(push, NULL);
}

Bool
NV50EXAPrepareCopy(PixmapPtr pspix, PixmapPtr pdpix, int dx, int dy,
		   int alu, Pixel planemask)
{
	NV50EXA_LOCALS(pdpix);
	uint32_t src, dst;

	if (!NV50EXA2DSurfaceFormat(pspix, &src))
		NOUVEAU_FALLBACK("src format\n");
	if (!NV50EXA2DSurfaceFormat(pdpix, &dst))
		NOUVEAU_FALLBACK("dst format\n");

	if (!PUSH_SPACE(push, 64))
		NOUVEAU_FALLBACK("space\n");
	PUSH_RESET(push);

	NV50EXAAcquireSurface2D(pspix, 1, src);
	NV50EXAAcquireSurface2D(pdpix, 0, dst);
	NV50EXASetROP(pdpix, alu, planemask);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		NOUVEAU_FALLBACK("validate\n");
	}

	return TRUE;
}

void
NV50EXACopy(PixmapPtr pdpix, int srcX , int srcY,
			     int dstX , int dstY,
			     int width, int height)
{
	NV50EXA_LOCALS(pdpix);

	if (!PUSH_SPACE(push, 32))
		return;

	BEGIN_NV04(push, SUBC_2D(NV50_GRAPH_SERIALIZE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV50_2D(BLIT_CONTROL), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV50_2D(BLIT_DST_X), 12);
	PUSH_DATA (push, dstX);
	PUSH_DATA (push, dstY);
	PUSH_DATA (push, width);
	PUSH_DATA (push, height);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, srcX);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, srcY);

	if (width * height >= 512)
		PUSH_KICK(push);
}

void
NV50EXADoneCopy(PixmapPtr pdpix)
{
	NV50EXA_LOCALS(pdpix);
	nouveau_pushbuf_bufctx(push, NULL);
}

#if !defined(__AROS__)
Bool
NV50EXAUploadSIFC(const char *src, int src_pitch,
		  PixmapPtr pdpix, int x, int y, int w, int h, int cpp)
{
	NV50EXA_LOCALS(pdpix);
	ScreenPtr pScreen = pdpix->drawable.pScreen;
	int line_dwords = (w * cpp + 3) / 4;
	uint32_t sifc_fmt;
	Bool ret = FALSE;

	if (!NV50EXA2DSurfaceFormat(pdpix, &sifc_fmt))
		NOUVEAU_FALLBACK("hostdata format\n");

	if (!PUSH_SPACE(push, 64))
		NOUVEAU_FALLBACK("space\n");
	PUSH_RESET(push);

	NV50EXAAcquireSurface2D(pdpix, 0, sifc_fmt);
	NV50EXASetClip(pdpix, x, y, w, h);

	BEGIN_NV04(push, NV50_2D(OPERATION), 1);
	PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY);
	BEGIN_NV04(push, NV50_2D(SIFC_BITMAP_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, sifc_fmt);
	BEGIN_NV04(push, NV50_2D(SIFC_WIDTH), 10);
	PUSH_DATA (push, (line_dwords * 4) / cpp);
	PUSH_DATA (push, h);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, x);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, y);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push))
		goto out;

	while (h--) {
		int count = line_dwords;
		const char *p = src;

		while(count) {
			int size = count > 1792 ? 1792 : count;

			if (!PUSH_SPACE(push, size + 1))
				goto out;
			BEGIN_NI04(push, NV50_2D(SIFC_DATA), size);
			PUSH_DATAp(push, p, size);

			p += size * 4;
			count -= size;
		}

		src += src_pitch;
	}

	ret = TRUE;
out:
	nouveau_pushbuf_bufctx(push, NULL);
	if (pdpix == pScreen->GetScreenPixmap(pScreen))
		PUSH_KICK(push);
	return ret;
}
#endif

static Bool
NV50EXACheckRenderTarget(PicturePtr ppict)
{
	if (ppict->pDrawable->width > 8192 ||
	    ppict->pDrawable->height > 8192)
		NOUVEAU_FALLBACK("render target dimensions exceeded %dx%d\n",
				 ppict->pDrawable->width,
				 ppict->pDrawable->height);

	switch (ppict->format) {
	case PICT_a8r8g8b8:
	case PICT_x8r8g8b8:
	case PICT_r5g6b5:
	case PICT_a8:
	case PICT_x1r5g5b5:
	case PICT_a1r5g5b5:
	case PICT_x8b8g8r8:
	case PICT_a2b10g10r10:
	case PICT_x2b10g10r10:
	case PICT_a2r10g10b10:
	case PICT_x2r10g10b10:
		break;
	default:
		NOUVEAU_FALLBACK("picture format 0x%08x\n", ppict->format);
	}

	return TRUE;
}

static Bool
NV50EXARenderTarget(PixmapPtr ppix, PicturePtr ppict)
{
	NV50EXA_LOCALS(ppix);
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
	unsigned format;

	/*XXX: Scanout buffer not tiled, someone needs to figure it out */
	if (!nv50_style_tiled_pixmap(ppix))
		NOUVEAU_FALLBACK("pixmap is scanout buffer\n");

	switch (ppict->format) {
	case PICT_a8r8g8b8: format = NV50_SURFACE_FORMAT_BGRA8_UNORM; break;
	case PICT_x8r8g8b8: format = NV50_SURFACE_FORMAT_BGRX8_UNORM; break;
	case PICT_r5g6b5  : format = NV50_SURFACE_FORMAT_B5G6R5_UNORM; break;
	case PICT_a8      : format = NV50_SURFACE_FORMAT_A8_UNORM; break;
	case PICT_x1r5g5b5:
	case PICT_a1r5g5b5:
		format = NV50_SURFACE_FORMAT_BGR5_A1_UNORM;
		break;
	case PICT_x8b8g8r8: format = NV50_SURFACE_FORMAT_RGBX8_UNORM; break;
	case PICT_a2b10g10r10:
	case PICT_x2b10g10r10:
		format = NV50_SURFACE_FORMAT_RGB10_A2_UNORM;
		break;
	case PICT_a2r10g10b10:
	case PICT_x2r10g10b10:
		format = NV50_SURFACE_FORMAT_BGR10_A2_UNORM;
		break;
	default:
		NOUVEAU_FALLBACK("invalid picture format\n");
	}

	PUSH_REFN (push, bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_NV04(push, NV50_3D(RT_ADDRESS_HIGH(0)), 5);
	PUSH_DATA (push, bo->offset >> 32);
	PUSH_DATA (push, bo->offset);
	PUSH_DATA (push, format);
	PUSH_DATA (push, bo->config.nv50.tile_mode);
	PUSH_DATA (push, 0x00000000);
	BEGIN_NV04(push, NV50_3D(RT_HORIZ(0)), 2);
	PUSH_DATA (push, ppix->drawable.width);
	PUSH_DATA (push, ppix->drawable.height);
	BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
	PUSH_DATA (push, 0x00000001);

	return TRUE;
}

#if !defined(__AROS__)
static Bool
NV50EXACheckTexture(PicturePtr ppict, PicturePtr pdpict, int op)
{
	if (ppict->pDrawable) {
		if (ppict->pDrawable->width > 8192 ||
		    ppict->pDrawable->height > 8192)
			NOUVEAU_FALLBACK("texture too large\n");
	} else {
		switch (ppict->pSourcePict->type) {
		case SourcePictTypeSolidFill:
			break;
		default:
			NOUVEAU_FALLBACK("pict %d\n", ppict->pSourcePict->type);
			break;
		}
	}

	switch (ppict->format) {
	case PICT_a8r8g8b8:
	case PICT_a8b8g8r8:
	case PICT_x8r8g8b8:
	case PICT_x8b8g8r8:
	case PICT_r5g6b5:
	case PICT_a8:
	case PICT_x1r5g5b5:
	case PICT_x1b5g5r5:
	case PICT_a1r5g5b5:
	case PICT_a1b5g5r5:
	case PICT_b5g6r5:
	case PICT_b8g8r8a8:
	case PICT_b8g8r8x8:
	case PICT_a2b10g10r10:
	case PICT_x2b10g10r10:
	case PICT_x2r10g10b10:
	case PICT_a2r10g10b10:
	case PICT_x4r4g4b4:
	case PICT_x4b4g4r4:
	case PICT_a4r4g4b4:
	case PICT_a4b4g4r4:
		break;
	default:
		NOUVEAU_FALLBACK("picture format 0x%08x\n", ppict->format);
	}

	switch (ppict->filter) {
	case PictFilterNearest:
	case PictFilterBilinear:
		break;
	default:
		NOUVEAU_FALLBACK("picture filter %d\n", ppict->filter);
	}

	/* Opengl and Render disagree on what should be sampled outside an XRGB 
	 * texture (with no repeating). Opengl has a hardcoded alpha value of 
	 * 1.0, while render expects 0.0. We assume that clipping is done for 
	 * untranformed sources.
	 */
	if (NV50EXABlendOp[op].src_alpha && !ppict->repeat &&
		ppict->transform && (PICT_FORMAT_A(ppict->format) == 0)
		&& (PICT_FORMAT_A(pdpict->format) != 0))
		NOUVEAU_FALLBACK("REPEAT_NONE unsupported for XRGB source\n");

	return TRUE;
}
#endif

#define _(X1,X2,X3,X4,FMT) (NV50TIC_0_0_TYPER_UNORM | NV50TIC_0_0_TYPEG_UNORM | NV50TIC_0_0_TYPEB_UNORM | NV50TIC_0_0_TYPEA_UNORM | \
			    NV50TIC_0_0_MAP##X1 | NV50TIC_0_0_MAP##X2 | NV50TIC_0_0_MAP##X3 | NV50TIC_0_0_MAP##X4 | \
			    NV50TIC_0_0_FMT_##FMT)

#if !defined(__AROS__)
static Bool
NV50EXAPictSolid(NVPtr pNv, PicturePtr ppict, unsigned unit)
{
	uint64_t offset = pNv->scratch->offset + SOLID(unit);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	PUSH_DATAu(push, pNv->scratch, SOLID(unit), 1);
	PUSH_DATA (push, ppict->pSourcePict->solidFill.color);
	PUSH_DATAu(push, pNv->scratch, TIC_OFFSET + (unit * 32), 8);
	PUSH_DATA (push, _(B_C0, G_C1, R_C2, A_C3, 8_8_8_8));
	PUSH_DATA (push,  offset);
	PUSH_DATA (push, (offset >> 32) | 0xd005d000);
	PUSH_DATA (push, 0x00300000);
	PUSH_DATA (push, 0x00000001);
	PUSH_DATA (push, 0x00010001);
	PUSH_DATA (push, 0x03000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATAu(push, pNv->scratch, TSC_OFFSET + (unit * 32), 8);
	PUSH_DATA (push, NV50TSC_1_0_WRAPS_REPEAT |
			 NV50TSC_1_0_WRAPT_REPEAT |
			 NV50TSC_1_0_WRAPR_REPEAT | 0x00024000);
	PUSH_DATA (push, NV50TSC_1_1_MAGF_NEAREST |
			 NV50TSC_1_1_MINF_NEAREST |
			 NV50TSC_1_1_MIPF_NONE);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);

	return TRUE;
}
#endif

static Bool
NV50EXAPictGradient(NVPtr pNv, PicturePtr ppict, unsigned unit)
{
	return FALSE;
}

static Bool
NV50EXAPictTexture(NVPtr pNv, PixmapPtr ppix, PicturePtr ppict, unsigned unit)
{
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t format;

	/*XXX: Scanout buffer not tiled, someone needs to figure it out */
	if (!nv50_style_tiled_pixmap(ppix))
		NOUVEAU_FALLBACK("pixmap is scanout buffer\n");

	switch (ppict->format) {
	case PICT_a8r8g8b8:
		format = _(B_C0, G_C1, R_C2, A_C3, 8_8_8_8);
		break;
	case PICT_a8b8g8r8:
		format = _(R_C0, G_C1, B_C2, A_C3, 8_8_8_8);
		break;
	case PICT_x8r8g8b8:
		format = _(B_C0, G_C1, R_C2, A_ONE, 8_8_8_8);
		break;
	case PICT_x8b8g8r8:
		format = _(R_C0, G_C1, B_C2, A_ONE, 8_8_8_8);
		break;
	case PICT_r5g6b5:
		format = _(B_C0, G_C1, R_C2, A_ONE, 5_6_5);
		break;
	case PICT_a8:
		format = _(A_C0, B_ZERO, G_ZERO, R_ZERO, 8);
		break;
	case PICT_x1r5g5b5:
		format = _(B_C0, G_C1, R_C2, A_ONE, 1_5_5_5);
		break;
	case PICT_x1b5g5r5:
		format = _(R_C0, G_C1, B_C2, A_ONE, 1_5_5_5);
		break;
	case PICT_a1r5g5b5:
		format = _(B_C0, G_C1, R_C2, A_C3, 1_5_5_5);
		break;
	case PICT_a1b5g5r5:
		format = _(R_C0, G_C1, B_C2, A_C3, 1_5_5_5);
		break;
	case PICT_b5g6r5:
		format = _(R_C0, G_C1, B_C2, A_ONE, 5_6_5);
		break;
	case PICT_b8g8r8x8:
		format = _(A_ONE, R_C1, G_C2, B_C3, 8_8_8_8);
		break;
	case PICT_b8g8r8a8:
		format = _(A_C0, R_C1, G_C2, B_C3, 8_8_8_8);
		break;
	case PICT_a2b10g10r10:
		format = _(R_C0, G_C1, B_C2, A_C3, 2_10_10_10);
		break;
	case PICT_x2b10g10r10:
		format = _(R_C0, G_C1, B_C2, A_ONE, 2_10_10_10);
		break;
	case PICT_x2r10g10b10:
		format = _(B_C0, G_C1, R_C2, A_ONE, 2_10_10_10);
		break;
	case PICT_a2r10g10b10:
		format = _(B_C0, G_C1, R_C2, A_C3, 2_10_10_10);
		break;
	case PICT_x4r4g4b4:
		format = _(B_C0, G_C1, R_C2, A_ONE, 4_4_4_4);
		break;
	case PICT_x4b4g4r4:
		format = _(R_C0, G_C1, B_C2, A_ONE, 4_4_4_4);
		break;
	case PICT_a4r4g4b4:
		format = _(B_C0, G_C1, R_C2, A_C3, 4_4_4_4);
		break;
	case PICT_a4b4g4r4:
		format = _(R_C0, G_C1, B_C2, A_C3, 4_4_4_4);
		break;
	default:
		NOUVEAU_FALLBACK("invalid picture format, this SHOULD NOT HAPPEN. Expect trouble.\n");
	}
#undef _

	PUSH_REFN (push, bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	PUSH_DATAu(push, pNv->scratch, TIC_OFFSET + (unit * 32), 8);
	PUSH_DATA (push, format);
	PUSH_DATA (push, bo->offset);
	PUSH_DATA (push, (bo->offset >> 32) |
			 (bo->config.nv50.tile_mode << 18) |
			 0xd0005000);
	PUSH_DATA (push, 0x00300000);
	PUSH_DATA (push, ppix->drawable.width);
	PUSH_DATA (push, (1 << NV50TIC_0_5_DEPTH_SHIFT) | ppix->drawable.height);
	PUSH_DATA (push, 0x03000000);
	PUSH_DATA (push, 0x00000000);

	PUSH_DATAu(push, pNv->scratch, TSC_OFFSET + (unit * 32), 8);
	if (ppict->repeat) {
		switch (ppict->repeatType) {
		case RepeatPad:
			PUSH_DATA (push, NV50TSC_1_0_WRAPS_CLAMP_TO_EDGE |
				 NV50TSC_1_0_WRAPT_CLAMP_TO_EDGE |
				 NV50TSC_1_0_WRAPR_CLAMP_TO_EDGE | 0x00024000);
			break;
		case RepeatReflect:
			PUSH_DATA (push, NV50TSC_1_0_WRAPS_MIRROR_REPEAT |
				 NV50TSC_1_0_WRAPT_MIRROR_REPEAT |
				 NV50TSC_1_0_WRAPR_MIRROR_REPEAT | 0x00024000);
			break;
		case RepeatNormal:
		default:
			PUSH_DATA (push, NV50TSC_1_0_WRAPS_REPEAT |
				 NV50TSC_1_0_WRAPT_REPEAT |
				 NV50TSC_1_0_WRAPR_REPEAT | 0x00024000);
			break;
		}
	} else {
		PUSH_DATA (push, NV50TSC_1_0_WRAPS_CLAMP_TO_BORDER |
			 NV50TSC_1_0_WRAPT_CLAMP_TO_BORDER |
			 NV50TSC_1_0_WRAPR_CLAMP_TO_BORDER | 0x00024000);
	}
	if (ppict->filter == PictFilterBilinear) {
		PUSH_DATA (push, NV50TSC_1_1_MAGF_LINEAR |
			 NV50TSC_1_1_MINF_LINEAR |
			 NV50TSC_1_1_MIPF_NONE);
	} else {
		PUSH_DATA (push, NV50TSC_1_1_MAGF_NEAREST |
			 NV50TSC_1_1_MINF_NEAREST |
			 NV50TSC_1_1_MIPF_NONE);
	}
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);

	PUSH_DATAu(push, pNv->scratch, PVP_DATA + (unit * 11 * 4), 11);
#if !defined(__AROS__)
	if (ppict->transform) {
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[0][0]));
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[0][1]));
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[0][2]));
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[1][0]));
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[1][1]));
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[1][2]));
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[2][0]));
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[2][1]));
		PUSH_DATAf(push, xFixedToFloat(ppict->transform->matrix[2][2]));
	} else {
#else
	{ // we are not doing any transformations
#endif
		PUSH_DATAf(push, 1.0);
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 1.0);
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 0.0);
		PUSH_DATAf(push, 1.0);
	}
	PUSH_DATAf(push, 1.0 / ppix->drawable.width);
	PUSH_DATAf(push, 1.0 / ppix->drawable.height);
	return TRUE;
}

static Bool
NV50EXAPicture(NVPtr pNv, PixmapPtr ppix, PicturePtr ppict, int unit)
{
	if (ppict->pDrawable)
		return NV50EXAPictTexture(pNv, ppix, ppict, unit);

NOT_IMPLEMENTED_STOP
#if 0
	switch (ppict->pSourcePict->type) {
	case SourcePictTypeSolidFill:
		return NV50EXAPictSolid(pNv, ppict, unit);
	case SourcePictTypeLinear:
		return NV50EXAPictGradient(pNv, ppict, unit);
	default:
		break;
	}
#endif

	return FALSE;
}

#if !defined(__AROS__)
static Bool
NV50EXACheckBlend(int op)
{
	if (op > PictOpAdd)
		NOUVEAU_FALLBACK("unsupported blend op %d\n", op);
	return TRUE;
}
#endif

static void
NV50EXABlend(PixmapPtr ppix, PicturePtr ppict, int op, int component_alpha)
{
	NV50EXA_LOCALS(ppix);
	struct nv50_blend_op *b = &NV50EXABlendOp[op];
	unsigned sblend = b->src_blend;
	unsigned dblend = b->dst_blend;

	if (b->dst_alpha) {
		if (!PICT_FORMAT_A(ppict->format)) {
			if (sblend == BF(DST_ALPHA))
				sblend = BF(ONE);
			else
			if (sblend == BF(ONE_MINUS_DST_ALPHA))
				sblend = BF(ZERO);
		}
	}

	if (b->src_alpha && component_alpha) {
		if (dblend == BF(SRC_ALPHA))
			dblend = BF(SRC_COLOR);
		else
		if (dblend == BF(ONE_MINUS_SRC_ALPHA))
			dblend = BF(ONE_MINUS_SRC_COLOR);
	}

	if (sblend == BF(ONE) && dblend == BF(ZERO)) {
		BEGIN_NV04(push, NV50_3D(BLEND_ENABLE(0)), 1);
		PUSH_DATA (push, 0);
	} else {
		BEGIN_NV04(push, NV50_3D(BLEND_ENABLE(0)), 1);
		PUSH_DATA (push, 1);
		BEGIN_NV04(push, NV50_3D(BLEND_EQUATION_RGB), 5);
		PUSH_DATA (push, NV50_3D_BLEND_EQUATION_RGB_FUNC_ADD);
		PUSH_DATA (push, sblend);
		PUSH_DATA (push, dblend);
		PUSH_DATA (push, NV50_3D_BLEND_EQUATION_ALPHA_FUNC_ADD);
		PUSH_DATA (push, sblend);
		BEGIN_NV04(push, NV50_3D(BLEND_FUNC_DST_ALPHA), 1);
		PUSH_DATA (push, dblend);
	}
}

#if !defined(__AROS__)
Bool
NV50EXACheckComposite(int op,
		      PicturePtr pspict, PicturePtr pmpict, PicturePtr pdpict)
{
	if (!NV50EXACheckBlend(op))
		NOUVEAU_FALLBACK("blend not supported\n");

	if (!NV50EXACheckRenderTarget(pdpict))
		NOUVEAU_FALLBACK("render target invalid\n");

	if (!NV50EXACheckTexture(pspict, pdpict, op))
		NOUVEAU_FALLBACK("src picture invalid\n");

	if (pmpict) {
		if (pmpict->componentAlpha &&
		    PICT_FORMAT_RGB(pmpict->format) &&
		    NV50EXABlendOp[op].src_alpha &&
		    NV50EXABlendOp[op].src_blend != BF(ZERO))
			NOUVEAU_FALLBACK("component-alpha not supported\n");

		if (!NV50EXACheckTexture(pmpict, pdpict, op))
			NOUVEAU_FALLBACK("mask picture invalid\n");
	}

	return TRUE;
}
#endif

Bool
NV50EXAPrepareComposite(int op,
			PicturePtr pspict, PicturePtr pmpict, PicturePtr pdpict,
			PixmapPtr pspix, PixmapPtr pmpix, PixmapPtr pdpix)
{
	NV50EXA_LOCALS(pdpix);

	if (!PUSH_SPACE(push, 256))
		NOUVEAU_FALLBACK("space\n");
	PUSH_RESET(push);
	PUSH_REFN (push, pNv->scratch, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);

	BEGIN_NV04(push, SUBC_2D(NV50_GRAPH_SERIALIZE), 1);
	PUSH_DATA (push, 0);

	if (!NV50EXARenderTarget(pdpix, pdpict))
		NOUVEAU_FALLBACK("render target invalid\n");

	NV50EXABlend(pdpix, pdpict, op, pmpict && pmpict->componentAlpha &&
		     PICT_FORMAT_RGB(pmpict->format));

	if (!NV50EXAPicture(pNv, pspix, pspict, 0))
		NOUVEAU_FALLBACK("src picture invalid\n");

	if (pmpict) {
		if (!NV50EXAPicture(pNv, pmpix, pmpict, 1))
			NOUVEAU_FALLBACK("mask picture invalid\n");

		BEGIN_NV04(push, NV50_3D(FP_START_ID), 1);
		if (pdpict->format == PICT_a8) {
			PUSH_DATA (push, PFP_C_A8);
		} else {
			if (pmpict->componentAlpha &&
			    PICT_FORMAT_RGB(pmpict->format)) {
				if (NV50EXABlendOp[op].src_alpha)
					PUSH_DATA (push, PFP_CCASA);
				else
					PUSH_DATA (push, PFP_CCA);
			} else {
				PUSH_DATA (push, PFP_C);
			}
		}
	} else {
		BEGIN_NV04(push, NV50_3D(FP_START_ID), 1);
		if (pdpict->format == PICT_a8)
			PUSH_DATA (push, PFP_S_A8);
		else
			PUSH_DATA (push, PFP_S);
	}

	BEGIN_NV04(push, NV50_3D(TIC_FLUSH), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV50_3D(BIND_TIC(2)), 1);
	PUSH_DATA (push, 1);
	BEGIN_NV04(push, NV50_3D(BIND_TIC(2)), 1);
	PUSH_DATA (push, 0x203);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		NOUVEAU_FALLBACK("validate\n");
	}

	return TRUE;
}

void
NV50EXAComposite(PixmapPtr pdpix, int sx, int sy, int mx, int my,
		 int dx, int dy, int w, int h)
{
	NV50EXA_LOCALS(pdpix);

	if (!PUSH_SPACE(push, 64))
		return;

	BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
	PUSH_DATA (push, (dx + w) << 16 | dx);
	PUSH_DATA (push, (dy + h) << 16 | dy);
	BEGIN_NV04(push, NV50_3D(VERTEX_BEGIN_GL), 1);
	PUSH_DATA (push, NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_TRIANGLES);
	PUSH_VTX2s(push, sx, sy + (h * 2), mx, my + (h * 2), dx, dy + (h * 2));
	PUSH_VTX2s(push, sx, sy, mx, my, dx, dy);
	PUSH_VTX2s(push, sx + (w * 2), sy, mx + (w * 2), my, dx + (w * 2), dy);
	BEGIN_NV04(push, NV50_3D(VERTEX_END_GL), 1);
	PUSH_DATA (push, 0);
}

void
NV50EXADoneComposite(PixmapPtr pdpix)
{
	NV50EXA_LOCALS(pdpix);
	nouveau_pushbuf_bufctx(push, NULL);
}

Bool
NV50EXARectM2MF(NVPtr pNv, int w, int h, int cpp,
		struct nouveau_bo *src, uint32_t src_off, int src_dom,
		int src_pitch, int src_h, int src_x, int src_y,
		struct nouveau_bo *dst, uint32_t dst_off, int dst_dom,
		int dst_pitch, int dst_h, int dst_x, int dst_y)
{
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_pushbuf_refn refs[] = {
		{ src, src_dom | NOUVEAU_BO_RD },
		{ dst, dst_dom | NOUVEAU_BO_WR },
	};

	if (!PUSH_SPACE(push, 64))
		return FALSE;

	if (src->config.nv50.memtype) {
		BEGIN_NV04(push, NV50_M2MF(LINEAR_IN), 6);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, src->config.nv50.tile_mode);
		PUSH_DATA (push, src_pitch);
		PUSH_DATA (push, src_h);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 0);
	} else {
		BEGIN_NV04(push, NV50_M2MF(LINEAR_IN), 1);
		PUSH_DATA (push, 1);
		BEGIN_NV04(push, NV03_M2MF(PITCH_IN), 1);
		PUSH_DATA (push, src_pitch);
		src_off += src_y * src_pitch + src_x * cpp;
	}

	if (dst->config.nv50.memtype) {
		BEGIN_NV04(push, NV50_M2MF(LINEAR_OUT), 6);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, dst->config.nv50.tile_mode);
		PUSH_DATA (push, dst_pitch);
		PUSH_DATA (push, dst_h);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 0);
	} else {
		BEGIN_NV04(push, NV50_M2MF(LINEAR_OUT), 1);
		PUSH_DATA (push, 1);
		BEGIN_NV04(push, NV03_M2MF(PITCH_OUT), 1);
		PUSH_DATA (push, dst_pitch);
		dst_off += dst_y * dst_pitch + dst_x * cpp;
	}

	while (h) {
		int line_count = h;
		if (line_count > 2047)
			line_count = 2047;

		if (nouveau_pushbuf_space(push, 32, 0, 0) ||
		    nouveau_pushbuf_refn (push, refs, 2))
			return FALSE;

		BEGIN_NV04(push, NV50_M2MF(OFFSET_IN_HIGH), 2);
		PUSH_DATA (push, (src->offset + src_off) >> 32);
		PUSH_DATA (push, (dst->offset + dst_off) >> 32);
		BEGIN_NV04(push, NV03_M2MF(OFFSET_IN), 2);
		PUSH_DATA (push, (src->offset + src_off));
		PUSH_DATA (push, (dst->offset + dst_off));

		if (src->config.nv50.memtype) {
			BEGIN_NV04(push, NV50_M2MF(TILING_POSITION_IN), 1);
			PUSH_DATA (push, (src_y << 16) | (src_x * cpp));
		} else {
			src_off += line_count * src_pitch;
		}

		if (dst->config.nv50.memtype) {
			BEGIN_NV04(push, NV50_M2MF(TILING_POSITION_OUT), 1);
			PUSH_DATA (push, (dst_y << 16) | (dst_x * cpp));
		} else {
			dst_off += line_count * dst_pitch;
		}

		BEGIN_NV04(push, NV03_M2MF(LINE_LENGTH_IN), 4);
		PUSH_DATA (push, w * cpp);
		PUSH_DATA (push, line_count);
		PUSH_DATA (push, 0x00000101);
		PUSH_DATA (push, 0x00000000);

		src_y += line_count;
		dst_y += line_count;
		h  -= line_count;
	}

	return TRUE;
}

/* AROS CODE */

VOID HIDDNouveauNV50SetPattern(struct CardData * carddata, LONG col0,
    LONG col1, LONG pat0, LONG pat1)
{
    NV50EXASetPattern(NULL, col0, col1, pat0, pat1);
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNV50FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, LONG minX, LONG minY, LONG maxX,
    LONG maxY, ULONG drawmode, ULONG color)
{
    if (NV50EXAPrepareSolid(bmdata, drawmode, ~0, color))
    {
        NV50EXASolid(bmdata, minX, minY, maxX + 1, maxY + 1);
        NV50EXADoneSolid(bmdata);
        return TRUE;
    }

    return FALSE;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNV50CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG drawmode)
{
    if (NV50EXAPrepareCopy(srcdata, destdata, 0, 0, drawmode, ~0))
    {
        NV50EXACopy(destdata, srcX, srcY, destX , destY, width, height);
        NV50EXADoneCopy(destdata);
        return TRUE;
    }

    return FALSE;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
/* NOTE: Allows different formats of source and destination */
BOOL HIDDNouveauNV503DCopyBox(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG blendop)
{
    struct Picture sPict, dPict;
    ULONG maskX = 0; ULONG maskY = 0;

    HIDDNouveauFillPictureFromBitMapData(&sPict, srcdata);
    HIDDNouveauFillPictureFromBitMapData(&dPict, destdata);

    if (NV50EXAPrepareComposite(blendop,
        &sPict, NULL, &dPict, srcdata, NULL, destdata))
    {
        NV50EXAComposite(destdata, srcX, srcY,
                        maskX, maskY,
                        destX , destY,
                        width, height);
        NV50EXADoneComposite(destdata);
        return TRUE;
    }

    return FALSE;
}
