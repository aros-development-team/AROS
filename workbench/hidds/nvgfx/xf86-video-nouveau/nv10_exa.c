/*
 * Copyright 2007 Stephane Marchesin
 * Copyright 2007 Arthur Huillet
 * Copyright 2007 Peter Winters
 * Copyright 2009 Francisco Jerez
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "nv_include.h"

#include "hwdefs/nv_object.xml.h"
#include "hwdefs/nv10_3d.xml.h"
#include "nv04_accel.h"

/* Texture/Render target formats. */
static struct pict_format {
	int exa;
	int hw;
} nv10_tex_format_pot[] = {
	{ PICT_a8,       0x80  },
	{ PICT_r5g6b5,	 0x280 },
	{ PICT_x8r8g8b8, 0x300 },
	{ PICT_a8r8g8b8, 0x300 },
	{},

}, nv10_tex_format_rect[] = {
	{ PICT_a8,       0x980 },
	{ PICT_r5g6b5,   0x880 },
	{ PICT_x8r8g8b8, 0x900 },
	{ PICT_a8r8g8b8, 0x900 },
	{},

}, nv20_tex_format_rect[] = {
	{ PICT_a8,	 0xd80 },
	{ PICT_r5g6b5,   0x880 },
	{ PICT_x8r8g8b8, 0x900 },
	{ PICT_a8r8g8b8, 0x900 },
	{},

}, nv10_rt_format[] = {
	{ PICT_r5g6b5,	 0x103 },
	{ PICT_x8r8g8b8, 0x108 },
	{ PICT_a8r8g8b8, 0x108 },
	{},
};

static int
get_tex_format(NVPtr pNv, PicturePtr pict)
{
	/* If repeat is set we're always handling a 1x1 texture with
	 * ARGB/XRGB destination, in that case we change the format to
	 * use the POT (swizzled) matching format.
	 */
	struct pict_format *format =
		pict->repeat != RepeatNone ? nv10_tex_format_pot :
		pNv->Architecture == NV_ARCH_20 ? nv20_tex_format_rect :
		nv10_tex_format_rect;

	for (; format->hw; format++) {
		if (format->exa == pict->format)
			return format->hw;
	}

	return 0;
}

static int
get_rt_format(PicturePtr pict)
{
	struct pict_format *format = nv10_rt_format;

	for (; format->hw; format++) {
		if (format->exa == pict->format)
			return format->hw;
	}

	return 0;
}

/* Blending functions. */
#define SF(x) NV10_3D_BLEND_FUNC_SRC_##x
#define DF(x) NV10_3D_BLEND_FUNC_DST_##x

static struct pict_op {
	int src;
	int dst;

} nv10_pict_op[] = {
	{ SF(ZERO),		   DF(ZERO) },		      /* Clear */
	{ SF(ONE),		   DF(ZERO) },		      /* Src */
	{ SF(ZERO),		   DF(ONE) },		      /* Dst */
	{ SF(ONE),		   DF(ONE_MINUS_SRC_ALPHA) }, /* Over */
	{ SF(ONE_MINUS_DST_ALPHA), DF(ONE) },		      /* OverReverse */
	{ SF(DST_ALPHA),	   DF(ZERO) },                /* In */
	{ SF(ZERO),		   DF(SRC_ALPHA) },           /* InReverse */
	{ SF(ONE_MINUS_DST_ALPHA), DF(ZERO) },		      /* Out */
	{ SF(ZERO),		   DF(ONE_MINUS_SRC_ALPHA) }, /* OutReverse */
	{ SF(DST_ALPHA),	   DF(ONE_MINUS_SRC_ALPHA) }, /* Atop */
	{ SF(ONE_MINUS_DST_ALPHA), DF(SRC_ALPHA) },	      /* AtopReverse */
	{ SF(ONE_MINUS_DST_ALPHA), DF(ONE_MINUS_SRC_ALPHA) }, /* Xor */
	{ SF(ONE),		   DF(ONE) },		      /* Add */
};

static inline Bool
needs_src_alpha(int op)
{
	return nv10_pict_op[op].dst == DF(ONE_MINUS_SRC_ALPHA)
		|| nv10_pict_op[op].dst == DF(SRC_ALPHA);
}

static inline Bool
needs_src(int op)
{
	return nv10_pict_op[op].src != SF(ZERO);
}

static inline Bool
effective_component_alpha(PicturePtr mask)
{
	return mask && mask->componentAlpha && PICT_FORMAT_RGB(mask->format);
}

#if !defined(__AROS__)
static Bool
check_texture(NVPtr pNv, PicturePtr pict)
{
	int w = 1, h = 1;

	if (pict->pDrawable) {
		w = pict->pDrawable->width;
		h = pict->pDrawable->height;
	} else {
		if (pict->pSourcePict->type != SourcePictTypeSolidFill)
			NOUVEAU_FALLBACK("gradient pictures unsupported\n");
	}

	if (w > 2046 || h > 2046)
		NOUVEAU_FALLBACK("picture too large, %dx%d\n", w, h);

	if (!get_tex_format(pNv, pict))
		return FALSE;

	if (pict->filter != PictFilterNearest &&
	    pict->filter != PictFilterBilinear)
		return FALSE;

	/* We cannot repeat on NV10 because NPOT textures do not
	 * support this. unfortunately. */
	if (pict->repeat != RepeatNone)
		/* we can repeat 1x1 textures */
		if (!(w == 1 && h == 1))
			return FALSE;

	return TRUE;
}

static Bool
check_render_target(PicturePtr pict)
{
	int w = pict->pDrawable->width;
	int h = pict->pDrawable->height;

	if (w > 4096 || h > 4096)
		return FALSE;

	if (!get_rt_format(pict))
		return FALSE;

	return TRUE;
}

static Bool
check_pict_op(int op)
{
	/* We do no saturate, disjoint, conjoint, though we
	 * could do e.g. DisjointClear which really is
	 * Clear. */
	return op < PictOpSaturate;
}

#if 0
static void
print_fallback_info(char *reason, int op, PicturePtr src, PicturePtr mask,
		    PicturePtr dst)
{
	char out2[4096];
	char *out = out2;

	sprintf(out, "%s  ", reason);
	out += strlen(out);

	switch (op) {
	case PictOpClear:
		sprintf(out, "PictOpClear ");
		break;
	case PictOpSrc:
		sprintf(out, "PictOpSrc ");
		break;
	case PictOpDst:
		sprintf(out, "PictOpDst ");
		break;
	case PictOpOver:
		sprintf(out, "PictOpOver ");
		break;
	case PictOpOutReverse:
		sprintf(out, "PictOpOutReverse ");
		break;
	case PictOpAdd:
		sprintf(out, "PictOpAdd ");
		break;
	default:
		sprintf(out, "PictOp%d ", op);
	}
	out += strlen(out);

	switch (src->format) {
	case PICT_a8r8g8b8:
		sprintf(out, "A8R8G8B8 ");
		break;
	case PICT_x8r8g8b8:
		sprintf(out, "X8R8G8B8 ");
		break;
	case PICT_x8b8g8r8:
		sprintf(out, "X8B8G8R8 ");
		break;
	case PICT_r5g6b5:
		sprintf(out, "R5G6B5 ");
		break;
	case PICT_a8:
		sprintf(out, "A8 ");
		break;
	case PICT_a1:
		sprintf(out, "A1 ");
		break;
	default:
		sprintf(out, "%x ", src->format);
	}
	out += strlen(out);

	sprintf(out, "(%dx%d) ", src->pDrawable->width,
		src->pDrawable->height);
	if (src->repeat != RepeatNone)
		strcat(out, "R ");
	strcat(out, "-> ");
	out += strlen(out);

	switch (dst->format) {
	case PICT_a8r8g8b8:
		sprintf(out, "A8R8G8B8 ");
		break;
	case PICT_x8r8g8b8:
		sprintf(out, "X8R8G8B8  ");
		break;
	case PICT_x8b8g8r8:
		sprintf(out, "X8B8G8R8  ");
		break;
	case PICT_r5g6b5:
		sprintf(out, "R5G6B5 ");
		break;
	case PICT_a8:
		sprintf(out, "A8  ");
		break;
	case PICT_a1:
		sprintf(out, "A1  ");
		break;
	default:
		sprintf(out, "%x  ", dst->format);
	}
	out += strlen(out);

	sprintf(out, "(%dx%d) ", dst->pDrawable->width,
		dst->pDrawable->height);
	if (dst->repeat != RepeatNone)
		strcat(out, "R ");
	out += strlen(out);

	if (!mask)
		sprintf(out, "& NONE");
	else {
		switch (mask->format) {
		case PICT_a8r8g8b8:
			sprintf(out, "& A8R8G8B8 ");
			break;
		case PICT_x8r8g8b8:
			sprintf(out, "& X8R8G8B8  ");
			break;
		case PICT_x8b8g8r8:
			sprintf(out, "& X8B8G8R8  ");
			break;
		case PICT_a8:
			sprintf(out, "& A8  ");
			break;
		case PICT_a1:
			sprintf(out, "& A1  ");
			break;
		default:
			sprintf(out, "& %x  ", mask->format);
		}
		out += strlen(out);

		sprintf(out, "(%dx%d) ", mask->pDrawable->width,
			mask->pDrawable->height);
		if (mask->repeat != RepeatNone)
			strcat(out, "R ");
		if (mask->componentAlpha)
			strcat(out, "C ");
		out += strlen(out);
	}
	strcat(out, "\n");

	xf86DrvMsg(0, X_INFO, "%s", out2);
}
#else
#define print_fallback_info(...)
#endif

Bool
NV10EXACheckComposite(int op, PicturePtr src, PicturePtr mask, PicturePtr dst)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(dst->pDrawable->pScreen);
	NVPtr pNv = NVPTR(pScrn);

	if (!check_pict_op(op)) {
		print_fallback_info("pictop", op, src, mask, dst);
		return FALSE;
	}

	if (!check_render_target(dst)) {
		print_fallback_info("dst", op, src, mask, dst);
		return FALSE;
	}

	if (!check_texture(pNv, src)) {
		print_fallback_info("src", op, src, mask, dst);
		return FALSE;
	}

	if (mask) {
		if (!check_texture(pNv, mask)) {
			print_fallback_info("mask", op, src,
					    mask, dst);
			return FALSE;
		}

		if (effective_component_alpha(mask) &&
		    needs_src(op) && needs_src_alpha(op)) {
			print_fallback_info("ca-mask", op, src,
					    mask, dst);
			return FALSE;
		}
	}

	print_fallback_info("Accelerating", op, src, mask, dst);
	return TRUE;
}
#endif

static Bool
setup_texture(NVPtr pNv, int unit, PicturePtr pict, PixmapPtr pixmap)
{
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pixmap);
	unsigned reloc = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;
	unsigned h = pict->pDrawable->height;
	unsigned w = pict->pDrawable->width;
	unsigned format;

	format = NV10_3D_TEX_FORMAT_WRAP_T_CLAMP_TO_EDGE |
		 NV10_3D_TEX_FORMAT_WRAP_S_CLAMP_TO_EDGE |
		 log2i(w) << 20 | log2i(h) << 16 |
		 1 << 12 | /* lod == 1 */
		 get_tex_format(pNv, pict) |
		 0x50 /* UNK */;

	/* NPOT_SIZE expects an even number for width, we can round up uneven
	 * numbers here because EXA always gives 64 byte aligned pixmaps and
	 * for all formats we support 64 bytes represents an even number of
	 * pixels
	 */
	w = (w + 1) & ~1;

	BEGIN_NV04(push, NV10_3D(TEX_OFFSET(unit)), 1);
	PUSH_MTHDl(push, NV10_3D(TEX_OFFSET(unit)), bo, 0, reloc);
	BEGIN_NV04(push, NV10_3D(TEX_FORMAT(unit)), 1);
	PUSH_MTHDs(push, NV10_3D(TEX_FORMAT(unit)), bo, format, reloc,
			 NV10_3D_TEX_FORMAT_DMA0,
			 NV10_3D_TEX_FORMAT_DMA1);
	BEGIN_NV04(push, NV10_3D(TEX_ENABLE(unit)), 1 );
	PUSH_DATA (push, NV10_3D_TEX_ENABLE_ENABLE);
	BEGIN_NV04(push, NV10_3D(TEX_NPOT_PITCH(unit)), 1);
	PUSH_DATA (push, exaGetPixmapPitch(pixmap) << 16);
	BEGIN_NV04(push, NV10_3D(TEX_NPOT_SIZE(unit)), 1);
	PUSH_DATA (push, (w << 16) | h);
	BEGIN_NV04(push, NV10_3D(TEX_FILTER(unit)), 1);
	if (pict->filter == PictFilterNearest)
		PUSH_DATA(push, NV10_3D_TEX_FILTER_MAGNIFY_NEAREST |
				NV10_3D_TEX_FILTER_MINIFY_NEAREST);
	else
		PUSH_DATA(push, NV10_3D_TEX_FILTER_MAGNIFY_LINEAR |
				NV10_3D_TEX_FILTER_MINIFY_LINEAR);
#if !defined(__AROS__)
	if (pict->transform) {
		BEGIN_NV04(push, NV10_3D(TEX_MATRIX_ENABLE(unit)), 1);
		PUSH_DATA (push, 1);
		BEGIN_NV04(push, NV10_3D(TEX_MATRIX(unit, 0)), 16);
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[0][0]));
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[0][1]));
		PUSH_DATAf(push, 0.f);
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[0][2]));
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[1][0]));
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[1][1]));
		PUSH_DATAf(push, 0.f);
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[1][2]));
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[2][0]));
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[2][1]));
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, xFixedToFloat(pict->transform->matrix[2][2]));
	} else {
#else
	{ // we are not doing any transformations
#endif
		BEGIN_NV04(push, NV10_3D(TEX_MATRIX_ENABLE(unit)), 1);
		PUSH_DATA (push, 0);
	}

	return TRUE;
}

static Bool
setup_render_target(NVPtr pNv, PicturePtr pict, PixmapPtr pixmap)
{
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pixmap);

	BEGIN_NV04(push, NV10_3D(RT_FORMAT), 3);
	PUSH_DATA (push, get_rt_format(pict));
	PUSH_DATA (push, (exaGetPixmapPitch(pixmap) << 16 |
			  exaGetPixmapPitch(pixmap)));
	PUSH_MTHDl(push, NV10_3D(COLOR_OFFSET), bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	return TRUE;
}

static void
setup_blend_function(NVPtr pNv, PicturePtr pdpict, PicturePtr pmpict, int alu)
{
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct pict_op *op = &nv10_pict_op[alu];
	int src_factor = op->src;
	int dst_factor = op->dst;

	if (src_factor == SF(ONE_MINUS_DST_ALPHA) &&
	    !PICT_FORMAT_A(pdpict->format))
		/* ONE_MINUS_DST_ALPHA doesn't always do the right thing for
		 * framebuffers without alpha channel. But it's the same as
		 * ZERO in that case.
		 */
		src_factor = SF(ZERO);

	if (effective_component_alpha(pmpict)) {
		if (dst_factor == DF(SRC_ALPHA))
			dst_factor = DF(SRC_COLOR);
		else if (dst_factor == DF(ONE_MINUS_SRC_ALPHA))
			dst_factor = DF(ONE_MINUS_SRC_COLOR);
	}

	BEGIN_NV04(push, NV10_3D(BLEND_FUNC_SRC), 2);
	PUSH_DATA (push, src_factor);
	PUSH_DATA (push, dst_factor);
	BEGIN_NV04(push, NV10_3D(BLEND_FUNC_ENABLE), 1);
	PUSH_DATA (push, 1);
}

#define RCSRC_COL(i)  (0x01 + (unit))
#define RCSRC_TEX(i)  (0x08 + (unit))
#define RCSEL_COLOR   (0x00)
#define RCSEL_ALPHA   (0x10)
#define RCINP_ZERO    (0x00)
#define RCINP_ONE     (0x20)
#define RCINP_A__SHIFT 24
#define RCINP_B__SHIFT 16

static Bool
setup_picture(NVPtr pNv, PicturePtr pict, PixmapPtr pixmap, int unit,
	      uint32_t *color, uint32_t *alpha)
{
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t shift, source;

	if (pict && pict->pDrawable) {
		if (!setup_texture(pNv, unit, pict, pixmap))
			return FALSE;
		source = RCSRC_TEX(unit);
	} else
	if (pict) {
NOT_IMPLEMENTED_STOP
#if 0
		BEGIN_NV04(push, NV10_3D(RC_COLOR(unit)), 1);
		PUSH_DATA (push, pict->pSourcePict->solidFill.color);
		source = RCSRC_COL(unit);
#endif
	}

	if (pict && PICT_FORMAT_RGB(pict->format))
		*color = RCSEL_COLOR | source;
	else
		*color = RCSEL_COLOR | RCINP_ZERO;

	if (pict && PICT_FORMAT_A(pict->format))
		*alpha = RCSEL_ALPHA | source;
	else
		*alpha = RCSEL_ALPHA | RCINP_ONE;

	if (unit)
		shift = RCINP_B__SHIFT;
	else
		shift = RCINP_A__SHIFT;
	*color <<= shift;
	*alpha <<= shift;
	return TRUE;
}

Bool
NV10EXAPrepareComposite(int op,
			PicturePtr pict_src,
			PicturePtr pict_mask,
			PicturePtr pict_dst,
			PixmapPtr src,
			PixmapPtr mask,
			PixmapPtr dst)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(dst->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t sc, sa, mc, ma;

	if (!PUSH_SPACE(push, 128))
		return FALSE;
	PUSH_RESET(push);

	/* setup render target and blending */
	if (!setup_render_target(pNv, pict_dst, dst))
		return FALSE;
	setup_blend_function(pNv, pict_dst, pict_mask, op);

	/* select picture sources */
	if (!setup_picture(pNv, pict_src, src, 0, &sc, &sa))
		return FALSE;
	if (!setup_picture(pNv, pict_mask, mask, 1, &mc, &ma))
		return FALSE;

	/* configure register combiners */
	BEGIN_NV04(push, NV10_3D(RC_IN_ALPHA(0)), 1);
	PUSH_DATA (push, sa | ma);
	BEGIN_NV04(push, NV10_3D(RC_IN_RGB(0)), 1);
	if (effective_component_alpha(pict_mask)) {
		if (needs_src_alpha(op))
			PUSH_DATA(push, sa | mc);
		else
			PUSH_DATA(push, sc | mc);
	} else {
		PUSH_DATA(push, sc | ma);
	}

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		return FALSE;
	}

#if !defined(__AROS__)
	pNv->pspict = pict_src;
	pNv->pmpict = pict_mask;
#endif
	return TRUE;
}

static inline void
PUSH_VTX2s(struct nouveau_pushbuf *push,
	   int x1, int y1, int x2, int y2, int dx, int dy)
{
	BEGIN_NV04(push, NV10_3D(VERTEX_TX0_2I), 1);
	PUSH_DATA (push, ((y1 & 0xffff) << 16) | (x1 & 0xffff));
	BEGIN_NV04(push, NV10_3D(VERTEX_TX1_2I), 1);
	PUSH_DATA (push, ((y2 & 0xffff) << 16) | (x2 & 0xffff));
	BEGIN_NV04(push, NV10_3D(VERTEX_POS_3F_X), 3);
	PUSH_DATAf(push, dx);
	PUSH_DATAf(push, dy);
	PUSH_DATAf(push, 0.0);
}

void
NV10EXAComposite(PixmapPtr pix_dst,
		 int sx, int sy, int mx, int my, int dx, int dy, int w, int h)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pix_dst->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (!PUSH_SPACE(push, 64))
		return;

	BEGIN_NV04(push, NV10_3D(VERTEX_BEGIN_END), 1);
	PUSH_DATA (push, NV10_3D_VERTEX_BEGIN_END_QUADS);
	PUSH_VTX2s(push, sx, sy, mx, my, dx, dy);
	PUSH_VTX2s(push, sx + w, sy, mx + w, my, dx + w, dy);
	PUSH_VTX2s(push, sx + w, sy + h, mx + w, my + h, dx + w, dy + h);
	PUSH_VTX2s(push, sx, sy + h, mx, my + h, dx, dy + h);
	BEGIN_NV04(push, NV10_3D(VERTEX_BEGIN_END), 1);
	PUSH_DATA (push, NV10_3D_VERTEX_BEGIN_END_STOP);
}

void
NV10EXADoneComposite(PixmapPtr dst)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(dst->drawable.pScreen);
	nouveau_pushbuf_bufctx(NVPTR(pScrn)->pushbuf, NULL);
}

Bool
NVAccelInitNV10TCL(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nv04_fifo *fifo = pNv->channel->data;
	uint32_t class = 0;
	int i;

	if (((pNv->dev->chipset & 0xf0) != NV_ARCH_10) &&
	    ((pNv->dev->chipset & 0xf0) != NV_ARCH_20))
		return FALSE;

	if (pNv->dev->chipset >= 0x20 || pNv->dev->chipset == 0x1a)
		class = NV15_3D_CLASS;
	else if (pNv->dev->chipset >= 0x17)
		class = NV17_3D_CLASS;
	else if (pNv->dev->chipset >= 0x11)
		class = NV15_3D_CLASS;
	else
		class = NV10_3D_CLASS;

	if (nouveau_object_new(pNv->channel, Nv3D, class, NULL, 0, &pNv->Nv3D))
		return FALSE;

	if (!PUSH_SPACE(push, 256))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(3D, OBJECT), 1);
	PUSH_DATA (push, pNv->Nv3D->handle);
	BEGIN_NV04(push, NV10_3D(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->NvNull->handle);

	BEGIN_NV04(push, NV10_3D(DMA_TEXTURE0), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->gart);

	BEGIN_NV04(push, NV10_3D(DMA_COLOR), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->vram);

	BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(RT_HORIZ), 2);
	PUSH_DATA (push, 2048 << 16 | 0);
	PUSH_DATA (push, 2048 << 16 | 0);

	BEGIN_NV04(push, NV10_3D(ZETA_OFFSET), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_MODE), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_HORIZ(0)), 1);
	PUSH_DATA (push, 0x7ff << 16 | 0x800);
	BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_VERT(0)), 1);
	PUSH_DATA (push, 0x7ff << 16 | 0x800);

	for (i = 1; i < 8; i++) {
		BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_HORIZ(i)), 1);
		PUSH_DATA (push, 0);
		BEGIN_NV04(push, NV10_3D(VIEWPORT_CLIP_VERT(i)), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV10_3D(UNK0290), 1);
	PUSH_DATA (push, (0x10<<16)|1);
	BEGIN_NV04(push, NV10_3D(UNK03F4), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
	PUSH_DATA (push, 0);

	if (class != NV10_3D_CLASS) {
		/* For nv11, nv17 */
		BEGIN_NV04(push, SUBC_3D(NV15_3D_FLIP_SET_READ), 3);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 2);

		BEGIN_NV04(push, NV15_BLIT(FLIP_SET_READ), 3);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 2);

		BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
	PUSH_DATA (push, 0);

	/* Set state */
	BEGIN_NV04(push, NV10_3D(FOG_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(ALPHA_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(ALPHA_FUNC_FUNC), 2);
	PUSH_DATA (push, 0x207);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(TEX_ENABLE(0)), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(RC_IN_ALPHA(0)), 6);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(RC_OUT_ALPHA(0)), 6);
	PUSH_DATA (push, 0x00000c00);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0x00000c00);
	PUSH_DATA (push, 0x18000000);
	PUSH_DATA (push, 0x300c0000);
	PUSH_DATA (push, 0x00001c80);
	BEGIN_NV04(push, NV10_3D(BLEND_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(DITHER_ENABLE), 2);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(LINE_SMOOTH_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(VERTEX_WEIGHT_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(BLEND_FUNC_SRC), 4);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0x8006);
	BEGIN_NV04(push, NV10_3D(STENCIL_MASK), 8);
	PUSH_DATA (push, 0xff);
	PUSH_DATA (push, 0x207);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0xff);
	PUSH_DATA (push, 0x1e00);
	PUSH_DATA (push, 0x1e00);
	PUSH_DATA (push, 0x1e00);
	PUSH_DATA (push, 0x1d01);
	BEGIN_NV04(push, NV10_3D(NORMALIZE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(FOG_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(LIGHT_MODEL), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(SEPARATE_SPECULAR_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(ENABLED_LIGHTS), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(POLYGON_OFFSET_POINT_ENABLE), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(DEPTH_FUNC), 1);
	PUSH_DATA (push, 0x201);
	BEGIN_NV04(push, NV10_3D(DEPTH_WRITE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(DEPTH_TEST_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(POLYGON_OFFSET_FACTOR), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(POINT_SIZE), 1);
	PUSH_DATA (push, 8);
	BEGIN_NV04(push, NV10_3D(POINT_PARAMETERS_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(LINE_WIDTH), 1);
	PUSH_DATA (push, 8);
	BEGIN_NV04(push, NV10_3D(LINE_SMOOTH_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(POLYGON_MODE_FRONT), 2);
	PUSH_DATA (push, 0x1b02);
	PUSH_DATA (push, 0x1b02);
	BEGIN_NV04(push, NV10_3D(CULL_FACE), 2);
	PUSH_DATA (push, 0x405);
	PUSH_DATA (push, 0x901);
	BEGIN_NV04(push, NV10_3D(POLYGON_SMOOTH_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(CULL_FACE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(TEX_GEN_MODE(0, 0)), 8);
	for (i = 0; i < 8; i++)
		PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(FOG_COEFF(0)), 3);
	PUSH_DATA (push, 0x3fc00000);	/* -1.50 */
	PUSH_DATA (push, 0xbdb8aa0a);	/* -0.09 */
	PUSH_DATA (push, 0);		/*  0.00 */

	BEGIN_NV04(push, NV04_GRAPH(3D, NOP), 1);
	PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV10_3D(FOG_MODE), 2);
	PUSH_DATA (push, 0x802);
	PUSH_DATA (push, 2);
	/* for some reason VIEW_MATRIX_ENABLE need to be 6 instead of 4 when
	 * using texturing, except when using the texture matrix
	 */
	BEGIN_NV04(push, NV10_3D(VIEW_MATRIX_ENABLE), 1);
	PUSH_DATA (push, 6);
	BEGIN_NV04(push, NV10_3D(COLOR_MASK), 1);
	PUSH_DATA (push, 0x01010101);

	BEGIN_NV04(push, NV10_3D(PROJECTION_MATRIX(0)), 16);
	for(i = 0; i < 16; i++)
		PUSH_DATAf(push, i/4 == i%4 ? 1.0 : 0.0);

	BEGIN_NV04(push, NV10_3D(DEPTH_RANGE_NEAR), 2);
	PUSH_DATA (push, 0);
	PUSH_DATAf(push, 65536.0);

	BEGIN_NV04(push, NV10_3D(VIEWPORT_TRANSLATE_X), 4);
	PUSH_DATAf(push, -2048.0);
	PUSH_DATAf(push, -2048.0);
	PUSH_DATAf(push, 0);
	PUSH_DATA (push, 0);

	/* Set vertex component */
	BEGIN_NV04(push, NV10_3D(VERTEX_COL_4F_R), 4);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	BEGIN_NV04(push, NV10_3D(VERTEX_COL2_3F_R), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV10_3D(VERTEX_NOR_3F_X), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATAf(push, 1.0);
	BEGIN_NV04(push, NV10_3D(VERTEX_TX0_4F_S), 4);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	BEGIN_NV04(push, NV10_3D(VERTEX_TX1_4F_S), 4);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	BEGIN_NV04(push, NV10_3D(VERTEX_FOG_1F), 1);
	PUSH_DATAf(push, 0.0);
	BEGIN_NV04(push, NV10_3D(EDGEFLAG_ENABLE), 1);
	PUSH_DATA (push, 1);

	return TRUE;
}

/* AROS CODE */

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
/* NOTE: Allows different formats of source and destination */
BOOL HIDDNouveauNV103DCopyBox(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG blendop)
{
    struct Picture sPict, dPict;
    LONG maskX = 0; LONG maskY = 0;

    HIDDNouveauFillPictureFromBitMapData(&sPict, srcdata);
    HIDDNouveauFillPictureFromBitMapData(&dPict, destdata);

    if (NV10EXAPrepareComposite(blendop,
        &sPict, NULL, &dPict, srcdata, NULL, destdata))
    {
        NV10EXAComposite(destdata, srcX, srcY,
				      maskX, maskY,
				      destX , destY,
				      width, height);
        return TRUE;
    }

    return FALSE;
}