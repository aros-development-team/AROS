/*
 * Copyright 2007 Ben Skeggs
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
NV40SurfaceFormat[] = {
	{ PICT_a8r8g8b8	, NV30_3D_RT_FORMAT_COLOR_A8R8G8B8 },
	{ PICT_x8r8g8b8	, NV30_3D_RT_FORMAT_COLOR_X8R8G8B8 },
	{ PICT_r5g6b5	, NV30_3D_RT_FORMAT_COLOR_R5G6B5   },
	{ PICT_a8       , NV30_3D_RT_FORMAT_COLOR_B8       },
	{ -1, ~0 }
};

static nv_pict_surface_format_t *
NV40_GetPictSurfaceFormat(int format)
{
	int i = 0;

	while (NV40SurfaceFormat[i].pict_fmt != -1) {
		if (NV40SurfaceFormat[i].pict_fmt == format)
			return &NV40SurfaceFormat[i];
		i++;
	}

	return NULL;
}

#define _(r,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w)                        \
  {                                                                            \
  PICT_##r, NV40_3D_TEX_FORMAT_FORMAT_##tf,                                    \
  NV30_3D_TEX_SWIZZLE_S0_X_##ts0x | NV30_3D_TEX_SWIZZLE_S0_Y_##ts0y |          \
  NV30_3D_TEX_SWIZZLE_S0_Z_##ts0z | NV30_3D_TEX_SWIZZLE_S0_W_##ts0w |          \
  NV30_3D_TEX_SWIZZLE_S1_X_##ts1x | NV30_3D_TEX_SWIZZLE_S1_Y_##ts1y |          \
  NV30_3D_TEX_SWIZZLE_S1_Z_##ts1z | NV30_3D_TEX_SWIZZLE_S1_W_##ts1w,           \
  }
static nv_pict_texture_format_t
NV40TextureFormat[] = {
        _(a8r8g8b8, A8R8G8B8,   S1,   S1,   S1,   S1, X, Y, Z, W),
        _(x8r8g8b8, A8R8G8B8,   S1,   S1,   S1,  ONE, X, Y, Z, W),
        _(x8b8g8r8, A8R8G8B8,   S1,   S1,   S1,  ONE, Z, Y, X, W),
        _(a1r5g5b5, A1R5G5B5,   S1,   S1,   S1,   S1, X, Y, Z, W),
        _(x1r5g5b5, A1R5G5B5,   S1,   S1,   S1,  ONE, X, Y, Z, W),
        _(  r5g6b5,   R5G6B5,   S1,   S1,   S1,   S1, X, Y, Z, W),
        _(      a8,       L8, ZERO, ZERO, ZERO,   S1, X, X, X, X),
        { -1, ~0, ~0 }
};
#undef _

static nv_pict_texture_format_t *
NV40_GetPictTextureFormat(int format)
{
	int i = 0;

	while (NV40TextureFormat[i].pict_fmt != -1) {
		if (NV40TextureFormat[i].pict_fmt == format)
			return &NV40TextureFormat[i];
		i++;
	}

	return NULL;
}

#define SF(bf) (NV30_3D_BLEND_FUNC_SRC_RGB_##bf |                              \
		NV30_3D_BLEND_FUNC_SRC_ALPHA_##bf)
#define DF(bf) (NV30_3D_BLEND_FUNC_DST_RGB_##bf |                              \
		NV30_3D_BLEND_FUNC_DST_ALPHA_##bf)
static nv_pict_op_t 
NV40PictOp[] = {
/* Clear       */ { 0, 0, SF(               ZERO), DF(               ZERO) },
/* Src         */ { 0, 0, SF(                ONE), DF(               ZERO) },
/* Dst         */ { 0, 0, SF(               ZERO), DF(                ONE) },
/* Over        */ { 1, 0, SF(                ONE), DF(ONE_MINUS_SRC_ALPHA) },
/* OverReverse */ { 0, 1, SF(ONE_MINUS_DST_ALPHA), DF(                ONE) },
/* In          */ { 0, 1, SF(          DST_ALPHA), DF(               ZERO) },
/* InReverse   */ { 1, 0, SF(               ZERO), DF(          SRC_ALPHA) },
/* Out         */ { 0, 1, SF(ONE_MINUS_DST_ALPHA), DF(               ZERO) },
/* OutReverse  */ { 1, 0, SF(               ZERO), DF(ONE_MINUS_SRC_ALPHA) },
/* Atop        */ { 1, 1, SF(          DST_ALPHA), DF(ONE_MINUS_SRC_ALPHA) },
/* AtopReverse */ { 1, 1, SF(ONE_MINUS_DST_ALPHA), DF(          SRC_ALPHA) },
/* Xor         */ { 1, 1, SF(ONE_MINUS_DST_ALPHA), DF(ONE_MINUS_SRC_ALPHA) },
/* Add         */ { 0, 0, SF(                ONE), DF(                ONE) }
};

static nv_pict_op_t *
NV40_GetPictOpRec(int op)
{
	if (op >= PictOpSaturate)
		return NULL;
	return &NV40PictOp[op];
}

static void
NV40_SetupBlend(ScrnInfoPtr pScrn, nv_pict_op_t *blend,
		PictFormatShort dest_format, Bool component_alpha)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t sblend, dblend;

	sblend = blend->src_card_op;
	dblend = blend->dst_card_op;

	if (blend->dst_alpha) {
		if (!PICT_FORMAT_A(dest_format)) {
			if (sblend == SF(DST_ALPHA)) {
				sblend = SF(ONE);
			} else if (sblend == SF(ONE_MINUS_DST_ALPHA)) {
				sblend = SF(ZERO);
			}
		} else if (dest_format == PICT_a8) {
			if (sblend == SF(DST_ALPHA)) {
				sblend = SF(DST_COLOR);
			} else if (sblend == SF(ONE_MINUS_DST_ALPHA)) {
				sblend = SF(ONE_MINUS_DST_COLOR);
			}
		}
	}

	if (blend->src_alpha && (component_alpha || dest_format == PICT_a8)) {
		if (dblend == DF(SRC_ALPHA)) {
			dblend = DF(SRC_COLOR);
		} else if (dblend == DF(ONE_MINUS_SRC_ALPHA)) {
			dblend = DF(ONE_MINUS_SRC_COLOR);
		}
	}

	if (sblend == SF(ONE) && dblend == DF(ZERO)) {
		BEGIN_NV04(push, NV30_3D(BLEND_FUNC_ENABLE), 1);
		PUSH_DATA (push, 0);
	} else {
		BEGIN_NV04(push, NV30_3D(BLEND_FUNC_ENABLE), 5);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, sblend);
		PUSH_DATA (push, dblend);
		PUSH_DATA (push, 0x00000000);
		PUSH_DATA (push, NV40_3D_BLEND_EQUATION_ALPHA_FUNC_ADD |
				 NV40_3D_BLEND_EQUATION_RGB_FUNC_ADD);
	}
}

#if !defined(__AROS__)
static Bool
NV40EXAPictSolid(NVPtr pNv, PicturePtr pPict, int unit)
{
	struct nouveau_pushbuf *push = pNv->pushbuf;

	PUSH_DATAu(push, pNv->scratch, SOLID(unit), 2);
	PUSH_DATA (push, pPict->pSourcePict->solidFill.color);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(TEX_OFFSET(unit)), 8);
	PUSH_MTHDl(push, NV30_3D(TEX_OFFSET(unit)), pNv->scratch, SOLID(unit),
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	PUSH_DATA (push, NV40_3D_TEX_FORMAT_FORMAT_A8R8G8B8 | 0x8000 |
			 NV40_3D_TEX_FORMAT_LINEAR |
			 NV30_3D_TEX_FORMAT_DIMS_2D |
			 NV30_3D_TEX_FORMAT_NO_BORDER |
			 (1 << NV40_3D_TEX_FORMAT_MIPMAP_COUNT__SHIFT) |
			 NV30_3D_TEX_FORMAT_DMA0);
	PUSH_DATA (push, NV30_3D_TEX_WRAP_S_REPEAT |
			 NV30_3D_TEX_WRAP_T_REPEAT |
			 NV30_3D_TEX_WRAP_R_REPEAT);
	PUSH_DATA (push, NV40_3D_TEX_ENABLE_ENABLE);
	PUSH_DATA (push, 0x0000aae4);
	PUSH_DATA (push, NV30_3D_TEX_FILTER_MIN_NEAREST |
			 NV30_3D_TEX_FILTER_MAG_NEAREST | 0x3fd6);
	PUSH_DATA (push, 0x00010001);
	PUSH_DATA (push, 0x00000000);
	BEGIN_NV04(push, NV40_3D(TEX_SIZE1(unit)), 1);
	PUSH_DATA (push, 0x00100040);

	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_CONST_ID), 17);
	PUSH_DATA (push, unit * 4);
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
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	return TRUE;
}
#endif

static Bool
NV40EXAPictGradient(NVPtr pNv, PicturePtr pPict, int unit)
{
	return FALSE;
}

static Bool
NV40EXAPictTexture(NVPtr pNv, PixmapPtr pPix, PicturePtr pPict, int unit)
{
	unsigned reloc = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_WR;
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pPix);
	nv_pict_texture_format_t *fmt;

	fmt = NV40_GetPictTextureFormat(pPict->format);
	if (!fmt)
		return FALSE;

	BEGIN_NV04(push, NV30_3D(TEX_OFFSET(unit)), 8);
	PUSH_MTHDl(push, NV30_3D(TEX_OFFSET(unit)), bo, 0, reloc);
	PUSH_MTHDs(push, NV30_3D(TEX_FORMAT(unit)), bo, fmt->card_fmt |
			 NV40_3D_TEX_FORMAT_LINEAR |
			 NV30_3D_TEX_FORMAT_DIMS_2D | 0x8000 |
			 NV30_3D_TEX_FORMAT_NO_BORDER |
			 (1 << NV40_3D_TEX_FORMAT_MIPMAP_COUNT__SHIFT),
			 reloc | NOUVEAU_BO_OR,
			 NV30_3D_TEX_FORMAT_DMA0, NV30_3D_TEX_FORMAT_DMA1);
	if (pPict->repeat) {
		switch(pPict->repeatType) {
		case RepeatPad:
			PUSH_DATA (push, NV30_3D_TEX_WRAP_S_CLAMP_TO_EDGE |
					 NV30_3D_TEX_WRAP_T_CLAMP_TO_EDGE |
					 NV30_3D_TEX_WRAP_R_CLAMP_TO_EDGE);
			break;
		case RepeatReflect:
			PUSH_DATA (push, NV30_3D_TEX_WRAP_S_MIRRORED_REPEAT |
					 NV30_3D_TEX_WRAP_T_MIRRORED_REPEAT |
					 NV30_3D_TEX_WRAP_R_MIRRORED_REPEAT);
			break;
		case RepeatNormal:
		default:
			PUSH_DATA (push, NV30_3D_TEX_WRAP_S_REPEAT |
					 NV30_3D_TEX_WRAP_T_REPEAT |
					 NV30_3D_TEX_WRAP_R_REPEAT);
			break;
		}
	} else {
		PUSH_DATA (push, NV30_3D_TEX_WRAP_S_CLAMP_TO_BORDER |
				 NV30_3D_TEX_WRAP_T_CLAMP_TO_BORDER |
				 NV30_3D_TEX_WRAP_R_CLAMP_TO_BORDER);
	}
	PUSH_DATA (push, NV40_3D_TEX_ENABLE_ENABLE);
	PUSH_DATA (push, fmt->card_swz);
	if (pPict->filter == PictFilterBilinear) {
		PUSH_DATA (push, NV30_3D_TEX_FILTER_MIN_LINEAR |
				 NV30_3D_TEX_FILTER_MAG_LINEAR | 0x3fd6);
	} else {
		PUSH_DATA (push, NV30_3D_TEX_FILTER_MIN_NEAREST |
				 NV30_3D_TEX_FILTER_MAG_NEAREST | 0x3fd6);
	}
	PUSH_DATA (push, (pPix->drawable.width << 16) | pPix->drawable.height);
	PUSH_DATA (push, 0); /* border ARGB */
	BEGIN_NV04(push, NV40_3D(TEX_SIZE1(unit)), 1);
	PUSH_DATA (push, (1 << NV40_3D_TEX_SIZE1_DEPTH__SHIFT) |
			 (uint32_t)exaGetPixmapPitch(pPix));

	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_CONST_ID), 17);
	PUSH_DATA (push, unit * 4);
#if !defined(__AROS__)
	if (pPict->transform) {
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[0][0]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[0][1]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[0][2]));
		PUSH_DATAf(push, 0);
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[1][0]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[1][1]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[1][2]));
		PUSH_DATAf(push, 0);
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[2][0]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[2][1]));
		PUSH_DATAf(push, xFixedToFloat(pPict->transform->matrix[2][2]));
		PUSH_DATAf(push, 0);
	} else {
#else
	{ // we are not doing any transformations
#endif
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
	}
	PUSH_DATAf(push, 1.0 / pPix->drawable.width);
	PUSH_DATAf(push, 1.0 / pPix->drawable.height);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);

	return TRUE;
}

static Bool
NV40EXAPicture(NVPtr pNv, PixmapPtr ppix, PicturePtr ppict, int unit)
{
	if (ppict->pDrawable)
		return NV40EXAPictTexture(pNv, ppix, ppict, unit);

NOT_IMPLEMENTED_STOP
#if 0
	switch (ppict->pSourcePict->type) {
	case SourcePictTypeSolidFill:
		return NV40EXAPictSolid(pNv, ppict, unit);
	case SourcePictTypeLinear:
		return NV40EXAPictGradient(pNv, ppict, unit);
	default:
		break;
	}
#endif

	return FALSE;
}

static Bool
NV40_SetupSurface(ScrnInfoPtr pScrn, PixmapPtr pPix, PictFormatShort format)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pPix);
	nv_pict_surface_format_t *fmt;

	fmt = NV40_GetPictSurfaceFormat(format);
	if (!fmt) {
		ErrorF("AIII no format\n");
		return FALSE;
	}

	BEGIN_NV04(push, NV30_3D(RT_FORMAT), 3);
	PUSH_DATA (push, NV30_3D_RT_FORMAT_TYPE_LINEAR |
			 NV30_3D_RT_FORMAT_ZETA_Z24S8 | fmt->card_fmt);
	PUSH_DATA (push, exaGetPixmapPitch(pPix));
	PUSH_MTHDl(push, NV30_3D(COLOR0_OFFSET), bo, 0,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
	return TRUE;
}

#if !defined(__AROS__)
static Bool
NV40EXACheckCompositeTexture(PicturePtr pPict, PicturePtr pdPict, int op)
{
	nv_pict_texture_format_t *fmt;
	int w = 1, h = 1;

	if (pPict->pDrawable) {
		w = pPict->pDrawable->width;
		h = pPict->pDrawable->height;
	} else {
		switch (pPict->pSourcePict->type) {
		case SourcePictTypeSolidFill:
			break;
		default:
			NOUVEAU_FALLBACK("gradient\n");
		}
	}

	if ((w > 4096) || (h > 4096))
		NOUVEAU_FALLBACK("picture too large, %dx%d\n", w, h);

	fmt = NV40_GetPictTextureFormat(pPict->format);
	if (!fmt)
		NOUVEAU_FALLBACK("picture format 0x%08x not supported\n",
				pPict->format);

	if (pPict->filter != PictFilterNearest &&
	    pPict->filter != PictFilterBilinear)
		NOUVEAU_FALLBACK("filter 0x%x not supported\n", pPict->filter);

	/* Opengl and Render disagree on what should be sampled outside an XRGB 
	 * texture (with no repeating). Opengl has a hardcoded alpha value of 
	 * 1.0, while render expects 0.0. We assume that clipping is done for 
	 * untranformed sources.
	 */
	if (NV40PictOp[op].src_alpha && !pPict->repeat &&
		pPict->transform && (PICT_FORMAT_A(pPict->format) == 0)
		&& (PICT_FORMAT_A(pdPict->format) != 0))
		NOUVEAU_FALLBACK("REPEAT_NONE unsupported for XRGB source\n");

	return TRUE;
}

Bool
NV40EXACheckComposite(int op, PicturePtr psPict,
			      PicturePtr pmPict,
			      PicturePtr pdPict)
{
	nv_pict_surface_format_t *fmt;
	nv_pict_op_t *opr;

	opr = NV40_GetPictOpRec(op);
	if (!opr)
		NOUVEAU_FALLBACK("unsupported blend op 0x%x\n", op);

	fmt = NV40_GetPictSurfaceFormat(pdPict->format);
	if (!fmt)
		NOUVEAU_FALLBACK("dst picture format 0x%08x not supported\n",
				pdPict->format);

	if (!NV40EXACheckCompositeTexture(psPict, pdPict, op))
		NOUVEAU_FALLBACK("src picture\n");
	if (pmPict) {
		if (pmPict->componentAlpha && 
		    PICT_FORMAT_RGB(pmPict->format) &&
		    opr->src_alpha && opr->src_card_op != SF(ZERO))
			NOUVEAU_FALLBACK("mask CA + SA\n");
		if (!NV40EXACheckCompositeTexture(pmPict, pdPict, op))
			NOUVEAU_FALLBACK("mask picture\n");
	}

	return TRUE;
}
#endif

Bool
NV40EXAPrepareComposite(int op, PicturePtr psPict,
				PicturePtr pmPict,
				PicturePtr pdPict,
				PixmapPtr  psPix,
				PixmapPtr  pmPix,
				PixmapPtr  pdPix)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pdPix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	nv_pict_op_t *blend = NV40_GetPictOpRec(op);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t fragprog;

	if (!PUSH_SPACE(push, 128))
		NOUVEAU_FALLBACK("space\n");
	PUSH_RESET(push);

	NV40_SetupBlend(pScrn, blend, pdPict->format,
			(pmPict && pmPict->componentAlpha &&
			 PICT_FORMAT_RGB(pmPict->format)));

	if (!NV40_SetupSurface(pScrn, pdPix, pdPict->format) ||
	    !NV40EXAPicture(pNv, psPix, psPict, 0))
		return FALSE;

	if (pmPict) {
		if (!NV40EXAPicture(pNv, pmPix, pmPict, 1))
			return FALSE;

		if (pdPict->format == PICT_a8) {
			fragprog = PFP_C_A8;
		} else
		if (pmPict->componentAlpha && PICT_FORMAT_RGB(pmPict->format)) {
			if (blend->src_alpha)
				fragprog = PFP_CCASA;
			else
				fragprog = PFP_CCA;
		} else {
			fragprog = PFP_C;
		}
	} else {
		if (pdPict->format == PICT_a8)
			fragprog = PFP_S_A8;
		else
			fragprog = PFP_S;
	}

	BEGIN_NV04(push, NV30_3D(FP_ACTIVE_PROGRAM), 1);
	PUSH_MTHD (push, NV30_3D(FP_ACTIVE_PROGRAM), pNv->scratch, fragprog,
			 NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_LOW |
			 NOUVEAU_BO_OR,
			 NV30_3D_FP_ACTIVE_PROGRAM_DMA0,
			 NV30_3D_FP_ACTIVE_PROGRAM_DMA1);
	BEGIN_NV04(push, NV30_3D(FP_CONTROL), 1);
	PUSH_DATA (push, 0x02000000);

	/* Appears to be some kind of cache flush, needed here at least
	 * sometimes.. funky text rendering otherwise :)
	 */
	BEGIN_NV04(push, NV40_3D(TEX_CACHE_CTL), 1);
	PUSH_DATA (push, 2);
	BEGIN_NV04(push, NV40_3D(TEX_CACHE_CTL), 1);
	PUSH_DATA (push, 1);

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
NV40EXAComposite(PixmapPtr pdPix,
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
NV40EXADoneComposite(PixmapPtr pdPix)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pdPix->drawable.pScreen);
	nouveau_pushbuf_bufctx(NVPTR(pScrn)->pushbuf, NULL);
}

#define NV30_3D_CHIPSET_4X_MASK 0x00000baf
#define NV44TCL_CHIPSET_4X_MASK 0x00005450
Bool
NVAccelInitNV40TCL(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nv04_fifo *fifo = pNv->channel->data;
	uint32_t class = 0, chipset;
	int i;

	NVXVComputeBicubicFilter(pNv->scratch, XV_TABLE, XV_TABLE_SIZE);

	chipset = pNv->dev->chipset;
	if ((chipset & 0xf0) == NV_ARCH_40) {
		chipset &= 0xf;
		if (NV30_3D_CHIPSET_4X_MASK & (1<<chipset))
			class = NV40_3D_CLASS;
		else if (NV44TCL_CHIPSET_4X_MASK & (1<<chipset))
			class = NV44_3D_CLASS;
		else {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
					"NV40EXA: Unknown chipset NV4%1x\n", chipset);
			return FALSE;
		}
	} else if ( (chipset & 0xf0) == 0x60) {
		class = NV44_3D_CLASS;
	} else
		return TRUE;

	if (nouveau_object_new(pNv->channel, Nv3D, class, NULL, 0, &pNv->Nv3D))
		return FALSE;

	if (!PUSH_SPACE(push, 256))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(3D, OBJECT), 1);
	PUSH_DATA (push, pNv->Nv3D->handle);
	BEGIN_NV04(push, NV30_3D(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->notify0->handle);
	BEGIN_NV04(push, NV30_3D(DMA_TEXTURE0), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->gart);
	BEGIN_NV04(push, NV30_3D(DMA_COLOR0), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->vram);

	/* voodoo */
	BEGIN_NV04(push, SUBC_3D(0x1ea4), 3);
	PUSH_DATA (push, 0x00000010);
	PUSH_DATA (push, 0x01000100);
	PUSH_DATA (push, 0xff800006);
	BEGIN_NV04(push, SUBC_3D(0x1fc4), 1);
	PUSH_DATA (push, 0x06144321);
	BEGIN_NV04(push, SUBC_3D(0x1fc8), 2);
	PUSH_DATA (push, 0xedcba987);
	PUSH_DATA (push, 0x00000021);
	BEGIN_NV04(push, SUBC_3D(0x1fd0), 1);
	PUSH_DATA (push, 0x00171615);
	BEGIN_NV04(push, SUBC_3D(0x1fd4), 1);
	PUSH_DATA (push, 0x001b1a19);
	BEGIN_NV04(push, SUBC_3D(0x1ef8), 1);
	PUSH_DATA (push, 0x0020ffff);
	BEGIN_NV04(push, SUBC_3D(0x1d64), 1);
	PUSH_DATA (push, 0x00d30000);
	BEGIN_NV04(push, NV30_3D(ENGINE), 1);
	PUSH_DATA (push, NV30_3D_ENGINE_FP);

	/* This removes the the stair shaped tearing that i get. */
	/* Verified on one G70 card that it doesn't cause regressions for people without the problem. */
	/* The blob sets this up by default for NV43. */
	BEGIN_NV04(push, NV30_3D(FP_REG_CONTROL), 1);
	PUSH_DATA (push, 0x0000000F);

	BEGIN_NV04(push, NV30_3D(VIEWPORT_TRANSLATE_X), 8);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 1.0);
	PUSH_DATAf(push, 0.0);

	/* default 3D state */
	/*XXX: replace with the same state that the DRI emits on startup */
	BEGIN_NV04(push, NV30_3D(STENCIL_ENABLE(0)), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(STENCIL_ENABLE(1)), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(ALPHA_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(DEPTH_WRITE_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(COLOR_MASK), 1);
	PUSH_DATA (push, 0x01010101); /* TR,TR,TR,TR */
	BEGIN_NV04(push, NV30_3D(CULL_FACE_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(BLEND_FUNC_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(COLOR_LOGIC_OP_ENABLE), 2);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, NV30_3D_COLOR_LOGIC_OP_OP_COPY);
	BEGIN_NV04(push, NV30_3D(DITHER_ENABLE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(SHADE_MODEL), 1);
	PUSH_DATA (push, NV30_3D_SHADE_MODEL_SMOOTH);
	BEGIN_NV04(push, NV30_3D(POLYGON_OFFSET_FACTOR),2);
	PUSH_DATAf(push, 0.0);
	PUSH_DATAf(push, 0.0);
	BEGIN_NV04(push, NV30_3D(POLYGON_MODE_FRONT), 2);
	PUSH_DATA (push, NV30_3D_POLYGON_MODE_FRONT_FILL);
	PUSH_DATA (push, NV30_3D_POLYGON_MODE_BACK_FILL);
	BEGIN_NV04(push, NV30_3D(POLYGON_STIPPLE_PATTERN(0)), 0x20);
	for (i=0;i<0x20;i++)
		PUSH_DATA (push, 0xFFFFFFFF);
	for (i=0;i<16;i++) {
		BEGIN_NV04(push, NV30_3D(TEX_ENABLE(i)), 1);
		PUSH_DATA (push, 0);
	}

	BEGIN_NV04(push, NV30_3D(DEPTH_CONTROL), 1);
	PUSH_DATA (push, 0x110);

	BEGIN_NV04(push, NV30_3D(RT_ENABLE), 1);
	PUSH_DATA (push, NV30_3D_RT_ENABLE_COLOR0);

	BEGIN_NV04(push, NV30_3D(RT_HORIZ), 2);
	PUSH_DATA (push, (4096 << 16));
	PUSH_DATA (push, (4096 << 16));
	BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
	PUSH_DATA (push, (4096 << 16));
	PUSH_DATA (push, (4096 << 16));
	BEGIN_NV04(push, NV30_3D(VIEWPORT_HORIZ), 2);
	PUSH_DATA (push, (4096 << 16));
	PUSH_DATA (push, (4096 << 16));
	BEGIN_NV04(push, NV30_3D(VIEWPORT_CLIP_HORIZ(0)), 2);
	PUSH_DATA (push, (4095 << 16));
	PUSH_DATA (push, (4095 << 16));

	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_FROM_ID), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x401f9c6c); /* mov o[hpos], a[0] */
	PUSH_DATA (push, 0x0040000d);
	PUSH_DATA (push, 0x8106c083);
	PUSH_DATA (push, 0x6041ef80);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00001c6c); /* mov r0.xyw, a[8].xyww */
	PUSH_DATA (push, 0x0040080f);
	PUSH_DATA (push, 0x8106c083);
	PUSH_DATA (push, 0x6041affc);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00009c6c); /* dp3 r1.x, r0.xyw, c[0].xyz */
	PUSH_DATA (push, 0x0140000f);
	PUSH_DATA (push, 0x808680c3);
	PUSH_DATA (push, 0x60410ffc);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00009c6c); /* dp3 r1.y, r0.xyw, c[1].xyz */
	PUSH_DATA (push, 0x0140100f);
	PUSH_DATA (push, 0x808680c3);
	PUSH_DATA (push, 0x60408ffc);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00009c6c); /* dp3 r1.w, r0.xyw, c[2].xyz */
	PUSH_DATA (push, 0x0140200f);
	PUSH_DATA (push, 0x808680c3);
	PUSH_DATA (push, 0x60402ffc);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x401f9c6c); /* mul o[tex0].xyw, r1, c[3] */
	PUSH_DATA (push, 0x0080300d);
	PUSH_DATA (push, 0x8286c0c3);
	PUSH_DATA (push, 0x6041af9c);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00001c6c); /* mov r0.xyw, a[9].xyww */
	PUSH_DATA (push, 0x0040090f);
	PUSH_DATA (push, 0x8106c083);
	PUSH_DATA (push, 0x6041affc);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00009c6c); /* dp3 r1.x, r0.xyw, c[4].xyz */
	PUSH_DATA (push, 0x0140400f);
	PUSH_DATA (push, 0x808680c3);
	PUSH_DATA (push, 0x60410ffc);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00009c6c); /* dp3 r1.y, r0.xyw, c[5].xyz */
	PUSH_DATA (push, 0x0140500f);
	PUSH_DATA (push, 0x808680c3);
	PUSH_DATA (push, 0x60408ffc);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00009c6c); /* dp3 r1.w, r0.xyw, c[6].xyz */
	PUSH_DATA (push, 0x0140600f);
	PUSH_DATA (push, 0x808680c3);
	PUSH_DATA (push, 0x60402ffc);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x401f9c6c); /* exit mul o[tex1].xyw, r1, c[4] */
	PUSH_DATA (push, 0x0080700d);
	PUSH_DATA (push, 0x8286c0c3);
	PUSH_DATA (push, 0x6041afa1);
	BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
	PUSH_DATA (push, 0x00000000); /* exit */
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000000);
	PUSH_DATA (push, 0x00000001);
	BEGIN_NV04(push, NV30_3D(VP_START_FROM_ID), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV40_3D(VP_ATTRIB_EN), 2);
	PUSH_DATA (push, 0x00000309);
	PUSH_DATA (push, 0x0000c001);

	PUSH_DATAu(push, pNv->scratch, PFP_PASS, 1 * 4);
	PUSH_DATAs(push, 0x01403e81); /* mov r0, a[col0] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);

	PUSH_DATAu(push, pNv->scratch, PFP_S, 2 * 4);
	PUSH_DATAs(push, 0x18009e00); /* txp r0, a[tex0], t[0] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x01401e81); /* mov r0, r0 */
	PUSH_DATAs(push, 0x1c9dc800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);

	PUSH_DATAu(push, pNv->scratch, PFP_S_A8, 2 * 4);
	PUSH_DATAs(push, 0x18009000); /* txp r0.w, a[tex0], t[0] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x01401e81); /* mov r0, r0.w */
	PUSH_DATAs(push, 0x1c9dfe00);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);

	PUSH_DATAu(push, pNv->scratch, PFP_C, 3 * 4);
	PUSH_DATAs(push, 0x1802b102); /* txpc0 r1.w, a[tex1], t[1] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x18009e00); /* txp r0 (ne0.w), a[tex0], t[0] */
	PUSH_DATAs(push, 0x1ff5c801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x02001e81); /* mul r0, r0, r1.w */
	PUSH_DATAs(push, 0x1c9dc800);
	PUSH_DATAs(push, 0x0001fe04);
	PUSH_DATAs(push, 0x0001c800);

	PUSH_DATAu(push, pNv->scratch, PFP_C_A8, 3 * 4);
	PUSH_DATAs(push, 0x1802b102); /* txpc0 r1.w, a[tex1], t[1] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x18009000); /* txp r0.w (ne0.w), a[tex0], t[0] */
	PUSH_DATAs(push, 0x1ff5c801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x02001e81); /* mul r0, r0.w, r1.w */
	PUSH_DATAs(push, 0x1c9dfe00);
	PUSH_DATAs(push, 0x0001fe04);
	PUSH_DATAs(push, 0x0001c800);

	PUSH_DATAu(push, pNv->scratch, PFP_CCA, 3 * 4);
	PUSH_DATAs(push, 0x18009f00); /* txpc0 r0, a[tex0], t[0] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x1802be02); /* txp r1 (ne0), a[tex1], t[1] */
	PUSH_DATAs(push, 0x1c95c801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x02001e81); /* mul r0, r0, r1 */
	PUSH_DATAs(push, 0x1c9dc800);
	PUSH_DATAs(push, 0x0001c804);
	PUSH_DATAs(push, 0x0001c800);

	PUSH_DATAu(push, pNv->scratch, PFP_CCASA, 3 * 4);
	PUSH_DATAs(push, 0x18009102); /* txpc0 r1.w, a[tex0], t[0] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x1802be00); /* txp r0 (ne0.w), a[tex1], t[1] */
	PUSH_DATAs(push, 0x1ff5c801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x02001e81); /* mul r0, r1.w, r0 */
	PUSH_DATAs(push, 0x1c9dfe04);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);

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


	PUSH_DATAu(push, pNv->scratch, PFP_NV12_BICUBIC, 29 * 4);
	PUSH_DATAs(push, 0x01008600); /* movr r0.xy, a[tex0] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x03000800); /* addr r0.z, r0.y, imm.x */
	PUSH_DATAs(push, 0x1c9caa00);
	PUSH_DATAs(push, 0x00000002);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3f000000); /* { 0.50, 0.00, 0.00, 0.00 } */
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x03000202); /* addr r1.x, r0, imm.x */
	PUSH_DATAs(push, 0x1c9dc800);
	PUSH_DATAs(push, 0x00000002);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3f000000); /* { 0.50, 0.00, 0.00, 0.00 } */
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x17000f82); /* texrc0 r1.xyz, r0.z, t[0] */
	PUSH_DATAs(push, 0x1c9d5400);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x02001404); /* mulr r2.yw, r1.xxyy, imm.xxyy */
	PUSH_DATAs(push, 0x1c9ca104);
	PUSH_DATAs(push, 0x0000a002);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0xbf800000); /* { -1.00, 1.00, 0.00, 0.00 } */
	PUSH_DATAs(push, 0x3f800000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x17000e86); /* texr r3.xyz, r1, t[0] */
	PUSH_DATAs(push, 0x1c9dc804);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x02000a04); /* mulr r2.xz, r3.xxyy, imm.xxyy */
	PUSH_DATAs(push, 0x1c9ca10c);
	PUSH_DATAs(push, 0x0000a002);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0xbf800000); /* { -1.00, 1.00, 0.00, 0.00 } */
	PUSH_DATAs(push, 0x3f800000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x03001e04); /* addr r2, r0.xyxy, r2 */
	PUSH_DATAs(push, 0x1c9c8800);
	PUSH_DATAs(push, 0x0001c808);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x17020402); /* texr r1.y, r2.zwzz, -t[1] */
	PUSH_DATAs(push, 0x1c9d5c08);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x04400282); /* madh r1.x, -r1.z, r1.y, r1.y */
	PUSH_DATAs(push, 0x1c9f5504);
	PUSH_DATAs(push, 0x0000aa04);
	PUSH_DATAs(push, 0x0000aa04);
	PUSH_DATAs(push, 0x17020400); /* texr r0.y, r2.xwxw, -t[1] */
	PUSH_DATAs(push, 0x1c9d9808);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x04401080); /* madh r0.w, -r1.z, r0.y, r0.y */
	PUSH_DATAs(push, 0x1c9f5504);
	PUSH_DATAs(push, 0x0000aa00);
	PUSH_DATAs(push, 0x0000aa00);
	PUSH_DATAs(push, 0x17020200); /* texr r0.x, r2.zyxy, t[1] */
	PUSH_DATAs(push, 0x1c9c8c08);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x04400282); /* madh r1.x, r1.z, r0, r1 */
	PUSH_DATAs(push, 0x1c9d5504);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c904);
	PUSH_DATAs(push, 0x17020200); /* texr r0.x (NE0.z), r2, t[1] */
	PUSH_DATAs(push, 0x1555c808);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x04400280); /* madh r0.x, r1.z, r0, r0.w */
	PUSH_DATAs(push, 0x1c9d5504);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x0001ff00);
	PUSH_DATAs(push, 0x04401080); /* madh r0.w, -r3.z, r1.x, r1.x */
	PUSH_DATAs(push, 0x1c9f550c);
	PUSH_DATAs(push, 0x00000104);
	PUSH_DATAs(push, 0x00000104);
	PUSH_DATAs(push, 0x1704ac80); /* texr r0.yz, a[tex1], t[2] */
	PUSH_DATAs(push, 0x1c9dc801);
	PUSH_DATAs(push, 0x0001c800);
	PUSH_DATAs(push, 0x3fe1c800);
	PUSH_DATAs(push, 0x04400280); /* madh r0.x, r3.z, r0, r0.w */
	PUSH_DATAs(push, 0x1c9d550c);
	PUSH_DATAs(push, 0x0001c900);
	PUSH_DATAs(push, 0x0001ff00);
	PUSH_DATAs(push, 0x04400e82); /* madh r1.xyz, r0.x, imm.x, imm.yzww */
	PUSH_DATAs(push, 0x1c9c0100);
	PUSH_DATAs(push, 0x00000002);
	PUSH_DATAs(push, 0x0001f202);
	PUSH_DATAs(push, 0x3f9507c8); /* { 1.16, -0.87, 0.53, -1.08 } */
	PUSH_DATAs(push, 0xbf5ee393);
	PUSH_DATAs(push, 0x3f078fef);
	PUSH_DATAs(push, 0xbf8a6762);
	PUSH_DATAs(push, 0x04400e82); /* madh r1.xyz, r0.y, imm, r1 */
	PUSH_DATAs(push, 0x1c9cab00);
	PUSH_DATAs(push, 0x0001c802);
	PUSH_DATAs(push, 0x0001c904);
	PUSH_DATAs(push, 0x00000000); /* { 0.00, -0.39, 2.02, 0.00 } */
	PUSH_DATAs(push, 0xbec890d6);
	PUSH_DATAs(push, 0x40011687);
	PUSH_DATAs(push, 0x00000000);
	PUSH_DATAs(push, 0x04400e81); /* madh r0.xyz, r0.z, imm, r1 */
	PUSH_DATAs(push, 0x1c9d5500);
	PUSH_DATAs(push, 0x0001c802);
	PUSH_DATAs(push, 0x0001c904);
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
BOOL HIDDNouveauNV403DCopyBox(struct CardData * carddata,
    struct HIDDNouveauBitMapData * srcdata, struct HIDDNouveauBitMapData * destdata,
    LONG srcX, LONG srcY, LONG destX, LONG destY, LONG width, LONG height,
    ULONG blendop)
{
    struct Picture sPict, dPict;
    LONG maskX = 0; LONG maskY = 0;

    HIDDNouveauFillPictureFromBitMapData(&sPict, srcdata);
    HIDDNouveauFillPictureFromBitMapData(&dPict, destdata);

    if (NV40EXAPrepareComposite(blendop,
        &sPict, NULL, &dPict, srcdata, NULL, destdata))
    {
        NV40EXAComposite(destdata, srcX, srcY,
				      maskX, maskY,
				      destX , destY,
				      width, height);
        return TRUE;
    }

    return FALSE;
}
