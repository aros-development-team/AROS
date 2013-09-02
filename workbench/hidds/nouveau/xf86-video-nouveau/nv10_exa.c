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
#include "nv04_pushbuf.h"

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
get_tex_format(PicturePtr pict)
{
#if !defined(__AROS__)
	ScrnInfoPtr pScrn = xf86Screens[pict->pDrawable->pScreen->myNum];
#else
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);

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
#define SF(x) NV10TCL_BLEND_FUNC_SRC_##x
#define DF(x) NV10TCL_BLEND_FUNC_DST_##x

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
	{ SF(SRC_ALPHA),   DF(ONE_MINUS_SRC_ALPHA) }, /* OverAlpha */
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
	return nv10_pict_op[op].src != DF(ZERO);
}

static inline Bool
effective_component_alpha(PicturePtr mask)
{
	return mask && mask->componentAlpha && PICT_FORMAT_RGB(mask->format);
}

#if !defined(__AROS__)
static Bool
check_texture(PicturePtr pict)
{
	int w, h;

	if (!pict->pDrawable)
		NOUVEAU_FALLBACK("Solid and gradient pictures unsupported\n");

	w = pict->pDrawable->width;
	h = pict->pDrawable->height;

	if (w > 2046 || h > 2046)
		NOUVEAU_FALLBACK("picture too large, %dx%d\n", w, h);

	if (!get_tex_format(pict))
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
	if (!check_pict_op(op)) {
		print_fallback_info("pictop", op, src, mask, dst);
		return FALSE;
	}

	if (!check_render_target(dst)) {
		print_fallback_info("dst", op, src, mask, dst);
		return FALSE;
	}

	if (!check_texture(src)) {
		print_fallback_info("src", op, src, mask, dst);
		return FALSE;
	}

	if (mask) {
		if (!check_texture(mask)) {
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
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *celsius = pNv->Nv3D;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pixmap);
	unsigned tex_reloc = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;
#if !defined(__AROS__)
	long w = pict->pDrawable->width,
	     h = pict->pDrawable->height;
#else
	long w = pixmap->width,
	     h = pixmap->height;
#endif
	unsigned int txfmt =
		NV10TCL_TX_FORMAT_WRAP_T_CLAMP_TO_EDGE |
		NV10TCL_TX_FORMAT_WRAP_S_CLAMP_TO_EDGE |
		log2i(w) << 20 | log2i(h) << 16 |
		1 << 12 | /* lod == 1 */
		get_tex_format(pict) |
		0x50 /* UNK */;

	BEGIN_RING(chan, celsius, NV10TCL_TX_OFFSET(unit), 1);
	if (OUT_RELOCl(chan, bo, 0, tex_reloc))
		return FALSE;

	if (pict->repeat == RepeatNone) {
		/* NPOT_SIZE expects an even number for width, we can
		 * round up uneven numbers here because EXA always
		 * gives 64 byte aligned pixmaps and for all formats
		 * we support 64 bytes represents an even number of
		 * pixels
		 */
		w = (w + 1) &~ 1;

		BEGIN_RING(chan, celsius, NV10TCL_TX_NPOT_PITCH(unit), 1);
		OUT_RING  (chan, exaGetPixmapPitch(pixmap) << 16);

		BEGIN_RING(chan, celsius, NV10TCL_TX_NPOT_SIZE(unit), 1);
		OUT_RING  (chan, w << 16 | h);
	}

	BEGIN_RING(chan, celsius, NV10TCL_TX_FORMAT(unit), 1 );
	if (OUT_RELOCd(chan, bo, txfmt, tex_reloc | NOUVEAU_BO_OR,
		       NV10TCL_TX_FORMAT_DMA0, NV10TCL_TX_FORMAT_DMA1))
		return FALSE;

	BEGIN_RING(chan, celsius, NV10TCL_TX_ENABLE(unit), 1 );
	OUT_RING  (chan, NV10TCL_TX_ENABLE_ENABLE);

	BEGIN_RING(chan, celsius, NV10TCL_TX_FILTER(unit), 1);
	if (pict->filter == PictFilterNearest)
		OUT_RING(chan, (NV10TCL_TX_FILTER_MAGNIFY_NEAREST |
				NV10TCL_TX_FILTER_MINIFY_NEAREST));
	else
		OUT_RING(chan, (NV10TCL_TX_FILTER_MAGNIFY_LINEAR |
				NV10TCL_TX_FILTER_MINIFY_LINEAR));

	return TRUE;
}

static Bool
setup_render_target(NVPtr pNv, PicturePtr pict, PixmapPtr pixmap)
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *celsius = pNv->Nv3D;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pixmap);

	BEGIN_RING(chan, celsius, NV10TCL_RT_FORMAT, 2);
	OUT_RING  (chan, get_rt_format(pict));
	OUT_RING  (chan, (exaGetPixmapPitch(pixmap) << 16 |
			  exaGetPixmapPitch(pixmap)));

	BEGIN_RING(chan, celsius, NV10TCL_COLOR_OFFSET, 1);
	if (OUT_RELOCl(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR))
		return FALSE;

	return TRUE;
}

/*
 * This can be a bit difficult to understand at first glance.  Reg
 * combiners are described here:
 * http://icps.u-strasbg.fr/~marchesin/perso/extensions/NV/register_combiners.html
 *
 * Single texturing setup, without honoring vertex colors (non default
 * setup) is: Alpha RC 0 : a_0 * 1 + 0 * 0 RGB RC 0 : rgb_0 * 1 + 0 *
 * 0 RC 1s are unused Final combiner uses default setup
 *
 * Default setup uses vertex rgb/alpha in place of 1s above, but we
 * don't need that in 2D.
 *
 * Multi texturing setup, where we do TEX0 in TEX1 (masking) is:
 * Alpha RC 0 : a_0 * a_1 + 0 * 0
 * RGB RC0 : rgb_0 * a_1 + 0 * 0
 * RC 1s are unused
 * Final combiner uses default setup
 */

/* Bind the combiner variable <input> to a constant 1. */
#define RC_IN_ONE(input)						\
	(NV10TCL_RC_IN_RGB_##input##_INPUT_ZERO |			\
	 NV10TCL_RC_IN_RGB_##input##_COMPONENT_USAGE_ALPHA |		\
	 NV10TCL_RC_IN_RGB_##input##_MAPPING_UNSIGNED_INVERT)

/* Bind the combiner variable <input> to the specified channel from
 * the texture unit <unit>. */
#define RC_IN_TEX(input, chan, unit)					\
	(NV10TCL_RC_IN_RGB_##input##_INPUT_TEXTURE##unit |		\
	 NV10TCL_RC_IN_RGB_##input##_COMPONENT_USAGE_##chan)

/* Bind the combiner variable <input> to the specified channel from
 * the constant color <unit>. */
#define RC_IN_COLOR(input, chan, unit)					\
	(NV10TCL_RC_IN_RGB_##input##_INPUT_CONSTANT_COLOR##unit |	\
	 NV10TCL_RC_IN_RGB_##input##_COMPONENT_USAGE_##chan)

#if !defined(__AROS__)
static void
setup_combiners(NVPtr pNv, PicturePtr src, PicturePtr mask)
#else
static void
setup_combiners(NVPtr pNv, PicturePtr src, PicturePtr mask, int op)
#endif
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *celsius = pNv->Nv3D;
	uint32_t rc_in_alpha = 0, rc_in_rgb = 0;

	if (PICT_FORMAT_A(src->format))
		rc_in_alpha |= RC_IN_TEX(A, ALPHA, 0);
	else
		rc_in_alpha |= RC_IN_ONE(A);

	if (mask && PICT_FORMAT_A(mask->format))
		rc_in_alpha |= RC_IN_TEX(B, ALPHA, 1);
	else
		rc_in_alpha |= RC_IN_ONE(B);

	if (effective_component_alpha(mask)) {
#if !defined(__AROS__)
		if (!needs_src_alpha(pNv->alu)) {
#else
		if (!needs_src_alpha(op)) {
#endif
			/* The alpha channels won't be used for blending. Drop
			 * them, as our pixels only have 4 components...
			 * output_i = src_i * mask_i
			 */
			if (PICT_FORMAT_RGB(src->format))
				rc_in_rgb |= RC_IN_TEX(A, RGB, 0);
		} else {
			/* The RGB channels won't be used for blending. Drop
			 * them.
			 * output_i = src_alpha * mask_i
			 */
			if (PICT_FORMAT_A(src->format))
				rc_in_rgb |= RC_IN_TEX(A, ALPHA, 0);
			else
				rc_in_rgb |= RC_IN_ONE(A);
		}

		rc_in_rgb |= RC_IN_TEX(B, RGB, 1);

	} else {
		if (PICT_FORMAT_RGB(src->format))
			rc_in_rgb |= RC_IN_TEX(A, RGB, 0);

		if (mask && PICT_FORMAT_A(mask->format))
			rc_in_rgb |= RC_IN_TEX(B, ALPHA, 1);
		else
			rc_in_rgb |= RC_IN_ONE(B);
	}

	BEGIN_RING(chan, celsius, NV10TCL_RC_IN_ALPHA(0), 1);
	OUT_RING  (chan, rc_in_alpha);
	BEGIN_RING(chan, celsius, NV10TCL_RC_IN_RGB(0), 1);
	OUT_RING  (chan, rc_in_rgb);
}

#if !defined(__AROS__)
static void
setup_blend_function(NVPtr pNv)
#else
static void
setup_blend_function(NVPtr pNv, PicturePtr dst, PicturePtr mask, int blendop)
#endif
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *celsius = pNv->Nv3D;
#if !defined(__AROS__)
	struct pict_op *op = &nv10_pict_op[pNv->alu];
#else
	struct pict_op *op = &nv10_pict_op[blendop];
#endif
	int src_factor = op->src;
	int dst_factor = op->dst;

	if (src_factor == SF(ONE_MINUS_DST_ALPHA) &&
#if !defined(__AROS__)
	    !PICT_FORMAT_A(pNv->pdpict->format))
#else
	    !PICT_FORMAT_A(dst->format))
#endif
		/* ONE_MINUS_DST_ALPHA doesn't always do the right thing for
		 * framebuffers without alpha channel. But it's the same as
		 * ZERO in that case.
		 */
		src_factor = SF(ZERO);

#if !defined(__AROS__)
	if (effective_component_alpha(pNv->pmpict)) {
#else
	if (effective_component_alpha(mask)) {
#endif
		if (dst_factor == DF(SRC_ALPHA))
			dst_factor = DF(SRC_COLOR);
		else if (dst_factor == DF(ONE_MINUS_SRC_ALPHA))
			dst_factor = DF(ONE_MINUS_SRC_COLOR);
	}

	BEGIN_RING(chan, celsius, NV10TCL_BLEND_FUNC_SRC, 2);
	OUT_RING  (chan, src_factor);
	OUT_RING  (chan, dst_factor);
	BEGIN_RING(chan, celsius, NV10TCL_BLEND_FUNC_ENABLE, 1);
	OUT_RING  (chan, 1);
}

#if !defined(__AROS__)
static void
NV10StateCompositeReemit(struct nouveau_channel *chan)
{
	ScrnInfoPtr pScrn = chan->user_private;
	NVPtr pNv = NVPTR(pScrn);

	NV10EXAPrepareComposite(pNv->alu, pNv->pspict, pNv->pmpict, pNv->pdpict,
				pNv->pspix, pNv->pmpix, pNv->pdpix);
}
#endif

Bool
NV10EXAPrepareComposite(int op,
			PicturePtr pict_src,
			PicturePtr pict_mask,
			PicturePtr pict_dst,
			PixmapPtr src,
			PixmapPtr mask,
			PixmapPtr dst)
{
#if !defined(__AROS__)
	ScrnInfoPtr pScrn = xf86Screens[dst->drawable.pScreen->myNum];
#else
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;

	if (MARK_RING(chan, 128, 5))
		return FALSE;

#if !defined(__AROS__)
	pNv->alu = op;
	pNv->pspict = pict_src;
	pNv->pmpict = pict_mask;
	pNv->pdpict = pict_dst;
	pNv->pspix = src;
	pNv->pmpix = mask;
	pNv->pdpix = dst;
#endif

	/* Set dst format */
	if (!setup_render_target(pNv, pict_dst, dst))
		goto fail;

	/* Set src format */
	if (!setup_texture(pNv, 0, pict_src, src))
		goto fail;

	/* Set mask format */
	if (mask &&
	    !setup_texture(pNv, 1, pict_mask, mask))
		goto fail;

#if !defined(__AROS__)
	/* Set the register combiners up. */
	setup_combiners(pNv, pict_src, pict_mask);

	/* Set PictOp */
	setup_blend_function(pNv);

	chan->flush_notify = NV10StateCompositeReemit;
#else
	/* Set the register combiners up. */
	setup_combiners(pNv, pict_src, pict_mask, op);

	/* Set PictOp */
	setup_blend_function(pNv, pict_dst, pict_mask, op);

	chan->flush_notify = NULL;
#endif

	return TRUE;

fail:
	MARK_UNDO(chan);

	return FALSE;
}

#if defined(__AROS__)
/* WARNING: These defines are only used to hack QUAD/MAP/OUT_RINGi defines 
   in this use case. They WILL NOT work in generic case. DO NOT reuse them. */
struct _PictVector
{
    float vector[3];
};
typedef struct _PictVector PictVector;
#define xFixed1             0.0f
#define xFixedFrac(x)       0
#define xFixedToInt(x)      (x)
#define IntToxFixed(x)      (float)(x)
#endif

#define QUAD(x, y, w, h)					\
	{{{ IntToxFixed(x),     IntToxFixed(y),     xFixed1 }},	\
	 {{ IntToxFixed(x + w), IntToxFixed(y),     xFixed1 }},	\
	 {{ IntToxFixed(x + w), IntToxFixed(y + h), xFixed1 }},	\
	 {{ IntToxFixed(x),     IntToxFixed(y + h), xFixed1 }}}

#define MAP(f, p, v, ...) do {						\
		int __i;						\
		for (__i = 0; __i < sizeof(v)/sizeof((v)[0]); __i++)	\
			f(p, __i, v, ## __VA_ARGS__);			\
	} while (0);

#define xFixedToFloat(v) \
	((float)xFixedToInt((v)) + ((float)xFixedFrac(v) / 65536.0))

#define OUT_RINGi(chan, v, i)				\
	OUT_RINGf(chan, xFixedToFloat((v).vector[i]))

static inline void
emit_vertex(NVPtr pNv, int i, PictVector pos[],
	    PictVector tex0[], PictVector tex1[])
{
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *celsius = pNv->Nv3D;

	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_TX0_2F_S, 2);
	OUT_RINGi (chan, tex0[i], 0);
	OUT_RINGi (chan, tex0[i], 1);

	if (tex1) {
		BEGIN_RING(chan, celsius, NV10TCL_VERTEX_TX1_2F_S, 2);
		OUT_RINGi (chan, tex1[i], 0);
		OUT_RINGi (chan, tex1[i], 1);
	}

	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_POS_3F_X, 3);
	OUT_RINGi (chan, pos[i], 0);
	OUT_RINGi (chan, pos[i], 1);
	OUT_RINGf (chan, 0);
}

#if !defined(__AROS__)
static inline void
transform_vertex(PictTransformPtr t, int i, PictVector vs[])
{
	if  (t)
		PictureTransformPoint(t, &vs[i]);
}

void
NV10EXAComposite(PixmapPtr pix_dst,
		 int srcX, int srcY,
		 int maskX, int maskY,
		 int dstX, int dstY,
		 int width, int height)
{
	ScrnInfoPtr pScrn = xf86Screens[pix_dst->drawable.pScreen->myNum];
#else
static void
NV10EXAComposite(PixmapPtr pix_dst,
		 int srcX, int srcY,
		 int maskX, int maskY,
		 int dstX, int dstY,
		 int width, int height, 
		 PicturePtr src, PicturePtr mask)
{
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *celsius = pNv->Nv3D;
#if !defined(__AROS__)
	PicturePtr mask = pNv->pmpict,
		src = pNv->pspict;
#endif
	PictVector dstq[4] = QUAD(dstX, dstY, width, height),
		maskq[4] = QUAD(maskX, maskY, width, height),
		srcq[4] = QUAD(srcX, srcY, width, height);

#if !defined(__AROS__)
    /* We are not doing any transformations */
	MAP(transform_vertex, src->transform, srcq);
	if (mask)
		MAP(transform_vertex, mask->transform, maskq);
#endif

	WAIT_RING (chan, 64);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_BEGIN_END, 1);
	OUT_RING  (chan, NV10TCL_VERTEX_BEGIN_END_QUADS);

	MAP(emit_vertex, pNv, dstq, srcq, mask ? maskq : NULL);

	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_BEGIN_END, 1);
	OUT_RING  (chan, NV10TCL_VERTEX_BEGIN_END_STOP);
}

#if !defined(__AROS__)
void
NV10EXADoneComposite(PixmapPtr dst)
{
	ScrnInfoPtr pScrn = xf86Screens[dst->drawable.pScreen->myNum];
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;

	chan->flush_notify = NULL;
}
#endif

Bool
NVAccelInitNV10TCL(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *celsius;
	uint32_t class = 0;
	int i;

	if (((pNv->dev->chipset & 0xf0) != NV_ARCH_10) &&
	    ((pNv->dev->chipset & 0xf0) != NV_ARCH_20))
		return FALSE;

	if (pNv->dev->chipset >= 0x20 || pNv->dev->chipset == 0x1a)
		class = NV11TCL;
	else if (pNv->dev->chipset >= 0x17)
		class = NV17TCL;
	else if (pNv->dev->chipset >= 0x11)
		class = NV11TCL;
	else
		class = NV10TCL;

	if (!pNv->Nv3D) {
		if (nouveau_grobj_alloc(pNv->chan, Nv3D, class, &pNv->Nv3D))
			return FALSE;
	}
	celsius = pNv->Nv3D;

	BEGIN_RING(chan, celsius, NV10TCL_DMA_NOTIFY, 1);
	OUT_RING  (chan, chan->nullobj->handle);

	BEGIN_RING(chan, celsius, NV10TCL_DMA_IN_MEMORY0, 2);
	OUT_RING  (chan, pNv->chan->vram->handle);
	OUT_RING  (chan, pNv->chan->gart->handle);

	BEGIN_RING(chan, celsius, NV10TCL_DMA_IN_MEMORY2, 2);
	OUT_RING  (chan, pNv->chan->vram->handle);
	OUT_RING  (chan, pNv->chan->vram->handle);

	BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_RT_HORIZ, 2);
	OUT_RING  (chan, 2048 << 16 | 0);
	OUT_RING  (chan, 2048 << 16 | 0);

	BEGIN_RING(chan, celsius, NV10TCL_ZETA_OFFSET, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_MODE, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_HORIZ(0), 1);
	OUT_RING  (chan, 0x7ff << 16 | 0x800800);
	BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_VERT(0), 1);
	OUT_RING  (chan, 0x7ff << 16 | 0x800800);

	for (i = 1; i < 8; i++) {
		BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_HORIZ(i), 1);
		OUT_RING  (chan, 0);
		BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_VERT(i), 1);
		OUT_RING  (chan, 0);
	}

	BEGIN_RING(chan, celsius, 0x290, 1);
	OUT_RING  (chan, (0x10<<16)|1);
	BEGIN_RING(chan, celsius, 0x3f4, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
	OUT_RING  (chan, 0);

	if (class != NV10TCL) {
		/* For nv11, nv17 */
		BEGIN_RING(chan, celsius, 0x120, 3);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 2);

		BEGIN_RING(chan, pNv->NvImageBlit, 0x120, 3);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 2);

		BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
		OUT_RING  (chan, 0);
	}

	BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
	OUT_RING  (chan, 0);

	/* Set state */
	BEGIN_RING(chan, celsius, NV10TCL_FOG_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_ALPHA_FUNC_FUNC, 2);
	OUT_RING  (chan, 0x207);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_TX_ENABLE(0), 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_RC_IN_ALPHA(0), 6);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_RC_OUT_ALPHA(0), 6);
	OUT_RING  (chan, 0x00000c00);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0x00000c00);
	OUT_RING  (chan, 0x18000000);
	OUT_RING  (chan, 0x300c0000);
	OUT_RING  (chan, 0x00001c80);
	BEGIN_RING(chan, celsius, NV10TCL_BLEND_FUNC_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_DITHER_ENABLE, 2);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_WEIGHT_ENABLE, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_BLEND_FUNC_SRC, 4);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0x8006);
	BEGIN_RING(chan, celsius, NV10TCL_STENCIL_MASK, 8);
	OUT_RING  (chan, 0xff);
	OUT_RING  (chan, 0x207);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0xff);
	OUT_RING  (chan, 0x1e00);
	OUT_RING  (chan, 0x1e00);
	OUT_RING  (chan, 0x1e00);
	OUT_RING  (chan, 0x1d01);
	BEGIN_RING(chan, celsius, NV10TCL_NORMALIZE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_FOG_ENABLE, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_LIGHT_MODEL, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_SEPARATE_SPECULAR_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_ENABLED_LIGHTS, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_DEPTH_FUNC, 1);
	OUT_RING  (chan, 0x201);
	BEGIN_RING(chan, celsius, NV10TCL_DEPTH_WRITE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_DEPTH_TEST_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_POLYGON_OFFSET_FACTOR, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_POINT_SIZE, 1);
	OUT_RING  (chan, 8);
	BEGIN_RING(chan, celsius, NV10TCL_POINT_PARAMETERS_ENABLE, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_LINE_WIDTH, 1);
	OUT_RING  (chan, 8);
	BEGIN_RING(chan, celsius, NV10TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_POLYGON_MODE_FRONT, 2);
	OUT_RING  (chan, 0x1b02);
	OUT_RING  (chan, 0x1b02);
	BEGIN_RING(chan, celsius, NV10TCL_CULL_FACE, 2);
	OUT_RING  (chan, 0x405);
	OUT_RING  (chan, 0x901);
	BEGIN_RING(chan, celsius, NV10TCL_POLYGON_SMOOTH_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_CULL_FACE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_TX_GEN_MODE_S(0), 8);
	for (i = 0; i < 8; i++)
		OUT_RING  (chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_FOG_EQUATION_CONSTANT, 3);
	OUT_RING  (chan, 0x3fc00000);	/* -1.50 */
	OUT_RING  (chan, 0xbdb8aa0a);	/* -0.09 */
	OUT_RING  (chan, 0);		/*  0.00 */

	BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_FOG_MODE, 2);
	OUT_RING  (chan, 0x802);
	OUT_RING  (chan, 2);
	/* for some reason VIEW_MATRIX_ENABLE need to be 6 instead of 4 when
	 * using texturing, except when using the texture matrix
	 */
	BEGIN_RING(chan, celsius, NV10TCL_VIEW_MATRIX_ENABLE, 1);
	OUT_RING  (chan, 6);
	BEGIN_RING(chan, celsius, NV10TCL_COLOR_MASK, 1);
	OUT_RING  (chan, 0x01010101);

	BEGIN_RING(chan, celsius, NV10TCL_PROJECTION_MATRIX(0), 16);
	for(i = 0; i < 16; i++)
		OUT_RINGf(chan, i/4 == i%4 ? 1.0 : 0.0);

	BEGIN_RING(chan, celsius, NV10TCL_DEPTH_RANGE_NEAR, 2);
	OUT_RING  (chan, 0);
	OUT_RINGf (chan, 65536.0);

	BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_TRANSLATE_X, 4);
	OUT_RINGf (chan, -2048.0);
	OUT_RINGf (chan, -2048.0);
	OUT_RINGf (chan, 0);
	OUT_RING  (chan, 0);

	/* Set vertex component */
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_COL_4F_R, 4);
	OUT_RINGf (chan, 1.0);
	OUT_RINGf (chan, 1.0);
	OUT_RINGf (chan, 1.0);
	OUT_RINGf (chan, 1.0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_COL2_3F_R, 3);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_NOR_3F_X, 3);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RINGf (chan, 1.0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_TX0_4F_S, 4);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 1.0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_TX1_4F_S, 4);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 1.0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_FOG_1F, 1);
	OUT_RINGf (chan, 0.0);
	BEGIN_RING(chan, celsius, NV10TCL_EDGEFLAG_ENABLE, 1);
	OUT_RING  (chan, 1);

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
				      width, height, &sPict, NULL);
        return TRUE;
    }
    
    return FALSE;
}
