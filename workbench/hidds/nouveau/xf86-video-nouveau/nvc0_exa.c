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
#include "nvc0_accel.h"
#include "nouveau_copy.h"

#define NOUVEAU_BO(a, b, c) (NOUVEAU_BO_##a | NOUVEAU_BO_##b | NOUVEAU_BO_##c)

#define NVC0EXA_LOCALS(p)                                                      \
	ScrnInfoPtr pScrn = xf86ScreenToScrn((p)->drawable.pScreen);         \
	NVPtr pNv = NVPTR(pScrn);                                              \
	struct nouveau_pushbuf *push = pNv->pushbuf; (void)push;

#define BF(f) NV50_BLEND_FACTOR_##f

struct nvc0_blend_op {
	unsigned src_alpha;
	unsigned dst_alpha;
	unsigned src_blend;
	unsigned dst_blend;
};

static struct nvc0_blend_op
NVC0EXABlendOp[] = {
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
NVC0EXA2DSurfaceFormat(PixmapPtr ppix, uint32_t *fmt)
{
	NVC0EXA_LOCALS(ppix);

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

static void NVC0EXASetClip(PixmapPtr ppix, int x, int y, int w, int h)
{
	NVC0EXA_LOCALS(ppix);

	BEGIN_NVC0(push, NV50_2D(CLIP_X), 4);
	PUSH_DATA (push, x);
	PUSH_DATA (push, y);
	PUSH_DATA (push, w);
	PUSH_DATA (push, h);
}

static void
NVC0EXAAcquireSurface2D(PixmapPtr ppix, int is_src, uint32_t fmt)
{
	NVC0EXA_LOCALS(ppix);
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
		BEGIN_NVC0(push, SUBC_2D(mthd), 2);
		PUSH_DATA (push, fmt);
		PUSH_DATA (push, 1);
		BEGIN_NVC0(push, SUBC_2D(mthd + 0x14), 1);
		PUSH_DATA (push, (uint32_t)exaGetPixmapPitch(ppix));
	} else {
		BEGIN_NVC0(push, SUBC_2D(mthd), 5);
		PUSH_DATA (push, fmt);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, bo->config.nvc0.tile_mode);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NVC0(push, SUBC_2D(mthd + 0x18), 4);
	PUSH_DATA (push, ppix->drawable.width);
	PUSH_DATA (push, ppix->drawable.height);
	PUSH_DATA (push, bo->offset >> 32);
	PUSH_DATA (push, bo->offset);

	if (is_src == 0)
		NVC0EXASetClip(ppix, 0, 0, ppix->drawable.width, ppix->drawable.height);

	PUSH_REFN (push, bo, bo_flags);
}

static void
NVC0EXASetPattern(PixmapPtr pdpix, int col0, int col1, int pat0, int pat1)
{
	NVC0EXA_LOCALS(pdpix);

	BEGIN_NVC0(push, NV50_2D(PATTERN_COLOR(0)), 4);
	PUSH_DATA (push, col0);
	PUSH_DATA (push, col1);
	PUSH_DATA (push, pat0);
	PUSH_DATA (push, pat1);
}

static void
NVC0EXASetROP(PixmapPtr pdpix, int alu, Pixel planemask)
{
	NVC0EXA_LOCALS(pdpix);
	int rop;

	if (planemask != ~0)
		rop = NVROP[alu].copy_planemask;
	else
		rop = NVROP[alu].copy;

	BEGIN_NVC0(push, NV50_2D(OPERATION), 1);
	if (alu == GXcopy && EXA_PM_IS_SOLID(&pdpix->drawable, planemask)) {
		PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY);
		return;
	} else {
		PUSH_DATA (push, NV50_2D_OPERATION_ROP);
	}

	BEGIN_NVC0(push, NV50_2D(PATTERN_COLOR_FORMAT), 2);
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

	/* There are 16 ALUs.
	 * 0-15: copy
	 * 16-31: copy_planemask
	 */

	if (!EXA_PM_IS_SOLID(&pdpix->drawable, planemask)) {
		alu += 16;
		NVC0EXASetPattern(pdpix, 0, planemask, ~0, ~0);
	} else {
		if (pNv->currentRop > 15)
			NVC0EXASetPattern(pdpix, ~0, ~0, ~0, ~0);
	}

	if (pNv->currentRop != alu) {
		BEGIN_NVC0(push, NV50_2D(ROP), 1);
		PUSH_DATA (push, rop);
		pNv->currentRop = alu;
	}
}

Bool
NVC0EXAPrepareSolid(PixmapPtr pdpix, int alu, Pixel planemask, Pixel fg)
{
	NVC0EXA_LOCALS(pdpix);
	uint32_t fmt;

	if (!NVC0EXA2DSurfaceFormat(pdpix, &fmt))
		NOUVEAU_FALLBACK("rect format\n");

	if (!PUSH_SPACE(push, 64))
		NOUVEAU_FALLBACK("space\n");
	PUSH_RESET(push);

	NVC0EXAAcquireSurface2D(pdpix, 0, fmt);
	NVC0EXASetROP(pdpix, alu, planemask);

	BEGIN_NVC0(push, NV50_2D(DRAW_SHAPE), 3);
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
NVC0EXASolid(PixmapPtr pdpix, int x1, int y1, int x2, int y2)
{
	NVC0EXA_LOCALS(pdpix);

	if (!PUSH_SPACE(push, 8))
		return;

	BEGIN_NVC0(push, NV50_2D(DRAW_POINT32_X(0)), 4);
	PUSH_DATA (push, x1);
	PUSH_DATA (push, y1);
	PUSH_DATA (push, x2);
	PUSH_DATA (push, y2);

#if !defined(__AROS__)
	if ((x2 - x1) * (y2 - y1) >= 512)
#endif
		PUSH_KICK(push);
}

void
NVC0EXADoneSolid(PixmapPtr pdpix)
{
	NVC0EXA_LOCALS(pdpix);
	nouveau_pushbuf_bufctx(push, NULL);
}

Bool
NVC0EXAPrepareCopy(PixmapPtr pspix, PixmapPtr pdpix, int dx, int dy,
		   int alu, Pixel planemask)
{
	NVC0EXA_LOCALS(pdpix);
	uint32_t src, dst;

	if (!NVC0EXA2DSurfaceFormat(pspix, &src))
		NOUVEAU_FALLBACK("src format\n");
	if (!NVC0EXA2DSurfaceFormat(pdpix, &dst))
		NOUVEAU_FALLBACK("dst format\n");

	if (!PUSH_SPACE(push, 64))
		NOUVEAU_FALLBACK("space\n");
	PUSH_RESET(push);

	NVC0EXAAcquireSurface2D(pspix, 1, src);
	NVC0EXAAcquireSurface2D(pdpix, 0, dst);
	NVC0EXASetROP(pdpix, alu, planemask);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		NOUVEAU_FALLBACK("validate\n");
	}

	return TRUE;
}

void
NVC0EXACopy(PixmapPtr pdpix, int srcX , int srcY,
			     int dstX , int dstY,
			     int width, int height)
{
	NVC0EXA_LOCALS(pdpix);

	if (!PUSH_SPACE(push, 32))
		return;

	BEGIN_NVC0(push, SUBC_2D(NV50_GRAPH_SERIALIZE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NVC0(push, NV50_2D(BLIT_CONTROL), 1);
	PUSH_DATA (push, 0);
	BEGIN_NVC0(push, NV50_2D(BLIT_DST_X), 12);
	PUSH_DATA (push, dstX);
	PUSH_DATA (push, dstY);
	PUSH_DATA (push, width);
	PUSH_DATA (push, height);
	PUSH_DATA (push, 0); /* DU,V_DX,Y_FRACT,INT */
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0); /* BLIT_SRC_X,Y_FRACT,INT */
	PUSH_DATA (push, srcX);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, srcY);

#if !defined(__AROS__)
	if (width * height >= 512)
#endif
		PUSH_KICK(push);
}

void
NVC0EXADoneCopy(PixmapPtr pdpix)
{
	NVC0EXA_LOCALS(pdpix);
	nouveau_pushbuf_bufctx(push, NULL);
}

#if !defined(__AROS__)
Bool
NVC0EXAUploadSIFC(const char *src, int src_pitch,
		  PixmapPtr pdpix, int x, int y, int w, int h, int cpp)
{
	NVC0EXA_LOCALS(pdpix);
	ScreenPtr pScreen = pdpix->drawable.pScreen;
	int line_dwords = (w * cpp + 3) / 4;
	uint32_t sifc_fmt;
	Bool ret = FALSE;

	if (!NVC0EXA2DSurfaceFormat(pdpix, &sifc_fmt))
		NOUVEAU_FALLBACK("hostdata format\n");

	if (!PUSH_SPACE(push, 64))
		NOUVEAU_FALLBACK("pushbuf\n");
	PUSH_RESET(push);

	NVC0EXAAcquireSurface2D(pdpix, 0, sifc_fmt);
	NVC0EXASetClip(pdpix, x, y, w, h);

	BEGIN_NVC0(push, NV50_2D(OPERATION), 1);
	PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY);
	BEGIN_NVC0(push, NV50_2D(SIFC_BITMAP_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, sifc_fmt);
	BEGIN_NVC0(push, NV50_2D(SIFC_WIDTH), 10);
	PUSH_DATA (push, (line_dwords * 4) / cpp);
	PUSH_DATA (push, h);
	PUSH_DATA (push, 0); /* SIFC_DX,Y_DU,V_FRACT,INT */
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0); /* SIFC_DST_X,Y_FRACT,INT */
	PUSH_DATA (push, x);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, y);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push))
		goto out;

	while (h--) {
		const char *ptr = src;
		int count = line_dwords;

		while (count) {
			int size = count > 1792 ? 1792 : count;

			if (!PUSH_SPACE(push, size + 1))
				goto out;
			BEGIN_NIC0(push, NV50_2D(SIFC_DATA), size);
			PUSH_DATAp(push, ptr, size);

			ptr += size * 4;
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
NVC0EXACheckRenderTarget(PicturePtr ppict)
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
NVC0EXARenderTarget(PixmapPtr ppix, PicturePtr ppict)
{
	NVC0EXA_LOCALS(ppix);
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
	case PICT_x1r5g5b5: format = NV50_SURFACE_FORMAT_BGR5_X1_UNORM; break;
	case PICT_a1r5g5b5: format = NV50_SURFACE_FORMAT_BGR5_A1_UNORM; break;
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

	BEGIN_NVC0(push, NVC0_3D(RT_ADDRESS_HIGH(0)), 8);
	PUSH_DATA (push, bo->offset >> 32);
	PUSH_DATA (push, bo->offset);
	PUSH_DATA (push, ppix->drawable.width);
	PUSH_DATA (push, ppix->drawable.height);
	PUSH_DATA (push, format);
	PUSH_DATA (push, bo->config.nvc0.tile_mode);
	PUSH_DATA (push, 0x00000001);
	PUSH_DATA (push, 0x00000000);
	return TRUE;
}

#if !defined(__AROS__)
static Bool
NVC0EXACheckTexture(PicturePtr ppict, PicturePtr pdpict, int op)
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

	/* OpenGL and Render disagree on what should be sampled outside an XRGB 
	 * texture (with no repeating). Opengl has a hardcoded alpha value of 
	 * 1.0, while render expects 0.0. We assume that clipping is done for 
	 * untranformed sources.
	 */
	if (NVC0EXABlendOp[op].src_alpha && !ppict->repeat &&
	    ppict->transform && (PICT_FORMAT_A(ppict->format) == 0)
	    && (PICT_FORMAT_A(pdpict->format) != 0))
		NOUVEAU_FALLBACK("REPEAT_NONE unsupported for XRGB source\n");

	return TRUE;
}
#endif

#define _(X1, X2, X3, X4, FMT)						\
	(NV50TIC_0_0_TYPER_UNORM | NV50TIC_0_0_TYPEG_UNORM |		\
	 NV50TIC_0_0_TYPEB_UNORM | NV50TIC_0_0_TYPEA_UNORM |		\
	 NV50TIC_0_0_MAP##X1 | NV50TIC_0_0_MAP##X2 |			\
	 NV50TIC_0_0_MAP##X3 | NV50TIC_0_0_MAP##X4 |			\
	 NV50TIC_0_0_FMT_##FMT)

#if !defined(__AROS__)
static Bool
NVC0EXAPictSolid(NVPtr pNv, PicturePtr ppict, unsigned unit)
{
	struct nouveau_pushbuf *push = pNv->pushbuf;

	PUSH_DATAu(push, pNv->scratch, SOLID(unit), 1);
	PUSH_DATA (push, ppict->pSourcePict->solidFill.color);
	PUSH_DATAu(push, pNv->scratch, TIC_OFFSET + (unit * 32), 8);
	PUSH_TIC  (push, pNv->scratch, SOLID(unit), 1, 1, 4,
		   _(B_C0, G_C1, R_C2, A_C3, 8_8_8_8));
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
NVC0EXAPictGradient(NVPtr pNv, PicturePtr ppict, unsigned unit)
{
	return FALSE;
}

static Bool
NVC0EXAPictTexture(NVPtr pNv, PixmapPtr ppix, PicturePtr ppict, unsigned unit)
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
	PUSH_TIC  (push, bo, 0, ppix->drawable.width, ppix->drawable.height, 0,
		   format);

	PUSH_DATAu(push, pNv->scratch, TSC_OFFSET + (unit * 32), 8);
	if (ppict->repeat) {
		switch (ppict->repeatType) {
		case RepeatPad:
			PUSH_DATA (push, 0x00024000 |
					 NV50TSC_1_0_WRAPS_CLAMP_TO_EDGE |
					 NV50TSC_1_0_WRAPT_CLAMP_TO_EDGE |
					 NV50TSC_1_0_WRAPR_CLAMP_TO_EDGE);
			break;
		case RepeatReflect:
			PUSH_DATA (push, 0x00024000 |
					 NV50TSC_1_0_WRAPS_MIRROR_REPEAT |
					 NV50TSC_1_0_WRAPT_MIRROR_REPEAT |
					 NV50TSC_1_0_WRAPR_MIRROR_REPEAT);
			break;
		case RepeatNormal:
		default:
			PUSH_DATA (push, 0x00024000 |
					 NV50TSC_1_0_WRAPS_REPEAT |
					 NV50TSC_1_0_WRAPT_REPEAT |
					 NV50TSC_1_0_WRAPR_REPEAT);
			break;
		}
	} else {
		PUSH_DATA (push, 0x00024000 |
				 NV50TSC_1_0_WRAPS_CLAMP_TO_BORDER |
				 NV50TSC_1_0_WRAPT_CLAMP_TO_BORDER |
				 NV50TSC_1_0_WRAPR_CLAMP_TO_BORDER);
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
NVC0EXAPicture(NVPtr pNv, PixmapPtr ppix, PicturePtr ppict, int unit)
{
	if (ppict->pDrawable)
		return NVC0EXAPictTexture(pNv, ppix, ppict, unit);

NOT_IMPLEMENTED_STOP
#if 0
	switch (ppict->pSourcePict->type) {
	case SourcePictTypeSolidFill:
		return NVC0EXAPictSolid(pNv, ppict, unit);
	case SourcePictTypeLinear:
		return NVC0EXAPictGradient(pNv, ppict, unit);
	default:
		break;
	}
#endif

	return FALSE;
}

#if !defined(__AROS__)
static Bool
NVC0EXACheckBlend(int op)
{
	if (op > PictOpAdd)
		NOUVEAU_FALLBACK("unsupported blend op %d\n", op);
	return TRUE;
}
#endif

static void
NVC0EXABlend(PixmapPtr ppix, PicturePtr ppict, int op, int component_alpha)
{
	NVC0EXA_LOCALS(ppix);
	struct nvc0_blend_op *b = &NVC0EXABlendOp[op];
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
		BEGIN_NVC0(push, NVC0_3D(BLEND_ENABLE(0)), 1);
		PUSH_DATA (push, 0);
	} else {
		BEGIN_NVC0(push, NVC0_3D(BLEND_ENABLE(0)), 1);
		PUSH_DATA (push, 1);
		BEGIN_NVC0(push, NVC0_3D(BLEND_EQUATION_RGB), 5);
		PUSH_DATA (push, NVC0_3D_BLEND_EQUATION_RGB_FUNC_ADD);
		PUSH_DATA (push, sblend);
		PUSH_DATA (push, dblend);
		PUSH_DATA (push, NVC0_3D_BLEND_EQUATION_ALPHA_FUNC_ADD);
		PUSH_DATA (push, sblend);
		BEGIN_NVC0(push, NVC0_3D(BLEND_FUNC_DST_ALPHA), 1);
		PUSH_DATA (push, dblend);
	}
}

#if !defined(__AROS__)
Bool
NVC0EXACheckComposite(int op,
		      PicturePtr pspict, PicturePtr pmpict, PicturePtr pdpict)
{
	if (!NVC0EXACheckBlend(op))
		NOUVEAU_FALLBACK("blend not supported\n");

	if (!NVC0EXACheckRenderTarget(pdpict))
		NOUVEAU_FALLBACK("render target invalid\n");

	if (!NVC0EXACheckTexture(pspict, pdpict, op))
		NOUVEAU_FALLBACK("src picture invalid\n");

	if (pmpict) {
		if (pmpict->componentAlpha &&
		    PICT_FORMAT_RGB(pmpict->format) &&
		    NVC0EXABlendOp[op].src_alpha &&
		    NVC0EXABlendOp[op].src_blend != BF(ZERO))
			NOUVEAU_FALLBACK("component-alpha not supported\n");

		if (!NVC0EXACheckTexture(pmpict, pdpict, op))
			NOUVEAU_FALLBACK("mask picture invalid\n");
	}

	return TRUE;
}
#endif

Bool
NVC0EXAPrepareComposite(int op,
			PicturePtr pspict, PicturePtr pmpict, PicturePtr pdpict,
			PixmapPtr pspix, PixmapPtr pmpix, PixmapPtr pdpix)
{
	struct nouveau_bo *dst = nouveau_pixmap_bo(pdpix);
	NVC0EXA_LOCALS(pdpix);

	if (!PUSH_SPACE(push, 256))
		NOUVEAU_FALLBACK("space\n");

	BEGIN_NVC0(push, SUBC_2D(NV50_GRAPH_SERIALIZE), 1);
	PUSH_DATA (push, 0);

	if (!NVC0EXARenderTarget(pdpix, pdpict))
		NOUVEAU_FALLBACK("render target invalid\n");

	NVC0EXABlend(pdpix, pdpict, op, pmpict && pmpict->componentAlpha &&
		     PICT_FORMAT_RGB(pmpict->format));

	if (!NVC0EXAPicture(pNv, pspix, pspict, 0))
		NOUVEAU_FALLBACK("src picture invalid\n");

	if (pmpict) {
		if (!NVC0EXAPicture(pNv, pmpix, pmpict, 1))
			NOUVEAU_FALLBACK("mask picture invalid\n");

		BEGIN_NVC0(push, NVC0_3D(SP_START_ID(5)), 1);
		if (pdpict->format == PICT_a8) {
			PUSH_DATA (push, PFP_C_A8);
		} else {
			if (pmpict->componentAlpha &&
			    PICT_FORMAT_RGB(pmpict->format)) {
				if (NVC0EXABlendOp[op].src_alpha)
					PUSH_DATA (push, PFP_CCASA);
				else
					PUSH_DATA (push, PFP_CCA);
			} else {
				PUSH_DATA (push, PFP_C);
			}
		}
	} else {
		BEGIN_NVC0(push, NVC0_3D(SP_START_ID(5)), 1);
		if (pdpict->format == PICT_a8)
			PUSH_DATA (push, PFP_S_A8);
		else
			PUSH_DATA (push, PFP_S);
	}

	BEGIN_NVC0(push, NVC0_3D(TSC_FLUSH), 1);
	PUSH_DATA (push, 0);
	BEGIN_NVC0(push, NVC0_3D(TIC_FLUSH), 1);
	PUSH_DATA (push, 0);
	BEGIN_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 1);
	PUSH_DATA (push, 0);

	PUSH_RESET(push);
	PUSH_REFN (push, pNv->scratch, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	if (pspict->pDrawable)
		PUSH_REFN (push, nouveau_pixmap_bo(pspix),
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	PUSH_REFN (push, dst, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	if (pmpict && pmpict->pDrawable)
		PUSH_REFN (push, nouveau_pixmap_bo(pmpix),
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		NOUVEAU_FALLBACK("validate\n");
	}

	return TRUE;
}

void
NVC0EXAComposite(PixmapPtr pdpix,
		 int sx, int sy, int mx, int my,
		 int dx, int dy, int w, int h)
{
	NVC0EXA_LOCALS(pdpix);

	if (!PUSH_SPACE(push, 64))
		return;

	if (pNv->dev->chipset >= 0x110) {
		BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
		PUSH_DATA (push, 256);
		PUSH_DATA (push, (pNv->scratch->offset + PVP_DATA) >> 32);
		PUSH_DATA (push, (pNv->scratch->offset + PVP_DATA));
		BEGIN_1IC0(push, NVC0_3D(CB_POS), 24 + 1);
		PUSH_DATA (push, 0x80);

		PUSH_DATAf(push, dx);
		PUSH_DATAf(push, dy + (h * 2));
		PUSH_DATAf(push, 0);
		PUSH_DATAf(push, 1);
		PUSH_DATAf(push, sx);
		PUSH_DATAf(push, sy + (h * 2));
		PUSH_DATAf(push, mx);
		PUSH_DATAf(push, my + (h * 2));

		PUSH_DATAf(push, dx);
		PUSH_DATAf(push, dy);
		PUSH_DATAf(push, 0);
		PUSH_DATAf(push, 1);
		PUSH_DATAf(push, sx);
		PUSH_DATAf(push, sy);
		PUSH_DATAf(push, mx);
		PUSH_DATAf(push, my);

		PUSH_DATAf(push, dx + (w * 2));
		PUSH_DATAf(push, dy);
		PUSH_DATAf(push, 0);
		PUSH_DATAf(push, 1);
		PUSH_DATAf(push, sx + (w * 2));
		PUSH_DATAf(push, sy);
		PUSH_DATAf(push, mx + (w * 2));
		PUSH_DATAf(push, my);
	}

	BEGIN_NVC0(push, NVC0_3D(SCISSOR_HORIZ(0)), 2);
	PUSH_DATA (push, ((dx + w) << 16) | dx);
	PUSH_DATA (push, ((dy + h) << 16) | dy);
	BEGIN_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), 1);
	PUSH_DATA (push, NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_TRIANGLES);
	if (pNv->dev->chipset < 0x110) {
		PUSH_VTX2s(push, sx, sy + (h * 2), mx, my + (h * 2), dx, dy + (h * 2));
		PUSH_VTX2s(push, sx, sy, mx, my, dx, dy);
		PUSH_VTX2s(push, sx + (w * 2), sy, mx + (w * 2), my, dx + (w * 2), dy);
	} else {
		BEGIN_NVC0(push, NVC0_3D(VERTEX_BUFFER_FIRST), 2);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, 3);
	}
	BEGIN_NVC0(push, NVC0_3D(VERTEX_END_GL), 1);
	PUSH_DATA (push, 0);
}

void
NVC0EXADoneComposite(PixmapPtr pdpix)
{
	NVC0EXA_LOCALS(pdpix);
	nouveau_pushbuf_bufctx(push, NULL);
}

Bool
NVC0EXARectM2MF(NVPtr pNv, int w, int h, int cpp,
		struct nouveau_bo *src, uint32_t src_off, int src_dom,
		int src_pitch, int src_h, int src_x, int src_y,
		struct nouveau_bo *dst, uint32_t dst_off, int dst_dom,
		int dst_pitch, int dst_h, int dst_x, int dst_y)
{
	struct nouveau_pushbuf_refn refs[] = {
		{ src, src_dom | NOUVEAU_BO_RD },
		{ dst, dst_dom | NOUVEAU_BO_WR },
	};
	struct nouveau_pushbuf *push = pNv->pushbuf;
	unsigned exec = 0;

	if (!PUSH_SPACE(push, 64))
		return FALSE;

	if (src->config.nvc0.memtype) {
		BEGIN_NVC0(push, NVC0_M2MF(TILING_MODE_IN), 5);
		PUSH_DATA (push, src->config.nvc0.tile_mode);
		PUSH_DATA (push, src_pitch);
		PUSH_DATA (push, src_h);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 0);
	} else {
		BEGIN_NVC0(push, NVC0_M2MF(PITCH_IN), 1);
		PUSH_DATA (push, src_pitch);

		src_off += src_y * src_pitch + src_x * cpp;
		exec |= NVC0_M2MF_EXEC_LINEAR_IN;
	}

	if (dst->config.nvc0.memtype) {
		BEGIN_NVC0(push, NVC0_M2MF(TILING_MODE_OUT), 5);
		PUSH_DATA (push, dst->config.nvc0.tile_mode);
		PUSH_DATA (push, dst_pitch);
		PUSH_DATA (push, dst_h);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 0);
	} else {
		BEGIN_NVC0(push, NVC0_M2MF(PITCH_OUT), 1);
		PUSH_DATA (push, dst_pitch);

		dst_off += dst_y * dst_pitch + dst_x * cpp;
		exec |= NVC0_M2MF_EXEC_LINEAR_OUT;
	}

	while (h) {
		int line_count = h;
		if (line_count > 2047)
			line_count = 2047;

		if (nouveau_pushbuf_space(push, 32, 0, 0) ||
		    nouveau_pushbuf_refn (push, refs, 2))
			return FALSE;

		BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
		PUSH_DATA (push, (dst->offset + dst_off) >> 32);
		PUSH_DATA (push, (dst->offset + dst_off));
		BEGIN_NVC0(push, NVC0_M2MF(OFFSET_IN_HIGH), 2);
		PUSH_DATA (push, (src->offset + src_off) >> 32);
		PUSH_DATA (push, (src->offset + src_off));

		if (src->config.nvc0.memtype) {
			BEGIN_NVC0(push, NVC0_M2MF(TILING_POSITION_IN_X), 2);
			PUSH_DATA (push, src_x * cpp);
			PUSH_DATA (push, src_y);
		} else {
			src_off += line_count * src_pitch;
		}

		if (dst->config.nvc0.memtype) {
			BEGIN_NVC0(push, NVC0_M2MF(TILING_POSITION_OUT_X), 2);
			PUSH_DATA (push, dst_x * cpp);
			PUSH_DATA (push, dst_y);
		} else {
			dst_off += line_count * dst_pitch;
		}

		BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
		PUSH_DATA (push, w * cpp);
		PUSH_DATA (push, line_count);
		BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
		PUSH_DATA (push, NVC0_M2MF_EXEC_QUERY_SHORT | exec);

		src_y += line_count;
		dst_y += line_count;
		h  -= line_count;
	}

	return TRUE;
}

Bool
NVE0EXARectCopy(NVPtr pNv, int w, int h, int cpp,
		struct nouveau_bo *src, uint32_t src_off, int src_dom,
		int src_pitch, int src_h, int src_x, int src_y,
		struct nouveau_bo *dst, uint32_t dst_off, int dst_dom,
		int dst_pitch, int dst_h, int dst_x, int dst_y)
{
	return nouveau_copya0b5_rect(pNv->pushbuf, pNv->NvCOPY, w, h, cpp,
				     src, src_off, src_dom, src_pitch,
				     src_h, src_x, src_y, dst, dst_off,
				     dst_dom, dst_pitch, dst_h, dst_x, dst_y);
}

/* AROS CODE */

VOID HIDDNouveauNVC0SetPattern(struct CardData * carddata, LONG clr0, LONG clr1,
		  LONG pat0, LONG pat1)
{
    NVC0EXASetPattern(NULL, clr0, clr1, pat0, pat1);
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HIDDNouveauNVC0FillSolidRect(struct CardData * carddata,
    struct HIDDNouveauBitMapData * bmdata, LONG minX, LONG minY, LONG maxX,
    LONG maxY, ULONG drawmode, ULONG color)
{
    if (NVC0EXAPrepareSolid(bmdata, drawmode, ~0, color))
    {
        NVC0EXASolid(bmdata, minX, minY, maxX + 1, maxY + 1);
        return TRUE;
    }

    return FALSE;
}

/* NOTE: Assumes lock on both bitmaps is already made */
/* NOTE: Assumes both buffers are not mapped */
BOOL HIDDNouveauNVC0CopySameFormat(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG drawmode)
{
    if (NVC0EXAPrepareCopy(srcdata, destdata, 0, 0, drawmode, ~0))
    {
        NVC0EXACopy(destdata, srcX, srcY, destX , destY, width, height);
        return TRUE;
    }

    return FALSE;
}