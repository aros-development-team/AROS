/*
 * Copyright 2007 Ben Skeggs
 * Copyright 2007 Stephane Marchesin
 * Copyright 2007 Jeremy Kolb
 * Copyright 2007 Patrice Mandin
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

#include "hwdefs/nv_object.xml.h"
#include "hwdefs/nv30-40_3d.xml.h"
#include "nv04_accel.h"

typedef struct nv_pict_surface_format {
	int	 pict_fmt;
	uint32_t card_fmt;
} nv_pict_surface_format_t;

typedef struct nv_pict_texture_format {
	int	 pict_fmt;
	uint32_t card_fmt;
	uint32_t card_swz;
} nv_pict_texture_format_t;

typedef struct nv_pict_op {
	Bool	 src_alpha;
	Bool	 dst_alpha;
	uint32_t src_card_op;
	uint32_t dst_card_op;
} nv_pict_op_t;

static nv_pict_surface_format_t
NV30SurfaceFormat[] = {
	{ PICT_a8r8g8b8	, 0x148 },
	{ PICT_a8b8g8r8	, 0x150 },
	{ PICT_x8r8g8b8	, 0x145 },
	{ PICT_x8b8g8r8	, 0x14f },
	{ PICT_r5g6b5	, 0x143 },
	{ PICT_a8       , 0x149 },
	{ PICT_x1r5g5b5	, 0x142 },
};

static nv_pict_surface_format_t *
NV30_GetPictSurfaceFormat(int format)
{
	int i;

	for(i=0;i<sizeof(NV30SurfaceFormat)/sizeof(NV30SurfaceFormat[0]);i++)
	{
		if (NV30SurfaceFormat[i].pict_fmt == format)
			return &NV30SurfaceFormat[i];
	}

	return NULL;
}

/* should be in nouveau_reg.h at some point.. */
#define NV30_3D_TEX_SWIZZLE_UNIT_S0_X_ZERO	 0
#define NV30_3D_TEX_SWIZZLE_UNIT_S0_X_ONE	 1
#define NV30_3D_TEX_SWIZZLE_UNIT_S0_X_S1		 2

#define NV30_3D_TEX_SWIZZLE_UNIT_S0_X_SHIFT	14
#define NV30_3D_TEX_SWIZZLE_UNIT_S0_Y_SHIFT	12
#define NV30_3D_TEX_SWIZZLE_UNIT_S0_Z_SHIFT	10
#define NV30_3D_TEX_SWIZZLE_UNIT_S0_W_SHIFT	 8

#define NV30_3D_TEX_SWIZZLE_UNIT_S1_X_X		 3
#define NV30_3D_TEX_SWIZZLE_UNIT_S1_X_Y		 2
#define NV30_3D_TEX_SWIZZLE_UNIT_S1_X_Z		 1
#define NV30_3D_TEX_SWIZZLE_UNIT_S1_X_W		 0

#define NV30_3D_TEX_SWIZZLE_UNIT_S1_X_SHIFT	 6
#define NV30_3D_TEX_SWIZZLE_UNIT_S1_Y_SHIFT	 4
#define NV30_3D_TEX_SWIZZLE_UNIT_S1_Z_SHIFT	 2
#define NV30_3D_TEX_SWIZZLE_UNIT_S1_W_SHIFT	 0

#define _(r,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w)                       \
  {                                                                           \
  PICT_##r,                                                                   \
  (tf),                                                                       \
  (NV30_3D_TEX_SWIZZLE_UNIT_S0_X_##ts0x << NV30_3D_TEX_SWIZZLE_UNIT_S0_X_SHIFT)|\
  (NV30_3D_TEX_SWIZZLE_UNIT_S0_X_##ts0y << NV30_3D_TEX_SWIZZLE_UNIT_S0_Y_SHIFT)|\
  (NV30_3D_TEX_SWIZZLE_UNIT_S0_X_##ts0z << NV30_3D_TEX_SWIZZLE_UNIT_S0_Z_SHIFT)|\
  (NV30_3D_TEX_SWIZZLE_UNIT_S0_X_##ts0w << NV30_3D_TEX_SWIZZLE_UNIT_S0_W_SHIFT)|\
  (NV30_3D_TEX_SWIZZLE_UNIT_S1_X_##ts1x << NV30_3D_TEX_SWIZZLE_UNIT_S1_X_SHIFT)|\
  (NV30_3D_TEX_SWIZZLE_UNIT_S1_X_##ts1y << NV30_3D_TEX_SWIZZLE_UNIT_S1_Y_SHIFT)|\
  (NV30_3D_TEX_SWIZZLE_UNIT_S1_X_##ts1z << NV30_3D_TEX_SWIZZLE_UNIT_S1_Z_SHIFT)|\
  (NV30_3D_TEX_SWIZZLE_UNIT_S1_X_##ts1w << NV30_3D_TEX_SWIZZLE_UNIT_S1_W_SHIFT)\
  }

static nv_pict_texture_format_t
NV30TextureFormat[] = {
	_(a8r8g8b8, 0x12,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(a8b8g8r8, 0x12,   S1,   S1,   S1,   S1, Z, Y, X, W),
	_(x8r8g8b8, 0x12,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(x8b8g8r8, 0x12,   S1,   S1,   S1,  ONE, Z, Y, X, W),

	_(a1r5g5b5, 0x10,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(x1r5g5b5, 0x10,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(a1b5g5r5, 0x10,   S1,   S1,   S1,   S1, Z, Y, X, W),
	_(x1b5g5r5, 0x10,   S1,   S1,   S1,  ONE, Z, Y, X, W),

	_(x4r4g4b4, 0x1d,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(a4r4g4b4, 0x1d,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(x4b4g4r4, 0x1d,   S1,   S1,   S1,  ONE, Z, Y, X, W),
	_(a4b4g4r4, 0x1d,   S1,   S1,   S1,   S1, Z, Y, X, W),

	_(      a8, 0x1b, ZERO, ZERO, ZERO,   S1, X, X, X, X),

	_(  r5g6b5, 0x11,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(  b5g6r5, 0x11,   S1,   S1,   S1,  ONE, Z, Y, X, W),
};


static nv_pict_texture_format_t *
NV30_GetPictTextureFormat(int format)
{
	int i;

	for(i=0;i<sizeof(NV30TextureFormat)/sizeof(NV30TextureFormat[0]);i++)
	{
		if (NV30TextureFormat[i].pict_fmt == format)
			return &NV30TextureFormat[i];
	}

	return NULL;
}

#define NV30_3D_BF_ZERO                                     0x0000
#define NV30_3D_BF_ONE                                      0x0001
#define NV30_3D_BF_SRC_COLOR                                0x0300
#define NV30_3D_BF_ONE_MINUS_SRC_COLOR                      0x0301
#define NV30_3D_BF_SRC_ALPHA                                0x0302
#define NV30_3D_BF_ONE_MINUS_SRC_ALPHA                      0x0303
#define NV30_3D_BF_DST_ALPHA                                0x0304
#define NV30_3D_BF_ONE_MINUS_DST_ALPHA                      0x0305
#define NV30_3D_BF_DST_COLOR                                0x0306
#define NV30_3D_BF_ONE_MINUS_DST_COLOR                      0x0307
#define NV30_3D_BF_ALPHA_SATURATE                           0x0308
#define BF(bf) NV30_3D_BF_##bf

static nv_pict_op_t 
NV30PictOp[] = {
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
/* Add         */ { 0, 0, BF(                ONE), BF(                ONE) }
};

static nv_pict_op_t *
NV30_GetPictOpRec(int op)
{
	if (op >= PictOpSaturate)
		return NULL;
#if 0
	switch(op)
	{
		case 0:ErrorF("Op Clear\n");break;
		case 1:ErrorF("Op Src\n");break;
		case 2:ErrorF("Op Dst\n");break;
		case 3:ErrorF("Op Over\n");break;
		case 4:ErrorF("Op OverReverse\n");break;
		case 5:ErrorF("Op In\n");break;
		case 6:ErrorF("Op InReverse\n");break;
		case 7:ErrorF("Op Out\n");break;
		case 8:ErrorF("Op OutReverse\n");break;
		case 9:ErrorF("Op Atop\n");break;
		case 10:ErrorF("Op AtopReverse\n");break;
		case 11:ErrorF("Op Xor\n");break;
		case 12:ErrorF("Op Add\n");break;
	}
#endif
	return &NV30PictOp[op];
}

static void
NV30_SetupBlend(ScrnInfoPtr pScrn, nv_pict_op_t *blend,
		PictFormatShort dest_format, Bool component_alpha)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t sblend, dblend;

	sblend = blend->src_card_op;
	dblend = blend->dst_card_op;

	if (blend->dst_alpha) {
		if (!PICT_FORMAT_A(dest_format)) {
			if (sblend == BF(DST_ALPHA)) {
				sblend = BF(ONE);
			} else if (sblend == BF(ONE_MINUS_DST_ALPHA)) {
				sblend = BF(ZERO);
			}
		} else if (dest_format == PICT_a8) {
			if (sblend == BF(DST_ALPHA)) {
				sblend = BF(DST_COLOR);
			} else if (sblend == BF(ONE_MINUS_DST_ALPHA)) {
				sblend = BF(ONE_MINUS_DST_COLOR);
			}
		}
	}

	if (blend->src_alpha && (component_alpha || dest_format == PICT_a8)) {
		if (dblend == BF(SRC_ALPHA)) {
			dblend = BF(SRC_COLOR);
		} else if (dblend == BF(ONE_MINUS_SRC_ALPHA)) {
			dblend = BF(ONE_MINUS_SRC_COLOR);
		}
	}

	if (sblend == BF(ONE) && dblend == BF(ZERO)) {
		BEGIN_NV04(push, NV30_3D(BLEND_FUNC_ENABLE), 1);
		PUSH_DATA (push, 0);
	} else {
		BEGIN_NV04(push, NV30_3D(BLEND_FUNC_ENABLE), 3);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, (sblend << 16) | sblend);
		PUSH_DATA (push, (dblend << 16) | dblend);
	}
}

static Bool
NV30EXATexture(ScrnInfoPtr pScrn, PixmapPtr pPix, PicturePtr pPict, int unit)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pPix);
	nv_pict_texture_format_t *fmt;
	unsigned reloc = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;
	uint32_t pitch = exaGetPixmapPitch(pPix);
	uint32_t log2h = log2i(pPix->drawable.height);
	uint32_t log2w = log2i(pPix->drawable.width);
	uint32_t card_filter, card_repeat;

	fmt = NV30_GetPictTextureFormat(pPict->format);
	if (!fmt)
		return FALSE;

	card_repeat = 3; /* repeatNone */

	if (pPict->filter == PictFilterBilinear)
		card_filter = 2;
	else
		card_filter = 1;

	BEGIN_NV04(push, NV30_3D(TEX_OFFSET(unit)), 8);
	PUSH_MTHDl(push, NV30_3D(TEX_OFFSET(unit)), bo, 0, reloc);
	PUSH_MTHDs(push, NV30_3D(TEX_FORMAT(unit)), bo, (1 << 16) | 8 |
			 NV30_3D_TEX_FORMAT_DIMS_2D |
			 (fmt->card_fmt << NV30_3D_TEX_FORMAT_FORMAT__SHIFT) |
			 (log2w << NV30_3D_TEX_FORMAT_BASE_SIZE_U__SHIFT) |
			 (log2h << NV30_3D_TEX_FORMAT_BASE_SIZE_V__SHIFT),
			 reloc, NV30_3D_TEX_FORMAT_DMA0,
			 NV30_3D_TEX_FORMAT_DMA1);
	PUSH_DATA (push, (card_repeat << NV30_3D_TEX_WRAP_S__SHIFT) |
			 (card_repeat << NV30_3D_TEX_WRAP_T__SHIFT) |
			 (card_repeat << NV30_3D_TEX_WRAP_R__SHIFT));
	PUSH_DATA (push, NV30_3D_TEX_ENABLE_ENABLE);
	PUSH_DATA (push, (pitch << NV30_3D_TEX_SWIZZLE_RECT_PITCH__SHIFT ) |
			 fmt->card_swz);
	PUSH_DATA (push, (card_filter << NV30_3D_TEX_FILTER_MIN__SHIFT) |
			 (card_filter << NV30_3D_TEX_FILTER_MAG__SHIFT) |
			 0x2000 /* engine lock */);
	PUSH_DATA (push, (pPix->drawable.width <<
			  NV30_3D_TEX_NPOT_SIZE_W__SHIFT) |
			 pPix->drawable.height);
	PUSH_DATA (push, 0x00000000); /* border ARGB */
#if !defined(__AROS__)
	if (pPict->transform) {
		BEGIN_NV04(push, NV30_3D(TEX_MATRIX_ENABLE(unit)), 1);
		PUSH_DATA (push, 1);
		BEGIN_NV04(push, NV30_3D(TEX_MATRIX(unit, 0)), 16);
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[0][0]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[0][1]));
		PUSH_DATAf(push, 0.f);
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[0][2]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[1][0]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[1][1]));
		PUSH_DATAf(push, 0.f);
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[1][2]));
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[2][0]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[2][1]));
		PUSH_DATAf(push, 0.0f);
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[2][2]));
	} else {
#else
	{ // we are not doing any transformations
#endif
		BEGIN_NV04(push, NV30_3D(TEX_MATRIX_ENABLE(unit)), 1);
		PUSH_DATA (push, 0);
	}

	return TRUE;
}

#define RCSRC_COL(i)  (0x01 + (unit))
#define RCSRC_TEX(i)  (0x08 + (unit)) /* fragprog register */
#define RCSEL_COLOR   (0x00)
#define RCSEL_ALPHA   (0x10)
#define RCINP_ZERO    (0x00)
#define RCINP_ONE     (0x20)
#define RCINP_A__SHIFT 24
#define RCINP_B__SHIFT 16

static Bool
NV30EXAPicture(ScrnInfoPtr pScrn, PixmapPtr pPix, PicturePtr pPict, int unit,
	       uint32_t *color, uint32_t *alpha, uint32_t *solid)
{
	uint32_t shift, source;

	if (pPict && pPict->pDrawable) {
		if (!NV30EXATexture(pScrn, pPix, pPict, unit))
			return FALSE;
		*solid = 0x00000000;
		source = RCSRC_TEX(unit);
	} else
	if (pPict) {
NOT_IMPLEMENTED_STOP
#if 0
		*solid = pPict->pSourcePict->solidFill.color;
		source = RCSRC_COL(unit);
#endif
	}

	if (pPict && PICT_FORMAT_RGB(pPict->format))
		*color = RCSEL_COLOR | source;
	else
		*color = RCSEL_ALPHA | RCINP_ZERO;

	if (pPict && PICT_FORMAT_A(pPict->format))
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

static Bool
NV30_SetupSurface(ScrnInfoPtr pScrn, PixmapPtr pPix, PicturePtr pPict)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pPix);
	uint32_t pitch = exaGetPixmapPitch(pPix);
	nv_pict_surface_format_t *fmt;

	fmt = NV30_GetPictSurfaceFormat(pPict->format);
	if (!fmt) {
		ErrorF("AIII no format\n");
		return FALSE;
	}

	BEGIN_NV04(push, NV30_3D(RT_FORMAT), 3);
	PUSH_DATA (push, fmt->card_fmt); /* format */
	PUSH_DATA (push, pitch << 16 | pitch);
	PUSH_MTHDl(push, NV30_3D(COLOR0_OFFSET), bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	return TRUE;
}

#if !defined(__AROS__)
static Bool
NV30EXACheckCompositeTexture(PicturePtr pPict, PicturePtr pdPict, int op)
{
	nv_pict_texture_format_t *fmt;
	int w = 1, h = 1;

	if (pPict->pDrawable) {
		w = pPict->pDrawable->width;
		h = pPict->pDrawable->height;
	} else {
		if (pPict->pSourcePict->type != SourcePictTypeSolidFill)
			NOUVEAU_FALLBACK("gradient pictures unsupported\n");
	}

	if ((w > 4096) || (h > 4096))
		NOUVEAU_FALLBACK("picture too large, %dx%d\n", w, h);

	fmt = NV30_GetPictTextureFormat(pPict->format);
	if (!fmt)
		NOUVEAU_FALLBACK("picture format 0x%08x not supported\n",
				pPict->format);

	if (pPict->filter != PictFilterNearest &&
			pPict->filter != PictFilterBilinear)
		NOUVEAU_FALLBACK("filter 0x%x not supported\n", pPict->filter);

	if (!(w==1 && h==1) && pPict->repeat && pPict->repeatType != RepeatNone)
		NOUVEAU_FALLBACK("repeat 0x%x not supported (surface %dx%d)\n",
				 pPict->repeatType,w,h);

	/* Opengl and Render disagree on what should be sampled outside an XRGB 
	 * texture (with no repeating). Opengl has a hardcoded alpha value of 
	 * 1.0, while render expects 0.0. We assume that clipping is done for 
	 * untranformed sources.
	 */
	if (NV30PictOp[op].src_alpha && !pPict->repeat &&
		pPict->transform && (PICT_FORMAT_A(pPict->format) == 0)
		&& (PICT_FORMAT_A(pdPict->format) != 0))
		NOUVEAU_FALLBACK("REPEAT_NONE unsupported for XRGB source\n");

	return TRUE;
}

Bool
NV30EXACheckComposite(int op, PicturePtr psPict,
		PicturePtr pmPict,
		PicturePtr pdPict)
{
	nv_pict_surface_format_t *fmt;
	nv_pict_op_t *opr;

	opr = NV30_GetPictOpRec(op);
	if (!opr)
		NOUVEAU_FALLBACK("unsupported blend op 0x%x\n", op);

	fmt = NV30_GetPictSurfaceFormat(pdPict->format);
	if (!fmt)
		NOUVEAU_FALLBACK("dst picture format 0x%08x not supported\n",
				pdPict->format);

	if (!NV30EXACheckCompositeTexture(psPict, pdPict, op))
		NOUVEAU_FALLBACK("src picture\n");
	if (pmPict) {
		if (pmPict->componentAlpha &&
				PICT_FORMAT_RGB(pmPict->format) &&
				opr->src_alpha && opr->src_card_op != BF(ZERO))
			NOUVEAU_FALLBACK("mask CA + SA\n");
		if (!NV30EXACheckCompositeTexture(pmPict, pdPict, op))
			NOUVEAU_FALLBACK("mask picture\n");
	}

	return TRUE;
}
#endif

Bool
NV30EXAPrepareComposite(int op, PicturePtr psPict,
		PicturePtr pmPict,
		PicturePtr pdPict,
		PixmapPtr  psPix,
		PixmapPtr  pmPix,
		PixmapPtr  pdPix)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pdPix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	nv_pict_op_t *blend = NV30_GetPictOpRec(op);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t sc, sa, mc, ma, solid[2];

	if (!PUSH_SPACE(push, 128))
		return FALSE;
	PUSH_RESET(push);

	/* setup render target and blending */
	if (!NV30_SetupSurface(pScrn, pdPix, pdPict))
		return FALSE;
	NV30_SetupBlend(pScrn, blend, pdPict->format,
			(pmPict && pmPict->componentAlpha &&
			 PICT_FORMAT_RGB(pmPict->format)));

	/* select picture sources */
	if (!NV30EXAPicture(pScrn, psPix, psPict, 0, &sc, &sa, &solid[0]))
		return FALSE;
	if (!NV30EXAPicture(pScrn, pmPix, pmPict, 1, &mc, &ma, &solid[1]))
		return FALSE;

	/* configure register combiners */
	BEGIN_NV04(push, NV30_3D(RC_IN_ALPHA(0)), 6);
	PUSH_DATA (push, sa | ma);
	if (pmPict &&
	    pmPict->componentAlpha && PICT_FORMAT_RGB(pmPict->format)) {
		if (blend->src_alpha)
			PUSH_DATA(push, sa | mc);
		else
			PUSH_DATA(push, sc | mc);
	} else {
		PUSH_DATA(push, sc | ma);
	}
	PUSH_DATA (push, solid[0]);
	PUSH_DATA (push, solid[1]);
	PUSH_DATA (push, 0x00000c00);
	PUSH_DATA (push, 0x00000c00);
	BEGIN_NV04(push, NV30_3D(RC_FINAL0), 3);
	if (pdPict->format != PICT_a8)
		PUSH_DATA (push, 0x0000000c);
	else
		PUSH_DATA (push, 0x0000001c);
	PUSH_DATA (push, 0x00001c00);
	PUSH_DATA (push, 0x01000101);

	/* select fragprog which just sources textures for combiners */
	BEGIN_NV04(push, NV30_3D(FP_ACTIVE_PROGRAM), 1);
	PUSH_MTHD (push, NV30_3D(FP_ACTIVE_PROGRAM), pNv->scratch, PFP_PASS,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_LOW |
			 NOUVEAU_BO_OR,
			 NV30_3D_FP_ACTIVE_PROGRAM_DMA0,
			 NV30_3D_FP_ACTIVE_PROGRAM_DMA1);
	BEGIN_NV04(push, NV30_3D(FP_REG_CONTROL), 1);
	PUSH_DATA (push, 0x0001000f);
	BEGIN_NV04(push, NV30_3D(FP_CONTROL), 1);
	PUSH_DATA (push, 0x00000000);
	BEGIN_NV04(push, NV30_3D(TEX_UNITS_ENABLE), 1);
	PUSH_DATA (push, 3);

	nouveau_pushbuf_bufctx(push, pNv->bufctx);
	if (nouveau_pushbuf_validate(push)) {
		nouveau_pushbuf_bufctx(push, NULL);
		return FALSE;
	}

	return TRUE;
}

static __inline__ void
PUSH_VTX2s(struct nouveau_pushbuf *push,
	   int x1, int y1, int x2, int y2, int dx, int dy)
{
	BEGIN_NV04(push, NV30_3D(VTX_ATTR_2I(8)), 2);
	PUSH_DATA (push, ((y1 & 0xffff) << 16) | (x1 & 0xffff));
	PUSH_DATA (push, ((y2 & 0xffff) << 16) | (x2 & 0xffff));
	BEGIN_NV04(push, NV30_3D(VTX_ATTR_2I(0)), 1);
	PUSH_DATA (push, ((dy & 0xffff) << 16) | (dx & 0xffff));
}

void
NV30EXAComposite(PixmapPtr pdPix,
		 int sx, int sy, int mx, int my, int dx, int dy, int w, int h)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pdPix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (!PUSH_SPACE(push, 64))
		return;

	/* We're drawing a triangle, we need to scissor it to a quad. */
	/* The scissors are here for a good reason, we don't get the full
	 * image, but just a part.
	 */
	/* Handling the cliprects is done for us already. */
	BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
	PUSH_DATA (push, (w << 16) | dx);
	PUSH_DATA (push, (h << 16) | dy);
	BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
	PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_TRIANGLES);
	PUSH_VTX2s(push, sx, sy + (h * 2), mx, my + (h * 2), dx, dy + (h * 2));
	PUSH_VTX2s(push, sx, sy, mx, my, dx, dy);
	PUSH_VTX2s(push, sx + (w * 2), sy, mx + (w * 2), my, dx + (w * 2), dy);
	BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
	PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_STOP);
}

void
NV30EXADoneComposite(PixmapPtr pdPix)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pdPix->drawable.pScreen);
	nouveau_pushbuf_bufctx(NVPTR(pScrn)->pushbuf, NULL);
}

Bool
NVAccelInitNV30TCL(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nv04_fifo *fifo = pNv->channel->data;
	uint32_t class = 0, chipset;
	int i;

	NVXVComputeBicubicFilter(pNv->scratch, XV_TABLE, XV_TABLE_SIZE);

#define NV30TCL_CHIPSET_3X_MASK 0x00000003
#define NV35TCL_CHIPSET_3X_MASK 0x000001e0
#define NV30_3D_CHIPSET_3X_MASK 0x00000010

	chipset = pNv->dev->chipset;
	if ((chipset & 0xf0) != NV_ARCH_30)
		return TRUE;
	chipset &= 0xf;

	if (NV30TCL_CHIPSET_3X_MASK & (1<<chipset))
		class = NV30_3D_CLASS;
	else if (NV35TCL_CHIPSET_3X_MASK & (1<<chipset))
		class = NV35_3D_CLASS;
	else if (NV30_3D_CHIPSET_3X_MASK & (1<<chipset))
		class = NV34_3D_CLASS;
	else {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "NV30EXA: Unknown chipset NV3%1x\n", chipset);
		return FALSE;
	}

	if (nouveau_object_new(pNv->channel, Nv3D, class, NULL, 0, &pNv->Nv3D))
		return FALSE;

	if (!PUSH_SPACE(push, 256))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(3D, OBJECT), 1);
	PUSH_DATA (push, pNv->Nv3D->handle);
	BEGIN_NV04(push, NV30_3D(DMA_TEXTURE0), 3);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->gart);
	PUSH_DATA (push, fifo->vram);
	BEGIN_NV04(push, NV30_3D(DMA_UNK1AC), 1);
	PUSH_DATA (push, fifo->vram);
	BEGIN_NV04(push, NV30_3D(DMA_COLOR0), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->vram);
	BEGIN_NV04(push, NV30_3D(DMA_UNK1B0), 1);
	PUSH_DATA (push, fifo->vram);

	for (i=1; i<8; i++) {
		BEGIN_NV04(push, NV30_3D(VIEWPORT_CLIP_HORIZ(i)), 2);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV30_3D(RT_ENABLE), 1);
	PUSH_DATA (push, 1);

	BEGIN_NV04(push, NV40_3D(MIPMAP_ROUNDING), 1);
	PUSH_DATA (push, NV40_3D_MIPMAP_ROUNDING_MODE_DOWN);
	BEGIN_NV04(push, NV30_3D(FLATSHADE_FIRST), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, SUBC_3D(0x1d80), 1);
	PUSH_DATA (push, 3);
	BEGIN_NV04(push, NV30_3D(FP_REG_CONTROL), 1);
	PUSH_DATA (push, 0x00030004);

	/* NEW */
	BEGIN_NV04(push, SUBC_3D(0x1e98), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, SUBC_3D(0x17e0), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0x3f800000);
	BEGIN_NV04(push, SUBC_3D(0x1f80), 16);
	PUSH_DATA (push, 0); PUSH_DATA (push, 0); PUSH_DATA (push, 0); PUSH_DATA (push, 0);
	PUSH_DATA (push, 0); PUSH_DATA (push, 0); PUSH_DATA (push, 0); PUSH_DATA (push, 0);
	PUSH_DATA (push, 0x0000ffff);
	PUSH_DATA (push, 0); PUSH_DATA (push, 0); PUSH_DATA (push, 0); PUSH_DATA (push, 0);
	PUSH_DATA (push, 0); PUSH_DATA (push, 0); PUSH_DATA (push, 0);

	BEGIN_NV04(push, NV30_3D(FLIP_SET_READ), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 2);

	BEGIN_NV04(push, NV15_BLIT(FLIP_SET_READ), 3);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 2);

	BEGIN_NV04(push, NV30_3D(COORD_CONVENTIONS), 1);
	PUSH_DATA (push, 0x00001200);

	BEGIN_NV04(push, NV30_3D(MULTISAMPLE_CONTROL), 1);
	PUSH_DATA (push, 0xffff0000);

	/* Attempt to setup a known state.. Probably missing a heap of
	 * stuff here..
	 */
	BEGIN_NV04(push, NV30_3D(STENCIL_ENABLE(0)), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(STENCIL_ENABLE(1)), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(ALPHA_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(DEPTH_WRITE_ENABLE), 2);
	PUSH_DATA (push, 0); /* wr disable */
	PUSH_DATA (push, 0); /* test disable */
	BEGIN_NV04(push, NV30_3D(COLOR_MASK), 1);
	PUSH_DATA (push, 0x01010101); /* TR,TR,TR,TR */
	BEGIN_NV04(push, NV30_3D(CULL_FACE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(BLEND_FUNC_ENABLE), 5);
	PUSH_DATA (push, 0);				/* Blend enable */
	PUSH_DATA (push, 0);				/* Blend src */
	PUSH_DATA (push, 0);				/* Blend dst */
	PUSH_DATA (push, 0x00000000);			/* Blend colour */
	PUSH_DATA (push, 0x8006);			/* FUNC_ADD */
	BEGIN_NV04(push, NV30_3D(COLOR_LOGIC_OP_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0x1503 /*GL_COPY*/);
	BEGIN_NV04(push, NV30_3D(DITHER_ENABLE), 1);
	PUSH_DATA (push, 1);
	BEGIN_NV04(push, NV30_3D(SHADE_MODEL), 1);
	PUSH_DATA (push, 0x1d01 /*GL_SMOOTH*/);
	BEGIN_NV04(push, NV30_3D(POLYGON_OFFSET_FACTOR),2);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	BEGIN_NV04(push, NV30_3D(POLYGON_MODE_FRONT), 2);
	PUSH_DATA (push, 0x1b02 /*GL_FILL*/);
	PUSH_DATA (push, 0x1b02 /*GL_FILL*/);
	/* - Disable texture units
	 * - Set fragprog to MOVR result.color, fragment.color */
	for (i=0;i<4;i++) {
		BEGIN_NV04(push, NV30_3D(TEX_ENABLE(i)), 1);
		PUSH_DATA (push, 0);
	}
	/* Polygon stipple */
	BEGIN_NV04(push, NV30_3D(POLYGON_STIPPLE_PATTERN(0)), 0x20);
	for (i=0;i<0x20;i++)
		PUSH_DATA (push, 0xFFFFFFFF);

	BEGIN_NV04(push, NV30_3D(DEPTH_RANGE_NEAR), 2);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);

	/* Ok.  If you start X with the nvidia driver, kill it, and then
	 * start X with nouveau you will get black rendering instead of
	 * what you'd expect.  This fixes the problem, and it seems that
	 * it's not needed between nouveau restarts - which suggests that
	 * the 3D context (wherever it's stored?) survives somehow.
	 */
	//BEGIN_NV04(push, NV30_3D(FP_CONTROL),1);
	//PUSH_DATA (push, 0x03008000);

	int w=4096;
	int h=4096;
	int pitch=4096*4;
	BEGIN_NV04(push, NV30_3D(RT_HORIZ), 5);
	PUSH_DATA (push, w<<16);
	PUSH_DATA (push, h<<16);
	PUSH_DATA (push, 0x148); /* format */
	PUSH_DATA (push, pitch << 16 | pitch);
	PUSH_DATA (push, 0x0);
	BEGIN_NV04(push, NV30_3D(VIEWPORT_TX_ORIGIN), 1);
	PUSH_DATA (push, 0);
        BEGIN_NV04(push, NV30_3D(VIEWPORT_HORIZ), 2);
        PUSH_DATA (push, (w<<16) | 0);
        PUSH_DATA (push, (h<<16) | 0);
	BEGIN_NV04(push, NV30_3D(VIEWPORT_CLIP_HORIZ(0)), 2);
	PUSH_DATA (push, (w-1)<<16);
	PUSH_DATA (push, (h-1)<<16);
	BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
	PUSH_DATA (push, w<<16);
	PUSH_DATA (push, h<<16);
	BEGIN_NV04(push, NV30_3D(VIEWPORT_HORIZ), 2);
	PUSH_DATA (push, w<<16);
	PUSH_DATA (push, h<<16);

	BEGIN_NV04(push, NV30_3D(VIEWPORT_TRANSLATE_X), 8);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);

	BEGIN_NV04(push, NV30_3D(MODELVIEW_MATRIX(0)), 16);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);

	BEGIN_NV04(push, NV30_3D(PROJECTION_MATRIX(0)), 16);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);

	BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
	PUSH_DATA (push, 4096<<16);
	PUSH_DATA (push, 4096<<16);

	PUSH_DATAu(push, pNv->scratch, PFP_PASS, 2 * 4);
	PUSH_DATAs(push, 0x18009e80); /* txph r0, a[tex0], t[0] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x1802be83); /* txph r1, a[tex1], t[1] */
	PUSH_DATAs(push, 0x1c9dc801); /* exit */
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);

	PUSH_DATAu(push, pNv->scratch, PFP_NV12_BILINEAR, 8 * 4);
	PUSH_DATAs(push, 0x17028200); /* texr r0.x, a[tex0], t[1] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x04000e02); /* madr r1.xyz, r0.x, imm.x, imm.yzww */
	PUSH_DATAs(push, 0x1c9c0000);
	PUSH_DATAs(push, 0x00000002);
	PUSH_DATAs(push, 0x0001f202);
	PUSH_DATAs(push, 0x3f9507c8); /* { 1.16, -0.87, 0.53, -1.08 } */
	PUSH_DATAs(push, 0xbf5ee393);
	PUSH_DATAs(push, 0x3f078fef);
	PUSH_DATAs(push, 0xbf8a6762);
	PUSH_DATAs(push, 0x1704ac80); /* texr r0.yz, a[tex1], t[2] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x04000e02); /* madr r1.xyz, r0.y, imm, r1 */
	PUSH_DATAs(push, 0x1c9cab00);
	PUSH_DATAs(push, 0x0001c802);
	PUSH_DATAs(push, 0x0001c804);
	PUSH_DATAs(push, 0x00000000); /* { 0.00, -0.39, 2.02, 0.00 } */
	PUSH_DATAs(push, 0xbec890d6);
	PUSH_DATAs(push, 0x40011687);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x04000e81); /* madr r0.xyz, r0.z, imm, r1 */
	PUSH_DATAs(push, 0x1c9d5500);
	PUSH_DATAs(push, 0x0001c802);
	PUSH_DATAs(push, 0x0001c804);
	PUSH_DATAs(push, 0x3fcc432d); /* { 1.60, -0.81, 0.00, 0.00 } */
	PUSH_DATAs(push, 0xbf501a37);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);

	PUSH_DATAu(push, pNv->scratch, PFP_NV12_BICUBIC, 24 * 4);
	PUSH_DATAs(push, 0x01008604); /* movr r2.xy, a[tex0] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x03000600); /* addr r0.xy, r2, imm.x */
	PUSH_DATAs(push, 0x1c9dc808);
	PUSH_DATAs(push, 0x00000002);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3f000000); /* { 0.50, 0.00, 0.00, 0.00 } */
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x17000e06); /* texr r3.xyz, r0, t[0] */
	PUSH_DATAs(push, 0x1c9dc800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x17000e00); /* texr r0.xyz, r0.y, t[0] */
	PUSH_DATAs(push, 0x1c9caa00);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x02000a02); /* mulr r1.xz, r3.xxyy, imm.xxyy */
	PUSH_DATAs(push, 0x1c9ca00c);
	PUSH_DATAs(push, 0x0000a002);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0xbf800000); /* { -1.00, 1.00, 0.00, 0.00 } */
	PUSH_DATAs(push, 0x3f800000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x02001402); /* mulr r1.yw, r0.xxyy, imm.xxyy */
	PUSH_DATAs(push, 0x1c9ca000);
	PUSH_DATAs(push, 0x0000a002);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0xbf800000); /* { -1.00, 1.00, 0.00, 0.00 } */
	PUSH_DATAs(push, 0x3f800000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x03001e04); /* addr r2, r2.xyxy, r1 */
	PUSH_DATAs(push, 0x1c9c8808);
	PUSH_DATAs(push, 0x0001c804);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x17020200); /* texr r0.x, r2, t[1] */
	PUSH_DATAs(push, 0x1c9dc808);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x17020402); /* texr r1.y, r2.xwxw, t[1] */
	PUSH_DATAs(push, 0x1c9d9808);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x17020202); /* texr r1.x, r2.zyxy, t[1] */
	PUSH_DATAs(push, 0x1c9c8c08);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x1f400280); /* lrph r0.x, r0.z, r0, r1.y */
	PUSH_DATAs(push, 0x1c9d5400);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0000aa04);
	PUSH_DATAs(push, 0x17020400); /* texr r0.y, r2.zwzz, t[1] */
	PUSH_DATAs(push, 0x1c9d5c08);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x1f400480); /* lrph r0.y, r0.z, r1.x, r0 */
	PUSH_DATAs(push, 0x1c9d5400);
	PUSH_DATAs(push, 0x00000004);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x1f400280); /* lrph r0.x, r3.z, r0, r0.y */
	PUSH_DATAs(push, 0x1c9d540c);
	PUSH_DATAs(push, 0x0001c900);
	PUSH_DATAs(push, 0x0000ab00);
	PUSH_DATAs(push, 0x04400e80); /* madh r0.xyz, r0.x, imm.x, imm.yzww */
	PUSH_DATAs(push, 0x1c9c0100);
	PUSH_DATAs(push, 0x00000002);
	PUSH_DATAs(push, 0x0001f202);
	PUSH_DATAs(push, 0x3f9507c8); /* { 1.16, -0.87, 0.53, -1.08 } */
	PUSH_DATAs(push, 0xbf5ee393);
	PUSH_DATAs(push, 0x3f078fef);
	PUSH_DATAs(push, 0xbf8a6762);
	PUSH_DATAs(push, 0x1704ac02); /* texr r1.yz, a[tex1], t[2] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x04400e80); /* madh r0.xyz, r1.y, imm, r0 */
	PUSH_DATAs(push, 0x1c9caa04);
	PUSH_DATAs(push, 0x0001c802);
	PUSH_DATAs(push, 0x0001c900);
	PUSH_DATAs(push, 0x00000000); /* { 0.00, -0.39, 2.02, 0.00 } */
	PUSH_DATAs(push, 0xbec890d6);
	PUSH_DATAs(push, 0x40011687);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x04400e81); /* madh r0.xyz, r1.z, imm, r0 */
	PUSH_DATAs(push, 0x1c9d5404);
	PUSH_DATAs(push, 0x0001c802);
	PUSH_DATAs(push, 0x0001c900);
	PUSH_DATAs(push, 0x3fcc432d); /* { 1.60, -0.81, 0.00, 0.00 } */
	PUSH_DATAs(push, 0xbf501a37);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);

	return TRUE;
}

/* AROS CODE */

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes buffer is not mapped */
/* NOTE: Allows different formats of source and destination */
BOOL HIDDNouveauNV303DCopyBox(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG blendop)
{
    struct Picture sPict, dPict;
    LONG maskX = 0; LONG maskY = 0;

    HIDDNouveauFillPictureFromBitMapData(&sPict, srcdata);
    HIDDNouveauFillPictureFromBitMapData(&dPict, destdata);

    if (NV30EXAPrepareComposite(blendop,
        &sPict, NULL, &dPict, srcdata, NULL, destdata))
    {
        NV30EXAComposite(destdata, srcX, srcY,
				      maskX, maskY,
				      destX , destY,
				      width, height);
        return TRUE;
    }

    return FALSE;
}