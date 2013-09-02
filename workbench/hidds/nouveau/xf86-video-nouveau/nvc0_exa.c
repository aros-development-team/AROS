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
#include "nv50_texture.h"

#define NOUVEAU_BO(a, b, c) (NOUVEAU_BO_##a | NOUVEAU_BO_##b | NOUVEAU_BO_##c)

#if !defined(__AROS__)
Bool
NVC0AccelDownloadM2MF(PixmapPtr pspix, int x, int y, int w, int h,
		      char *dst, unsigned dst_pitch)
{
	ScrnInfoPtr pScrn = xf86Screens[pspix->drawable.pScreen->myNum];
#else
Bool
NVC0AccelDownloadM2MF(PixmapPtr pspix, int x, int y, int w, int h,
		      char *dst, unsigned dst_pitch,
		      HIDDT_StdPixFmt dstPixFmt, OOP_Class *cl, OOP_Object *o)
{
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pspix);
	struct nouveau_grobj *m2mf = pNv->NvMemFormat;
#if !defined(__AROS__)
	const int cpp = pspix->drawable.bitsPerPixel / 8;
	const int line_len = w * cpp;
	const int line_limit = (128 << 10) / line_len;
	unsigned src_offset = 0, src_pitch = 0, tiled = 1;
#else
	const int cpp = pspix->depth > 16 ? 4 : 2;
	const int line_len = w * cpp;
	const int line_limit = pNv->GART->size / line_len;
	unsigned src_offset = 0, src_pitch = 0, tiled = 1;
	unsigned int exec = (1 << 20) | NVC0_M2MF_EXEC_LINEAR_OUT;
#endif

	if (!nv50_style_tiled_pixmap(pspix)) {

		tiled = 0;
		src_pitch = exaGetPixmapPitch(pspix);
		src_offset = (y * src_pitch) + (x * cpp);
#if defined(__AROS__)
		exec |= NVC0_M2MF_EXEC_LINEAR_IN;
#endif
	} else {
		BEGIN_RING(chan, m2mf, NVC0_M2MF_TILING_MODE_IN, 5);
		OUT_RING  (chan, bo->tile_mode);
#if !defined(__AROS__)
		OUT_RING  (chan, pspix->drawable.width * cpp);
		OUT_RING  (chan, pspix->drawable.height);
#else
		OUT_RING  (chan, pspix->width * cpp);
		OUT_RING  (chan, pspix->height);
#endif
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 0);
	}

	while (h) {
		const char *src;
		int line_count, i;

		/* GART size >= 128 KiB assumed */
		line_count = h;
		if (line_count > line_limit)
			line_count = line_limit;

		MARK_RING(chan, 16, 4);

		BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
		OUT_RELOCh(chan, pNv->GART, 0, NOUVEAU_BO(GART, GART, WR));
		OUT_RELOCl(chan, pNv->GART, 0, NOUVEAU_BO(GART, GART, WR));

		BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_IN_HIGH, 6);
		OUT_RELOCh(chan, bo, src_offset, NOUVEAU_BO(VRAM, GART, RD));
		OUT_RELOCl(chan, bo, src_offset, NOUVEAU_BO(VRAM, GART, RD));
		OUT_RING  (chan, src_pitch);
		OUT_RING  (chan, line_len);
		OUT_RING  (chan, line_len);
		OUT_RING  (chan, line_count);

		if (tiled) {
			BEGIN_RING(chan, m2mf,
				   NVC0_M2MF_TILING_POSITION_IN_X, 2);
			OUT_RING  (chan, x * cpp);
			OUT_RING  (chan, y);
		}

		BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
#if !defined(__AROS__)
		OUT_RING  (chan, 0x100000 | (tiled << 8));
#else
		OUT_RING  (chan, exec | (tiled << 8));
#endif

		if (nouveau_bo_map(pNv->GART, NOUVEAU_BO_RD)) {
			MARK_UNDO(chan);
			return FALSE;
		}
		src = pNv->GART->map;

#if !defined(__AROS__)
		if (dst_pitch == line_len) {
			memcpy(dst, src, dst_pitch * line_count);
			dst += dst_pitch * line_count;
		} else {
			for (i = 0; i < line_count; ++i) {
				memcpy(dst, src, line_len);
				src += line_len;
				dst += dst_pitch;
			}
		}
#else
        (void)i;
        HiddNouveauReadIntoRAM(
            (char *)src, line_len,
            dst, dst_pitch, dstPixFmt,
            w, line_count,
            cl, o);
        dst += dst_pitch * line_count;
#endif
		nouveau_bo_unmap(pNv->GART);

		if (!tiled)
			src_offset += line_count * src_pitch;
		h -= line_count;
		y += line_count;
	}

	return TRUE;
}

#if !defined(__AROS__)
Bool
NVC0AccelUploadM2MF(PixmapPtr pdpix, int x, int y, int w, int h,
		    const char *src, int src_pitch)
{
	ScrnInfoPtr pScrn = xf86Screens[pdpix->drawable.pScreen->myNum];
#else
Bool
NVC0AccelUploadM2MF(PixmapPtr pdpix, int x, int y, int w, int h,
		    const char *src, int src_pitch,
		    HIDDT_StdPixFmt srcPixFmt, OOP_Class *cl, OOP_Object *o)
{
	ScrnInfoPtr pScrn = globalcarddataptr;
#endif
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_bo *bo = nouveau_pixmap_bo(pdpix);
	struct nouveau_grobj *m2mf = pNv->NvMemFormat;
#if !defined(__AROS__)
	int cpp = pdpix->drawable.bitsPerPixel / 8;
	int line_len = w * cpp;
	int line_limit = (128 << 10) / line_len;
	unsigned dst_offset = 0, dst_pitch = 0, tiled = 1;
#else
	int cpp = pdpix->depth > 16 ? 4 : 2;
	int line_len = w * cpp;
	int line_limit = pNv->GART->size / line_len;
	unsigned dst_offset = 0, dst_pitch = 0, tiled = 1;
	unsigned int exec = (1 << 20) | NVC0_M2MF_EXEC_LINEAR_IN;
#endif

	if (!nv50_style_tiled_pixmap(pdpix)) {
		tiled = 0;
		dst_pitch = exaGetPixmapPitch(pdpix);
		dst_offset = (y * dst_pitch) + (x * cpp);
#if defined(__AROS__)
		exec |= NVC0_M2MF_EXEC_LINEAR_OUT;
#endif
	} else {
		BEGIN_RING(chan, m2mf, NVC0_M2MF_TILING_MODE_OUT, 5);
		OUT_RING  (chan, bo->tile_mode);
#if !defined(__AROS__)
		OUT_RING  (chan, pdpix->drawable.width * cpp);
		OUT_RING  (chan, pdpix->drawable.height);
#else
		OUT_RING  (chan, pdpix->width * cpp);
		OUT_RING  (chan, pdpix->height);
#endif
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 0);
	}

	while (h) {
		char *dst;
		int i, line_count;

		line_count = h;
		if (line_count > line_limit)
			line_count = line_limit;

		if (nouveau_bo_map(pNv->GART, NOUVEAU_BO_WR))
			return FALSE;
		dst = pNv->GART->map;

#if !defined(__AROS__)
		if (src_pitch == line_len) {
			memcpy(dst, src, src_pitch * line_count);
			src += src_pitch * line_count;
		} else {
			for (i = 0; i < line_count; i++) {
				memcpy(dst, src, line_len);
				src += src_pitch;
				dst += line_len;
            }
		}
#else
        (void)i;
        HiddNouveauWriteFromRAM(
            (APTR)src, src_pitch, srcPixFmt,
            dst, line_len,
            w, line_count,
            cl, o);
        src += src_pitch * line_count;
#endif
		nouveau_bo_unmap(pNv->GART);

		if (MARK_RING(chan, 16, 4))
			return FALSE;

		BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_IN_HIGH, 2);
		OUT_RELOCh(chan, pNv->GART, 0, NOUVEAU_BO(GART, GART, RD));
		OUT_RELOCl(chan, pNv->GART, 0, NOUVEAU_BO(GART, GART, RD));

		BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
		OUT_RELOCh(chan, bo, dst_offset, NOUVEAU_BO(VRAM, GART, WR));
		OUT_RELOCl(chan, bo, dst_offset, NOUVEAU_BO(VRAM, GART, WR));

		if (tiled) {
			BEGIN_RING(chan, m2mf,
				   NVC0_M2MF_TILING_POSITION_OUT_X, 2);
			OUT_RING  (chan, x * cpp);
			OUT_RING  (chan, y);
		}

		BEGIN_RING(chan, m2mf, NVC0_M2MF_PITCH_IN, 4);
		OUT_RING  (chan, line_len);
		OUT_RING  (chan, dst_pitch);
		OUT_RING  (chan, line_len);
		OUT_RING  (chan, line_count);

		BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
#if !defined(__AROS__)
		OUT_RING  (chan, 0x100000 | (tiled << 4));
#else
		OUT_RING  (chan, exec | (tiled << 4));
#endif
		FIRE_RING (chan);

		if (!tiled)
			dst_offset += line_count * dst_pitch;
		h -= line_count;
		y += line_count;
	}

	return TRUE;
}

#if !defined(__AROS__)
struct nvc0_exa_state {
	struct {
		PictTransformPtr transform;
		float width;
		float height;
	} unit[2];

	Bool have_mask;
};

static struct nvc0_exa_state exa_state;
#endif

#if !defined(__AROS__)
#define NVC0EXA_LOCALS(p)                                              \
	ScrnInfoPtr pScrn = xf86Screens[(p)->drawable.pScreen->myNum]; \
	NVPtr pNv = NVPTR(pScrn);                                      \
	struct nouveau_channel *chan = pNv->chan; (void)chan;          \
	struct nouveau_grobj *m2mf = pNv->NvMemFormat; (void)m2mf;     \
	struct nouveau_grobj *eng2d = pNv->Nv2D; (void)eng2d;          \
	struct nouveau_grobj *fermi = pNv->Nv3D; (void)fermi;          \
	struct nvc0_exa_state *state = &exa_state; (void)state
#else
#define NVC0EXA_LOCALS(p)                                          \
	ScrnInfoPtr pScrn = globalcarddataptr;                         \
	NVPtr pNv = NVPTR(pScrn);                                      \
	struct nouveau_channel *chan = pNv->chan; (void)chan;          \
	struct nouveau_grobj *m2mf = pNv->NvMemFormat; (void)m2mf;     \
	struct nouveau_grobj *eng2d = pNv->Nv2D; (void)eng2d;          \
	struct nouveau_grobj *fermi = pNv->Nv3D; (void)fermi;
#endif

#if !defined(__AROS__)
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
#endif

static Bool
NVC0EXA2DSurfaceFormat(PixmapPtr ppix, uint32_t *fmt)
{
	NVC0EXA_LOCALS(ppix);

#if !defined(__AROS__)
	switch (ppix->drawable.bitsPerPixel) {
#else
	switch (ppix->depth) {
#endif
	case 8 : *fmt = NV50_2D_SRC_FORMAT_R8_UNORM; break;
	case 15: *fmt = NV50_2D_SRC_FORMAT_X1R5G5B5_UNORM; break;
	case 16: *fmt = NV50_2D_SRC_FORMAT_R5G6B5_UNORM; break;
	case 24: *fmt = NV50_2D_SRC_FORMAT_X8R8G8B8_UNORM; break;
	case 30: *fmt = NV50_2D_SRC_FORMAT_A2B10G10R10_UNORM; break;
	case 32: *fmt = NV50_2D_SRC_FORMAT_A8R8G8B8_UNORM; break;
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

	BEGIN_RING(chan, eng2d, NV50_2D_CLIP_X, 4);
	OUT_RING  (chan, x);
	OUT_RING  (chan, y);
	OUT_RING  (chan, w);
	OUT_RING  (chan, h);
}

static Bool
NVC0EXAAcquireSurface2D(PixmapPtr ppix, int is_src)
{
	NVC0EXA_LOCALS(ppix);
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
	int mthd = is_src ? NV50_2D_SRC_FORMAT : NV50_2D_DST_FORMAT;
	uint32_t fmt, bo_flags;

	if (!NVC0EXA2DSurfaceFormat(ppix, &fmt))
		return FALSE;

	bo_flags  = NOUVEAU_BO_VRAM;
	bo_flags |= is_src ? NOUVEAU_BO_RD : NOUVEAU_BO_WR;

	if (!nv50_style_tiled_pixmap(ppix)) {
		BEGIN_RING(chan, eng2d, mthd, 2);
		OUT_RING  (chan, fmt);
		OUT_RING  (chan, 1);
		BEGIN_RING(chan, eng2d, mthd + 0x14, 1);
		OUT_RING  (chan, (uint32_t)exaGetPixmapPitch(ppix));
	} else {
		BEGIN_RING(chan, eng2d, mthd, 5);
		OUT_RING  (chan, fmt);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, bo->tile_mode);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 0);
	}

	BEGIN_RING(chan, eng2d, mthd + 0x18, 4);
#if !defined(__AROS__)
	OUT_RING  (chan, ppix->drawable.width);
	OUT_RING  (chan, ppix->drawable.height);
#else
	OUT_RING  (chan, ppix->width);
	OUT_RING  (chan, ppix->height);
#endif
	if (OUT_RELOCh(chan, bo, 0, bo_flags) ||
	    OUT_RELOCl(chan, bo, 0, bo_flags))
		return FALSE;

	if (is_src == 0)
#if !defined(__AROS__)
		NVC0EXASetClip(ppix, 0, 0, ppix->drawable.width, ppix->drawable.height);
#else
		NVC0EXASetClip(ppix, 0, 0, ppix->width, ppix->height);
#endif

	return TRUE;
}

static void
NVC0EXASetPattern(PixmapPtr pdpix, int col0, int col1, int pat0, int pat1)
{
	NVC0EXA_LOCALS(pdpix);

	BEGIN_RING(chan, eng2d, NV50_2D_PATTERN_COLOR(0), 4);
	OUT_RING  (chan, col0);
	OUT_RING  (chan, col1);
	OUT_RING  (chan, pat0);
	OUT_RING  (chan, pat1);
}

#if !defined(__AROS__)
static void
NVC0EXASetROP(PixmapPtr pdpix, int alu, Pixel planemask)
{
	NVC0EXA_LOCALS(pdpix);
	int rop;

	if (planemask != ~0)
		rop = NVROP[alu].copy_planemask;
	else
		rop = NVROP[alu].copy;

	BEGIN_RING(chan, eng2d, NV50_2D_OPERATION, 1);
	if (alu == GXcopy && EXA_PM_IS_SOLID(&pdpix->drawable, planemask)) {
		OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY);
		return;
	} else {
		OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY_PREMULT);
	}

	BEGIN_RING(chan, eng2d, NV50_2D_PATTERN_FORMAT, 2);
	switch (pdpix->drawable.bitsPerPixel) {
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
		BEGIN_RING(chan, eng2d, NV50_2D_ROP, 1);
		OUT_RING  (chan, rop);
		pNv->currentRop = alu;
	}
}
#else
static void
NVC0EXASetROP(PixmapPtr pdpix, int alu, Pixel planemask)
{
	NVC0EXA_LOCALS(pdpix);
	LONG rop;

	rop = NVROP[alu].copy;

	BEGIN_RING(chan, eng2d, NV50_2D_OPERATION, 1);
	if (alu == 0x03 /* DrawMode_Copy */) {
		OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY);
		return;
	} else {
		OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY_PREMULT);
	}

	BEGIN_RING(chan, eng2d, NV50_2D_PATTERN_FORMAT, 2);
	switch (pdpix->depth) {
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

	BEGIN_RING(chan, eng2d, NV50_2D_ROP, 1);
	OUT_RING  (chan, rop);
}
#endif

#if !defined(__AROS__)
static void
NVC0EXAStateSolidResubmit(struct nouveau_channel *chan)
{
	ScrnInfoPtr pScrn = chan->user_private;
	NVPtr pNv = NVPTR(pScrn);

	NVC0EXAPrepareSolid(pNv->pdpix, pNv->alu, pNv->planemask,
			    pNv->fg_colour);
}
#endif

Bool
NVC0EXAPrepareSolid(PixmapPtr pdpix, int alu, Pixel planemask, Pixel fg)
{
	NVC0EXA_LOCALS(pdpix);
	uint32_t fmt;

	if (!NVC0EXA2DSurfaceFormat(pdpix, &fmt))
		NOUVEAU_FALLBACK("rect format\n");

	if (MARK_RING(chan, 64, 4))
		NOUVEAU_FALLBACK("ring space\n");

	if (!NVC0EXAAcquireSurface2D(pdpix, 0)) {
		MARK_UNDO(chan);
		NOUVEAU_FALLBACK("dest pixmap\n");
	}

	NVC0EXASetROP(pdpix, alu, planemask);

	BEGIN_RING(chan, eng2d, NV50_2D_DRAW_SHAPE, 3);
	OUT_RING  (chan, NV50_2D_DRAW_SHAPE_RECTANGLES);
	OUT_RING  (chan, fmt);
	OUT_RING  (chan, fg);

#if !defined(__AROS__)
	pNv->pdpix = pdpix;
	pNv->alu = alu;
	pNv->planemask = planemask;
	pNv->fg_colour = fg;
	chan->flush_notify = NVC0EXAStateSolidResubmit;
#else
	chan->flush_notify = NULL;
#endif
	return TRUE;
}

void
NVC0EXASolid(PixmapPtr pdpix, int x1, int y1, int x2, int y2)
{
	NVC0EXA_LOCALS(pdpix);

	WAIT_RING (chan, 5);
	BEGIN_RING(chan, eng2d, NV50_2D_DRAW_POINT32_X(0), 4);
	OUT_RING  (chan, x1);
	OUT_RING  (chan, y1);
	OUT_RING  (chan, x2);
	OUT_RING  (chan, y2);

#if !defined(__AROS__)
	if ((x2 - x1) * (y2 - y1) >= 512)
#endif
		FIRE_RING (chan);
}

#if !defined(__AROS__)
void
NVC0EXADoneSolid(PixmapPtr pdpix)
{
	NVC0EXA_LOCALS(pdpix);

	chan->flush_notify = NULL;
}

static void
NVC0EXAStateCopyResubmit(struct nouveau_channel *chan)
{
	ScrnInfoPtr pScrn = chan->user_private;
	NVPtr pNv = NVPTR(pScrn);

	NVC0EXAPrepareCopy(pNv->pspix, pNv->pdpix, 0, 0, pNv->alu,
			   pNv->planemask);
}
#endif

Bool
NVC0EXAPrepareCopy(PixmapPtr pspix, PixmapPtr pdpix, int dx, int dy,
		   int alu, Pixel planemask)
{
	NVC0EXA_LOCALS(pdpix);

	if (MARK_RING(chan, 64, 4))
		NOUVEAU_FALLBACK("ring space\n");

	if (!NVC0EXAAcquireSurface2D(pspix, 1)) {
		MARK_UNDO(chan);
		NOUVEAU_FALLBACK("src pixmap\n");
	}

	if (!NVC0EXAAcquireSurface2D(pdpix, 0)) {
		MARK_UNDO(chan);
		NOUVEAU_FALLBACK("dest pixmap\n");
	}

	NVC0EXASetROP(pdpix, alu, planemask);

#if !defined(__AROS__)
	pNv->pspix = pspix;
	pNv->pdpix = pdpix;
	pNv->alu = alu;
	pNv->planemask = planemask;
	chan->flush_notify = NVC0EXAStateCopyResubmit;
#else
	chan->flush_notify = NULL;
#endif
	return TRUE;
}

void
NVC0EXACopy(PixmapPtr pdpix, int srcX , int srcY,
			     int dstX , int dstY,
			     int width, int height)
{
	NVC0EXA_LOCALS(pdpix);

	WAIT_RING (chan, 17);
	BEGIN_RING(chan, eng2d, NV50_2D_SERIALIZE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, eng2d, 0x088c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, eng2d, NV50_2D_BLIT_DST_X, 12);
	OUT_RING  (chan, dstX);
	OUT_RING  (chan, dstY);
	OUT_RING  (chan, width);
	OUT_RING  (chan, height);
	OUT_RING  (chan, 0); /* DU,V_DX,Y_FRACT,INT */
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0); /* BLIT_SRC_X,Y_FRACT,INT */
	OUT_RING  (chan, srcX);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, srcY);

#if !defined(__AROS__)
	if (width * height >= 512)
#endif
		FIRE_RING (chan);
}

#if !defined(__AROS__)
void
NVC0EXADoneCopy(PixmapPtr pdpix)
{
	NVC0EXA_LOCALS(pdpix);

	chan->flush_notify = NULL;
}

static void
NVC0EXAStateSIFCResubmit(struct nouveau_channel *chan)
{
	ScrnInfoPtr pScrn = chan->user_private;
	NVPtr pNv = NVPTR(pScrn);
	
	if (MARK_RING(pNv->chan, 32, 2))
		return;

	if (!NVC0EXAAcquireSurface2D(pNv->pdpix, 0))
		MARK_UNDO(pNv->chan);
}

Bool
NVC0EXAUploadSIFC(const char *src, int src_pitch,
		  PixmapPtr pdpix, int x, int y, int w, int h, int cpp)
{
	NVC0EXA_LOCALS(pdpix);
	ScreenPtr pScreen = pdpix->drawable.pScreen;
	int line_dwords = (w * cpp + 3) / 4;
	uint32_t sifc_fmt;

	if (!NVC0EXA2DSurfaceFormat(pdpix, &sifc_fmt))
		NOUVEAU_FALLBACK("hostdata format\n");

	if (MARK_RING(chan, 64, 2))
		return FALSE;

	if (!NVC0EXAAcquireSurface2D(pdpix, 0)) {
		MARK_UNDO(chan);
		NOUVEAU_FALLBACK("dest pixmap\n");
	}

	/* If the pitch isn't aligned to a dword you can
	 * get corruption at the end of a line.
	 */
	NVC0EXASetClip(pdpix, x, y, w, h);

	BEGIN_RING(chan, eng2d, NV50_2D_OPERATION, 1);
	OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY);
	BEGIN_RING(chan, eng2d, NV50_2D_SIFC_BITMAP_ENABLE, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, sifc_fmt);
	BEGIN_RING(chan, eng2d, NV50_2D_SIFC_WIDTH, 10);
	OUT_RING  (chan, (line_dwords * 4) / cpp);
	OUT_RING  (chan, h);
	OUT_RING  (chan, 0); /* SIFC_DX,Y_DU,V_FRACT,INT */
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0); /* SIFC_DST_X,Y_FRACT,INT */
	OUT_RING  (chan, x);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, y);

	pNv->pdpix = pdpix;
	chan->flush_notify = NVC0EXAStateSIFCResubmit;

	while (h--) {
		const char *ptr = src;
		int count = line_dwords;

		while (count) {
			int size = count > 1792 ? 1792 : count;

			WAIT_RING (chan, size + 1);
			BEGIN_RING_NI(chan, eng2d, NV50_2D_SIFC_DATA, size);
			OUT_RINGp (chan, ptr, size);

			ptr += size * 4;
			count -= size;
		}

		src += src_pitch;
	}

	chan->flush_notify = NULL;

	if (pdpix == pScreen->GetScreenPixmap(pScreen))
		FIRE_RING(chan);
	return TRUE;
}

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
	case PICT_a8r8g8b8: format = NV50_SURFACE_FORMAT_A8R8G8B8_UNORM; break;
	case PICT_x8r8g8b8: format = NV50_SURFACE_FORMAT_X8R8G8B8_UNORM; break;
	case PICT_r5g6b5:   format = NV50_SURFACE_FORMAT_R5G6B5_UNORM; break;
	case PICT_a8:       format = NV50_SURFACE_FORMAT_A8_UNORM; break;
	case PICT_x1r5g5b5: format = NV50_SURFACE_FORMAT_X1R5G5B5_UNORM; break;
	case PICT_a1r5g5b5: format = NV50_SURFACE_FORMAT_A1R5G5B5_UNORM; break;
	case PICT_x8b8g8r8: format = NV50_SURFACE_FORMAT_X8B8G8R8_UNORM; break;
	case PICT_a2b10g10r10:
	case PICT_x2b10g10r10:
		format = NV50_SURFACE_FORMAT_A2B10G10R10_UNORM;
		break;
	case PICT_a2r10g10b10:
	case PICT_x2r10g10b10:
		format = NV50_SURFACE_FORMAT_A2R10G10B10_UNORM;
		break;
	default:
		NOUVEAU_FALLBACK("invalid picture format\n");
	}

	BEGIN_RING(chan, fermi, NVC0_3D_RT_ADDRESS_HIGH(0), 8);
	if (OUT_RELOCh(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR) ||
	    OUT_RELOCl(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR))
		return FALSE;
	OUT_RING  (chan, ppix->drawable.width);
	OUT_RING  (chan, ppix->drawable.height);
	OUT_RING  (chan, format);
	OUT_RING  (chan, bo->tile_mode);
	OUT_RING  (chan, 0x00000001);
	OUT_RING  (chan, 0x00000000);

	return TRUE;
}

static Bool
NVC0EXACheckTexture(PicturePtr ppict, PicturePtr pdpict, int op)
{
	if (!ppict->pDrawable)
		NOUVEAU_FALLBACK("Solid and gradient pictures unsupported.\n");

	if (ppict->pDrawable->width > 8192 ||
	    ppict->pDrawable->height > 8192)
		NOUVEAU_FALLBACK("texture dimensions exceeded %dx%d\n",
				 ppict->pDrawable->width,
				 ppict->pDrawable->height);

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

#define _(X1, X2, X3, X4, FMT)						\
	(NV50TIC_0_0_TYPER_UNORM | NV50TIC_0_0_TYPEG_UNORM |		\
	 NV50TIC_0_0_TYPEB_UNORM | NV50TIC_0_0_TYPEA_UNORM |		\
	 NV50TIC_0_0_MAP##X1 | NV50TIC_0_0_MAP##X2 |			\
	 NV50TIC_0_0_MAP##X3 | NV50TIC_0_0_MAP##X4 |			\
	 NV50TIC_0_0_FMT_##FMT)

static Bool
NVC0EXATexture(PixmapPtr ppix, PicturePtr ppict, unsigned unit)
{
	NVC0EXA_LOCALS(ppix);
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
	const unsigned tcb_flags = NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM;
	uint32_t mode;

	/* XXX: maybe add support for linear textures at some point */
	if (!nv50_style_tiled_pixmap(ppix))
		NOUVEAU_FALLBACK("pixmap is scanout buffer\n");

	BEGIN_RING(chan, fermi, NVC0_3D_TIC_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch, TIC_OFFSET, tcb_flags) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch, TIC_OFFSET, tcb_flags))
		return FALSE;
	OUT_RING  (chan, 15);

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, pNv->tesla_scratch,
		       TIC_OFFSET + unit * 32, tcb_flags) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch,
		       TIC_OFFSET + unit * 32, tcb_flags))
		return FALSE;
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 8 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 8);

	switch (ppict->format) {
	case PICT_a8r8g8b8:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_C3, 8_8_8_8));
		break;
	case PICT_a8b8g8r8:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_C3, 8_8_8_8));
		break;
	case PICT_x8r8g8b8:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_ONE, 8_8_8_8));
		break;
	case PICT_x8b8g8r8:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_ONE, 8_8_8_8));
		break;
	case PICT_r5g6b5:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_ONE, 5_6_5));
		break;
	case PICT_a8:
		OUT_RING(chan, _(A_C0, B_ZERO, G_ZERO, R_ZERO, 8));
		break;
	case PICT_x1r5g5b5:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_ONE, 1_5_5_5));
		break;
	case PICT_x1b5g5r5:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_ONE, 1_5_5_5));
		break;
	case PICT_a1r5g5b5:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_C3, 1_5_5_5));
		break;
	case PICT_a1b5g5r5:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_C3, 1_5_5_5));
		break;
	case PICT_b5g6r5:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_ONE, 5_6_5));
		break;
	case PICT_b8g8r8x8:
		OUT_RING(chan, _(A_ONE, R_C1, G_C2, B_C3, 8_8_8_8));
		break;
	case PICT_b8g8r8a8:
		OUT_RING(chan, _(A_C0, R_C1, G_C2, B_C3, 8_8_8_8));
		break;
	case PICT_a2b10g10r10:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_C3, 2_10_10_10));
		break;
	case PICT_x2b10g10r10:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_ONE, 2_10_10_10));
		break;
	case PICT_x2r10g10b10:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_ONE, 2_10_10_10));
		break;
	case PICT_a2r10g10b10:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_C3, 2_10_10_10));
		break;
	case PICT_x4r4g4b4:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_ONE, 4_4_4_4));
		break;
	case PICT_x4b4g4r4:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_ONE, 4_4_4_4));
		break;
	case PICT_a4r4g4b4:
		OUT_RING(chan, _(B_C0, G_C1, R_C2, A_C3, 4_4_4_4));
		break;
	case PICT_a4b4g4r4:
		OUT_RING(chan, _(R_C0, G_C1, B_C2, A_C3, 4_4_4_4));
		break;
	default:
		NOUVEAU_FALLBACK("invalid picture format, this SHOULD NOT HAPPEN. Expect trouble.\n");
	}
#undef _

	mode = 0xd0005000 | (bo->tile_mode << (22 - 4));
	if (OUT_RELOCl(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD) ||
	    OUT_RELOCd(chan, bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		       NOUVEAU_BO_HIGH | NOUVEAU_BO_OR, mode, mode))
		return FALSE;
	OUT_RING  (chan, 0x00300000);
	OUT_RING  (chan, (1 << 31) | ppix->drawable.width);
	OUT_RING  (chan, (1 << 16) | ppix->drawable.height);
	OUT_RING  (chan, 0x03000000);
	OUT_RING  (chan, 0x00000000);

	BEGIN_RING(chan, fermi, NVC0_3D_TSC_ADDRESS_HIGH, 3);
	if (OUT_RELOCh(chan, pNv->tesla_scratch, TSC_OFFSET, tcb_flags) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch, TSC_OFFSET, tcb_flags))
		return FALSE;
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, m2mf, NVC0_M2MF_OFFSET_OUT_HIGH, 2);
	if (OUT_RELOCh(chan, pNv->tesla_scratch,
		       TSC_OFFSET + unit * 32, tcb_flags) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch,
		       TSC_OFFSET + unit * 32, tcb_flags))
		return FALSE;
	BEGIN_RING(chan, m2mf, NVC0_M2MF_LINE_LENGTH_IN, 2);
	OUT_RING  (chan, 8 * 4);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, m2mf, NVC0_M2MF_EXEC, 1);
	OUT_RING  (chan, 0x100111);
	BEGIN_RING_NI(chan, m2mf, NVC0_M2MF_DATA, 8);

	if (ppict->repeat) {
		switch (ppict->repeatType) {
		case RepeatPad:
			OUT_RING  (chan, 0x00024000 |
				   NV50TSC_1_0_WRAPS_CLAMP |
				   NV50TSC_1_0_WRAPT_CLAMP |
				   NV50TSC_1_0_WRAPR_CLAMP);
			break;
		case RepeatReflect:
			OUT_RING  (chan, 0x00024000 |
				   NV50TSC_1_0_WRAPS_MIRROR_REPEAT |
				   NV50TSC_1_0_WRAPT_MIRROR_REPEAT |
				   NV50TSC_1_0_WRAPR_MIRROR_REPEAT);
			break;
		case RepeatNormal:
		default:
			OUT_RING  (chan, 0x00024000 |
				   NV50TSC_1_0_WRAPS_REPEAT |
				   NV50TSC_1_0_WRAPT_REPEAT |
				   NV50TSC_1_0_WRAPR_REPEAT);
			break;
		}
	} else {
		OUT_RING  (chan, 0x00024000 |
			   NV50TSC_1_0_WRAPS_CLAMP_TO_BORDER |
			   NV50TSC_1_0_WRAPT_CLAMP_TO_BORDER |
			   NV50TSC_1_0_WRAPR_CLAMP_TO_BORDER);
	}
	if (ppict->filter == PictFilterBilinear) {
		OUT_RING  (chan,
			   NV50TSC_1_1_MAGF_LINEAR |
			   NV50TSC_1_1_MINF_LINEAR | NV50TSC_1_1_MIPF_NONE);
	} else {
		OUT_RING  (chan,
			   NV50TSC_1_1_MAGF_NEAREST |
			   NV50TSC_1_1_MINF_NEAREST | NV50TSC_1_1_MIPF_NONE);
	}
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x00000000);
	OUT_RINGf (chan, 0.0f);
	OUT_RINGf (chan, 0.0f);
	OUT_RINGf (chan, 0.0f);
	OUT_RINGf (chan, 0.0f);

	state->unit[unit].width = ppix->drawable.width;
	state->unit[unit].height = ppix->drawable.height;
	state->unit[unit].transform = ppict->transform;
	return TRUE;
}

static Bool
NVC0EXACheckBlend(int op)
{
	if (op > PictOpAdd)
		NOUVEAU_FALLBACK("unsupported blend op %d\n", op);
	return TRUE;
}

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
		BEGIN_RING(chan, fermi, NVC0_3D_BLEND_ENABLE(0), 1);
		OUT_RING  (chan, 0);
	} else {
		BEGIN_RING(chan, fermi, NVC0_3D_BLEND_ENABLE(0), 1);
		OUT_RING  (chan, 1);
		BEGIN_RING(chan, fermi, NVC0_3D_BLEND_EQUATION_RGB, 5);
		OUT_RING  (chan, NVC0_3D_BLEND_EQUATION_RGB_FUNC_ADD);
		OUT_RING  (chan, sblend);
		OUT_RING  (chan, dblend);
		OUT_RING  (chan, NVC0_3D_BLEND_EQUATION_ALPHA_FUNC_ADD);
		OUT_RING  (chan, sblend);
		BEGIN_RING(chan, fermi, NVC0_3D_BLEND_FUNC_DST_ALPHA, 1);
		OUT_RING  (chan, dblend);
	}
}

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

static void
NVC0EXAStateCompositeResubmit(struct nouveau_channel *chan)
{
	ScrnInfoPtr pScrn = chan->user_private;
	NVPtr pNv = NVPTR(pScrn);

	NVC0EXAPrepareComposite(pNv->alu, pNv->pspict, pNv->pmpict, pNv->pdpict,
				pNv->pspix, pNv->pmpix, pNv->pdpix);
}

Bool
NVC0EXAPrepareComposite(int op,
			PicturePtr pspict, PicturePtr pmpict, PicturePtr pdpict,
			PixmapPtr pspix, PixmapPtr pmpix, PixmapPtr pdpix)
{
	NVC0EXA_LOCALS(pspix);
	const unsigned shd_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD;

	if (MARK_RING (chan, 128, 4 + 2 + 2 * 10))
		NOUVEAU_FALLBACK("ring space\n");

	// fonts: !pmpict, op == 12 (Add, ONE/ONE)
	/*
	if (pmpict || op != 12)
		NOUVEAU_FALLBACK("comp-alpha");
	*/

	BEGIN_RING(chan, eng2d, NV50_2D_SERIALIZE, 1);
	OUT_RING  (chan, 0);

	if (!NVC0EXARenderTarget(pdpix, pdpict)) {
		MARK_UNDO(chan);
		NOUVEAU_FALLBACK("render target invalid\n");
	}

	NVC0EXABlend(pdpix, pdpict, op, pmpict && pmpict->componentAlpha &&
		     PICT_FORMAT_RGB(pmpict->format));

	BEGIN_RING(chan, fermi, NVC0_3D_CODE_ADDRESS_HIGH, 2);
	if (OUT_RELOCh(chan, pNv->tesla_scratch, CODE_OFFSET, shd_flags) ||
	    OUT_RELOCl(chan, pNv->tesla_scratch, CODE_OFFSET, shd_flags)) {
		MARK_UNDO(chan);
		return FALSE;
	}

	if (!NVC0EXATexture(pspix, pspict, 0)) {
		MARK_UNDO(chan);
		NOUVEAU_FALLBACK("src picture invalid\n");
	}
	BEGIN_RING(chan, fermi, NVC0_3D_BIND_TIC(4), 1);
	OUT_RING  (chan, (0 << 9) | (0 << 1) | NVC0_3D_BIND_TIC_ACTIVE);

	if (pmpict) {
		if (!NVC0EXATexture(pmpix, pmpict, 1)) {
			MARK_UNDO(chan);
			NOUVEAU_FALLBACK("mask picture invalid\n");
		}
		state->have_mask = TRUE;

		BEGIN_RING(chan, fermi, NVC0_3D_BIND_TIC(4), 1);
		OUT_RING  (chan, (1 << 9) | (1 << 1) | NVC0_3D_BIND_TIC_ACTIVE);

		BEGIN_RING(chan, fermi, NVC0_3D_SP_START_ID(5), 1);
		if (pdpict->format == PICT_a8) {
			OUT_RING  (chan, PFP_C_A8);
		} else {
			if (pmpict->componentAlpha &&
			    PICT_FORMAT_RGB(pmpict->format)) {
				if (NVC0EXABlendOp[op].src_alpha)
					OUT_RING  (chan, PFP_CCASA);
				else
					OUT_RING  (chan, PFP_CCA);
			} else {
				OUT_RING  (chan, PFP_C);
			}
		}
	} else {
		state->have_mask = FALSE;

		BEGIN_RING(chan, fermi, NVC0_3D_BIND_TIC(4), 1);
		OUT_RING  (chan, (1 << 1) | 0);

		BEGIN_RING(chan, fermi, NVC0_3D_SP_START_ID(5), 1);
		if (pdpict->format == PICT_a8)
			OUT_RING  (chan, PFP_S_A8);
		else
			OUT_RING  (chan, PFP_S);
	}

	BEGIN_RING(chan, fermi, NVC0_3D_TSC_FLUSH, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_TIC_FLUSH, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, fermi, NVC0_3D_TEX_CACHE_CTL, 1);
	OUT_RING  (chan, 0);

	pNv->alu = op;
	pNv->pspict = pspict;
	pNv->pmpict = pmpict;
	pNv->pdpict = pdpict;
	pNv->pspix = pspix;
	pNv->pmpix = pmpix;
	pNv->pdpix = pdpix;
	chan->flush_notify = NVC0EXAStateCompositeResubmit;
	return TRUE;
}

#define xFixedToFloat(v) \
	((float)xFixedToInt((v)) + ((float)xFixedFrac(v) / 65536.0))

static inline void
NVC0EXATransform(PictTransformPtr t, int x, int y, float sx, float sy,
		 float *x_ret, float *y_ret)
{
	if (t) {
		PictVector v;

		v.vector[0] = IntToxFixed(x);
		v.vector[1] = IntToxFixed(y);
		v.vector[2] = xFixed1;
		PictureTransformPoint(t, &v);
		*x_ret = xFixedToFloat(v.vector[0]) / sx;
		*y_ret = xFixedToFloat(v.vector[1]) / sy;
	} else {
		*x_ret = (float)x / sx;
		*y_ret = (float)y / sy;
	}
}

void
NVC0EXAComposite(PixmapPtr pdpix,
		 int sx, int sy, int mx, int my,
		 int dx, int dy, int w, int h)
{
	NVC0EXA_LOCALS(pdpix);
	float sX0, sX1, sX2, sY0, sY1, sY2;

	WAIT_RING (chan, 64);
	BEGIN_RING(chan, fermi, NVC0_3D_SCISSOR_HORIZ(0), 2);
	OUT_RING  (chan, ((dx + w) << 16) | dx);
	OUT_RING  (chan, ((dy + h) << 16) | dy);
	BEGIN_RING(chan, fermi, NVC0_3D_VERTEX_BEGIN_GL, 1);
	OUT_RING  (chan, NVC0_3D_VERTEX_BEGIN_GL_PRIMITIVE_TRIANGLES);

	NVC0EXATransform(state->unit[0].transform, sx, sy + (h * 2),
			 state->unit[0].width, state->unit[0].height,
			 &sX0, &sY0);
	NVC0EXATransform(state->unit[0].transform, sx, sy,
			 state->unit[0].width, state->unit[0].height,
			 &sX1, &sY1);
	NVC0EXATransform(state->unit[0].transform, sx + (w * 2), sy,
			 state->unit[0].width, state->unit[0].height,
			 &sX2, &sY2);

	if (state->have_mask) {
		float mX0, mX1, mX2, mY0, mY1, mY2;

		NVC0EXATransform(state->unit[1].transform, mx, my + (h * 2),
				 state->unit[1].width, state->unit[1].height,
				 &mX0, &mY0);
		NVC0EXATransform(state->unit[1].transform, mx, my,
				 state->unit[1].width, state->unit[1].height,
				 &mX1, &mY1);
		NVC0EXATransform(state->unit[1].transform, mx + (w * 2), my,
				 state->unit[1].width, state->unit[1].height,
				 &mX2, &mY2);

		VTX2s(pNv, sX0, sY0, mX0, mY0, dx, dy + (h * 2));
		VTX2s(pNv, sX1, sY1, mX1, mY1, dx, dy);
		VTX2s(pNv, sX2, sY2, mX2, mY2, dx + (w * 2), dy);
	} else {
		VTX1s(pNv, sX0, sY0, dx, dy + (h * 2));
		VTX1s(pNv, sX1, sY1, dx, dy);
		VTX1s(pNv, sX2, sY2, dx + (w * 2), dy);
	}

	BEGIN_RING(chan, fermi, NVC0_3D_VERTEX_END_GL, 1);
	OUT_RING  (chan, 0);
}

void
NVC0EXADoneComposite(PixmapPtr pdpix)
{
	NVC0EXA_LOCALS(pdpix);

	chan->flush_notify = NULL;
}
#endif

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
