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

#if defined(__AROS__)
#include <aros/debug.h>
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

#include "nv_include.h"

#include "hwdefs/nv_object.xml.h"
#include "hwdefs/nv_m2mf.xml.h"
#include "hwdefs/nv01_2d.xml.h"
#include "hwdefs/nv50_2d.xml.h"
#include "nv04_accel.h"

#if !defined(__AROS__)
Bool
nouveau_allocate_surface(ScrnInfoPtr scrn, int width, int height, int bpp,
			 int usage_hint, int *pitch, struct nouveau_bo **bo)
{
	NVPtr pNv = NVPTR(scrn);
	Bool scanout = (usage_hint & NOUVEAU_CREATE_PIXMAP_SCANOUT) ? 1 : 0;
	Bool tiled = (usage_hint & NOUVEAU_CREATE_PIXMAP_TILED) ? 1 : 0;
	Bool shared = FALSE;
	union nouveau_bo_config cfg = {};
	int flags = NOUVEAU_BO_MAP | (bpp >= 8 ? NOUVEAU_BO_VRAM : 0);
	int cpp = bpp / 8, ret;

#ifdef NOUVEAU_PIXMAP_SHARING
	shared = ((usage_hint & 0xffff) == CREATE_PIXMAP_USAGE_SHARED);
#endif

	flags = NOUVEAU_BO_MAP;
	if (bpp >= 8)
		flags |= shared ? NOUVEAU_BO_GART : NOUVEAU_BO_VRAM;

	if (scanout && pNv->tiled_scanout)
		tiled = TRUE;

	if (pNv->Architecture >= NV_TESLA) {
		if (!scanout && bpp >= 8 && !shared)
			tiled = TRUE;

		*pitch = NOUVEAU_ALIGN(width * cpp, !tiled ? 256 : 64);
	} else {
		*pitch = NOUVEAU_ALIGN(width * cpp, 64);
	}

	if (tiled) {
		if (pNv->Architecture >= NV_FERMI) {
			if      (height > 64) cfg.nvc0.tile_mode = 0x040;
			else if (height > 32) cfg.nvc0.tile_mode = 0x030;
			else if (height > 16) cfg.nvc0.tile_mode = 0x020;
			else if (height >  8) cfg.nvc0.tile_mode = 0x010;
			else                  cfg.nvc0.tile_mode = 0x000;

			if (usage_hint & NOUVEAU_CREATE_PIXMAP_ZETA)
				cfg.nvc0.memtype = (bpp == 16) ? 0x01 : 0x11;
			else
				cfg.nvc0.memtype = 0xfe;

			height = NOUVEAU_ALIGN(height,
				 NVC0_TILE_HEIGHT(cfg.nvc0.tile_mode));
		} else if (pNv->Architecture >= NV_TESLA) {
			if      (height > 32) cfg.nv50.tile_mode = 0x040;
			else if (height > 16) cfg.nv50.tile_mode = 0x030;
			else if (height >  8) cfg.nv50.tile_mode = 0x020;
			else if (height >  4) cfg.nv50.tile_mode = 0x010;
			else                  cfg.nv50.tile_mode = 0x000;

			if (usage_hint & NOUVEAU_CREATE_PIXMAP_ZETA)
				cfg.nv50.memtype = (bpp == 16) ? 0x16c : 0x128;
			else if (usage_hint & NOUVEAU_CREATE_PIXMAP_SCANOUT)
				cfg.nv50.memtype = (bpp == 16) ? 0x070 : 0x07a;
			else
				cfg.nv50.memtype = 0x070;

			height = NOUVEAU_ALIGN(height,
				 NV50_TILE_HEIGHT(cfg.nv50.tile_mode));
		} else {
			int pitch_align = max(
				pNv->dev->chipset >= 0x40 ? 1024 : 256,
				round_down_pow2(*pitch / 4));

			*pitch = NOUVEAU_ALIGN(*pitch, pitch_align);
			cfg.nv04.surf_pitch = *pitch;
		}
	}

	if (pNv->Architecture < NV_TESLA) {
		if (bpp == 16)
			cfg.nv04.surf_flags |= NV04_BO_16BPP;
		if (bpp == 32)
			cfg.nv04.surf_flags |= NV04_BO_32BPP;
		if (usage_hint & NOUVEAU_CREATE_PIXMAP_ZETA)
			cfg.nv04.surf_flags |= NV04_BO_ZETA;
	}

	if (usage_hint & NOUVEAU_CREATE_PIXMAP_SCANOUT)
		flags |= NOUVEAU_BO_CONTIG;

	ret = nouveau_bo_new(pNv->dev, flags, 0, *pitch * height, &cfg, bo);
	if (ret) {
		ErrorF("%d\n", ret);
		return FALSE;
	}

	return TRUE;
}

void
NV11SyncToVBlank(PixmapPtr ppix, BoxPtr box)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(ppix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	int head;
	xf86CrtcPtr crtc;

	if (!nouveau_exa_pixmap_is_onscreen(ppix))
		return;

	crtc = nouveau_pick_best_crtc(pScrn, FALSE, box->x1, box->y1,
                                  box->x2 - box->x1,
                                  box->y2 - box->y1);
	if (!crtc)
		return;

	if (!PUSH_SPACE(push, 8))
		return;

	head = drmmode_head(crtc);

	BEGIN_NV04(push, NV15_BLIT(FLIP_INCR_WRITE), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV15_BLIT(FLIP_CRTC_INCR_READ), 1);
	PUSH_DATA (push, head);
	BEGIN_NV04(push, SUBC_BLIT(0x00000100), 1);
	PUSH_DATA (push, 0);
	BEGIN_NV04(push, NV15_BLIT(FLIP_WAIT), 1);
	PUSH_DATA (push, 0);
}
#endif

static Bool
NVAccelInitDmaNotifier0(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_object *chan = pNv->channel;
	struct nv04_notify ntfy = { .length = 32 };

	if (nouveau_object_new(chan, NvDmaNotifier0, NOUVEAU_NOTIFIER_CLASS,
			       &ntfy, sizeof(ntfy), &pNv->notify0))
		return FALSE;

	return TRUE;
}

static Bool
NVAccelInitNull(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);

	if (nouveau_object_new(pNv->channel, NvNullObject, NV01_NULL_CLASS,
			       NULL, 0, &pNv->NvNull))
		return FALSE;

	return TRUE;
}

static Bool
NVAccelInitContextSurfaces(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nv04_fifo *fifo = pNv->channel->data;
	uint32_t class;

	class = (pNv->Architecture >= NV_ARCH_10) ? NV10_SURFACE_2D_CLASS :
						    NV04_SURFACE_2D_CLASS;

	if (nouveau_object_new(pNv->channel, NvContextSurfaces, class,
			       NULL, 0, &pNv->NvContextSurfaces))
		return FALSE;

	if (!PUSH_SPACE(push, 8))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(SF2D, OBJECT), 1);
	PUSH_DATA (push, pNv->NvContextSurfaces->handle);
	BEGIN_NV04(push, NV04_SF2D(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	BEGIN_NV04(push, NV04_SF2D(DMA_IMAGE_SOURCE), 2);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, fifo->vram);
	return TRUE;
}

#if !defined(__AROS__)
static Bool
NVAccelInitContextBeta1(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (nouveau_object_new(pNv->channel, NvContextBeta1, NV01_BETA_CLASS,
			       NULL, 0, &pNv->NvContextBeta1))
		return FALSE;

	if (!PUSH_SPACE(push, 4))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvContextBeta1->handle);
	BEGIN_NV04(push, NV01_BETA(BETA_1D31), 1); /*alpha factor*/
	PUSH_DATA (push, 0xff << 23);
	return TRUE;
}


static Bool
NVAccelInitContextBeta4(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	
	if (nouveau_object_new(pNv->channel, NvContextBeta4, NV04_BETA4_CLASS,
			       NULL, 0, &pNv->NvContextBeta4))
		return FALSE;

	if (!PUSH_SPACE(push, 4))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvContextBeta4->handle);
	BEGIN_NV04(push, NV04_BETA4(BETA_FACTOR), 1); /*RGBA factor*/
	PUSH_DATA (push, 0xffff0000);
	return TRUE;
}
#endif

Bool
NVAccelGetCtxSurf2DFormatFromPixmap(PixmapPtr pPix, int *fmt_ret)
{
	switch (pPix->drawable.bitsPerPixel) {
	case 32:
		*fmt_ret = NV04_SURFACE_2D_FORMAT_A8R8G8B8;
		break;
	case 24:
		*fmt_ret = NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8;
		break;
	case 16:
		if (pPix->drawable.depth == 16)
			*fmt_ret = NV04_SURFACE_2D_FORMAT_R5G6B5;
		else
			*fmt_ret = NV04_SURFACE_2D_FORMAT_X1R5G5B5_Z1R5G5B5;
		break;
	case 8:
		*fmt_ret = NV04_SURFACE_2D_FORMAT_Y8;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

#if !defined(__AROS__)
Bool
NVAccelGetCtxSurf2DFormatFromPicture(PicturePtr pPict, int *fmt_ret)
{
	switch (pPict->format) {
	case PICT_a8r8g8b8:
		*fmt_ret = NV04_SURFACE_2D_FORMAT_A8R8G8B8;
		break;
	case PICT_x8r8g8b8:
		*fmt_ret = NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8;
		break;
	case PICT_r5g6b5:
		*fmt_ret = NV04_SURFACE_2D_FORMAT_R5G6B5;
		break;
	case PICT_a8:
		*fmt_ret = NV04_SURFACE_2D_FORMAT_Y8;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

/* A copy of exaGetOffscreenPixmap(), since it's private. */
PixmapPtr
NVGetDrawablePixmap(DrawablePtr pDraw)
{
	if (pDraw->type == DRAWABLE_WINDOW)
		return pDraw->pScreen->GetWindowPixmap ((WindowPtr) pDraw);
	else
		return (PixmapPtr) pDraw;
}
#endif

static Bool
NVAccelInitImagePattern(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (nouveau_object_new(pNv->channel, NvImagePattern, NV04_PATTERN_CLASS,
			       NULL, 0, &pNv->NvImagePattern))
		return FALSE;

	if (!PUSH_SPACE(push, 8))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvImagePattern->handle);
	BEGIN_NV04(push, NV01_PATT(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	BEGIN_NV04(push, NV01_PATT(MONOCHROME_FORMAT), 3);
#if X_BYTE_ORDER == X_BIG_ENDIAN
	PUSH_DATA (push, NV01_PATTERN_MONOCHROME_FORMAT_LE);
#else
	PUSH_DATA (push, NV01_PATTERN_MONOCHROME_FORMAT_CGA6);
#endif
	PUSH_DATA (push, NV01_PATTERN_MONOCHROME_SHAPE_8X8);
	PUSH_DATA (push, NV04_PATTERN_PATTERN_SELECT_MONO);

	return TRUE;
}

static Bool
NVAccelInitRasterOp(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (nouveau_object_new(pNv->channel, NvRop, NV03_ROP_CLASS,
			       NULL, 0, &pNv->NvRop))
		return FALSE;

	if (!PUSH_SPACE(push, 4))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvRop->handle);
	BEGIN_NV04(push, NV01_ROP(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->NvNull->handle);

	pNv->currentRop = ~0;
	return TRUE;
}

static Bool
NVAccelInitRectangle(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (nouveau_object_new(pNv->channel, NvRectangle, NV04_GDI_CLASS,
			       NULL, 0, &pNv->NvRectangle))
		return FALSE;

	if (!PUSH_SPACE(push, 16))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(RECT, OBJECT), 1);
	PUSH_DATA (push, pNv->NvRectangle->handle);
	BEGIN_NV04(push, NV04_RECT(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->notify0->handle);
	BEGIN_NV04(push, NV04_RECT(DMA_FONTS), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	BEGIN_NV04(push, NV04_RECT(SURFACE), 1);
	PUSH_DATA (push, pNv->NvContextSurfaces->handle);
	BEGIN_NV04(push, NV04_RECT(ROP), 1);
	PUSH_DATA (push, pNv->NvRop->handle);
	BEGIN_NV04(push, NV04_RECT(PATTERN), 1);
	PUSH_DATA (push, pNv->NvImagePattern->handle);
	BEGIN_NV04(push, NV04_RECT(OPERATION), 1);
	PUSH_DATA (push, NV04_GDI_OPERATION_ROP_AND);
	BEGIN_NV04(push, NV04_RECT(MONOCHROME_FORMAT), 1);
	/* XXX why putting 1 like renouveau dump, swap the text */
#if 1 || X_BYTE_ORDER == X_BIG_ENDIAN
	PUSH_DATA (push, NV04_GDI_MONOCHROME_FORMAT_LE);
#else
	PUSH_DATA (push, NV04_GDI_MONOCHROME_FORMAT_CGA6);
#endif

	return TRUE;
}

static Bool
NVAccelInitImageBlit(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t class;

	class = (pNv->dev->chipset >= 0x11) ? NV15_BLIT_CLASS : NV04_BLIT_CLASS;

	if (nouveau_object_new(pNv->channel, NvImageBlit, class,
			       NULL, 0, &pNv->NvImageBlit))
		return FALSE;

	if (!PUSH_SPACE(push, 16))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(BLIT, OBJECT), 1);
	PUSH_DATA (push, pNv->NvImageBlit->handle);
	BEGIN_NV04(push, NV01_BLIT(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->notify0->handle);
	BEGIN_NV04(push, NV01_BLIT(COLOR_KEY), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	BEGIN_NV04(push, NV04_BLIT(SURFACES), 1);
	PUSH_DATA (push, pNv->NvContextSurfaces->handle);
	BEGIN_NV04(push, NV01_BLIT(CLIP), 3);
	PUSH_DATA (push, pNv->NvNull->handle);
	PUSH_DATA (push, pNv->NvImagePattern->handle);
	PUSH_DATA (push, pNv->NvRop->handle);
	BEGIN_NV04(push, NV01_BLIT(OPERATION), 1);
	PUSH_DATA (push, NV01_BLIT_OPERATION_ROP_AND);
	if (pNv->NvImageBlit->oclass == NV15_BLIT_CLASS) {
		BEGIN_NV04(push, NV15_BLIT(FLIP_SET_READ), 3);
		PUSH_DATA (push, 0);
		PUSH_DATA (push, 1);
		PUSH_DATA (push, 2);
	}

	return TRUE;
}

#if !defined(__AROS__)
static Bool
NVAccelInitScaledImage(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	struct nv04_fifo *fifo = pNv->channel->data;
	uint32_t class;

	switch (pNv->Architecture) {
	case NV_ARCH_04:
		class = NV04_SIFM_CLASS;
		break;
	case NV_ARCH_10:
	case NV_ARCH_20:
	case NV_ARCH_30:
		class = NV10_SIFM_CLASS;
		break;
	case NV_ARCH_40:
	default:
		class = NV40_SIFM_CLASS;
		break;
	}

	if (nouveau_object_new(pNv->channel, NvScaledImage, class,
			       NULL, 0, &pNv->NvScaledImage))
		return FALSE;

	if (!PUSH_SPACE(push, 16))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvScaledImage->handle);
	BEGIN_NV04(push, NV03_SIFM(DMA_NOTIFY), 7);
	PUSH_DATA (push, pNv->notify0->handle);
	PUSH_DATA (push, fifo->vram);
	PUSH_DATA (push, pNv->NvNull->handle);
	PUSH_DATA (push, pNv->NvNull->handle);
	PUSH_DATA (push, pNv->NvContextBeta1->handle);
	PUSH_DATA (push, pNv->NvContextBeta4->handle);
	PUSH_DATA (push, pNv->NvContextSurfaces->handle);
	if (pNv->Architecture>=NV_ARCH_10) {
		BEGIN_NV04(push, NV05_SIFM(COLOR_CONVERSION), 1);
		PUSH_DATA (push, NV05_SIFM_COLOR_CONVERSION_DITHER);
	}
	BEGIN_NV04(push, NV03_SIFM(OPERATION), 1);
	PUSH_DATA (push, NV03_SIFM_OPERATION_SRCCOPY);

	return TRUE;
}

static Bool
NVAccelInitClipRectangle(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (nouveau_object_new(pNv->channel, NvClipRectangle, NV01_CLIP_CLASS,
			       NULL, 0, &pNv->NvClipRectangle))
		return FALSE;

	if (!PUSH_SPACE(push, 4))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(MISC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvClipRectangle->handle);
	BEGIN_NV04(push, NV01_CLIP(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	return TRUE;
}
#endif

static Bool
NVAccelInitMemFormat(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;

	if (nouveau_object_new(pNv->channel, NvMemFormat, NV03_M2MF_CLASS,
			       NULL, 0, &pNv->NvMemFormat))
		return FALSE;

	if (!PUSH_SPACE(push, 4))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(M2MF, OBJECT), 1);
	PUSH_DATA (push, pNv->NvMemFormat->handle);
	BEGIN_NV04(push, NV03_M2MF(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->notify0->handle);
	return TRUE;
}

#if !defined(__AROS__)
static Bool
NVAccelInitImageFromCpu(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_pushbuf *push = pNv->pushbuf;
	uint32_t class;

	switch (pNv->Architecture) {
	case NV_ARCH_04:
		class = NV04_IFC_CLASS;
		break;
	case NV_ARCH_10:
	case NV_ARCH_20:
	case NV_ARCH_30:
	case NV_ARCH_40:
	default:
		class = NV10_IFC_CLASS;
		break;
	}

	if (nouveau_object_new(pNv->channel, NvImageFromCpu, class,
			       NULL, 0, &pNv->NvImageFromCpu))
		return FALSE;

	if (!PUSH_SPACE(push, 16))
		return FALSE;

	BEGIN_NV04(push, NV01_SUBC(IFC, OBJECT), 1);
	PUSH_DATA (push, pNv->NvImageFromCpu->handle);
	BEGIN_NV04(push, NV01_IFC(DMA_NOTIFY), 1);
	PUSH_DATA (push, pNv->notify0->handle);
	BEGIN_NV04(push, NV01_IFC(CLIP), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	BEGIN_NV04(push, NV01_IFC(PATTERN), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	BEGIN_NV04(push, NV01_IFC(ROP), 1);
	PUSH_DATA (push, pNv->NvNull->handle);
	if (pNv->Architecture >= NV_ARCH_10) {
		BEGIN_NV04(push, NV01_IFC(BETA), 1);
		PUSH_DATA (push, pNv->NvNull->handle);
		BEGIN_NV04(push, NV04_IFC(BETA4), 1);
		PUSH_DATA (push, pNv->NvNull->handle);
	}
	BEGIN_NV04(push, NV04_IFC(SURFACE), 1);
	PUSH_DATA (push, pNv->NvContextSurfaces->handle);
	BEGIN_NV04(push, NV01_IFC(OPERATION), 1);
	PUSH_DATA (push, NV01_IFC_OPERATION_SRCCOPY);
	return TRUE;
}
#endif

#define INIT_CONTEXT_OBJECT(name) do {                                        \
	ret = NVAccelInit##name(pScrn);                                       \
	if (!ret) {                                                           \
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,                         \
			   "Failed to initialise context object: "#name       \
			   " (%d)\n", ret);                                   \
		return FALSE;                                                 \
	}                                                                     \
} while(0)

void
NVAccelCommonFini(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);

	nouveau_object_del(&pNv->notify0);
	nouveau_object_del(&pNv->vblank_sem);

	nouveau_object_del(&pNv->NvContextSurfaces);
#if !defined(__AROS__)
	nouveau_object_del(&pNv->NvContextBeta1);
	nouveau_object_del(&pNv->NvContextBeta4);
#endif
	nouveau_object_del(&pNv->NvImagePattern);
	nouveau_object_del(&pNv->NvRop);
	nouveau_object_del(&pNv->NvRectangle);
	nouveau_object_del(&pNv->NvImageBlit);
#if !defined(__AROS__)
	nouveau_object_del(&pNv->NvScaledImage);
	nouveau_object_del(&pNv->NvClipRectangle);
	nouveau_object_del(&pNv->NvImageFromCpu);
#endif
	nouveau_object_del(&pNv->Nv2D);
	nouveau_object_del(&pNv->NvMemFormat);
	nouveau_object_del(&pNv->NvSW);
	nouveau_object_del(&pNv->Nv3D);
	nouveau_object_del(&pNv->NvCOPY);

	nouveau_bo_ref(NULL, &pNv->scratch);

	nouveau_bufctx_del(&pNv->bufctx);
	nouveau_pushbuf_del(&pNv->pushbuf);
	nouveau_object_del(&pNv->channel);
}

Bool
NVAccelCommonInit(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nv04_fifo nv04_data = { .vram = NvDmaFB,
				       .gart = NvDmaTT };
	struct nvc0_fifo nvc0_data = { };
	struct nouveau_object *device = &pNv->dev->object;
	int size, ret;
	void *data;

	if (pNv->dev->drm_version < 0x01000000 && pNv->dev->chipset >= 0xc0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Fermi acceleration not supported on old kernel\n");
		return FALSE;
	}

	if (pNv->Architecture < NV_FERMI) {
		data = &nv04_data;
		size = sizeof(nv04_data);
	} else {
		data = &nvc0_data;
		size = sizeof(nvc0_data);
	}

	ret = nouveau_object_new(device, 0, NOUVEAU_FIFO_CHANNEL_CLASS,
				 data, size, &pNv->channel);
	if (ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Error creating GPU channel: %d\n", ret);
		return FALSE;
	}

	ret = nouveau_pushbuf_new(pNv->client, pNv->channel, 4, 32 * 1024,
				  true, &pNv->pushbuf);
	if (ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Error allocating DMA push buffer: %d\n",ret);
		NVAccelCommonFini(pScrn);
		return FALSE;
	}

	ret = nouveau_bufctx_new(pNv->client, 1, &pNv->bufctx);
	if (ret) {
		NVAccelCommonFini(pScrn);
		return FALSE;
	}

	pNv->pushbuf->user_priv = pNv->bufctx;

	/* Scratch buffer */
	ret = nouveau_bo_new(pNv->dev, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP,
			     128 * 1024, 128 * 1024, NULL, &pNv->scratch);
	if (!ret)
		ret = nouveau_bo_map(pNv->scratch, 0, pNv->client);
	if (ret) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Failed to allocate scratch buffer: %d\n", ret);
		return FALSE;
	}

	/* General engine objects */
	if (pNv->Architecture < NV_FERMI) {
		INIT_CONTEXT_OBJECT(DmaNotifier0);
		INIT_CONTEXT_OBJECT(Null);
	}

	/* 2D engine */
	if (pNv->Architecture < NV_TESLA) {
		INIT_CONTEXT_OBJECT(ContextSurfaces);
#if !defined(__AROS__)
		INIT_CONTEXT_OBJECT(ContextBeta1);
		INIT_CONTEXT_OBJECT(ContextBeta4);
#endif
		INIT_CONTEXT_OBJECT(ImagePattern);
		INIT_CONTEXT_OBJECT(RasterOp);
		INIT_CONTEXT_OBJECT(Rectangle);
		INIT_CONTEXT_OBJECT(ImageBlit);
#if !defined(__AROS__)
		INIT_CONTEXT_OBJECT(ScaledImage);
		INIT_CONTEXT_OBJECT(ClipRectangle);
		INIT_CONTEXT_OBJECT(ImageFromCpu);
#endif
	} else
	if (pNv->Architecture < NV_FERMI) {
		INIT_CONTEXT_OBJECT(2D_NV50);
	} else {
		INIT_CONTEXT_OBJECT(2D_NVC0);
	}

	if (pNv->Architecture < NV_TESLA)
		INIT_CONTEXT_OBJECT(MemFormat);
	else
	if (pNv->Architecture < NV_FERMI)
		INIT_CONTEXT_OBJECT(M2MF_NV50);
	else
	if (pNv->Architecture < NV_KEPLER)
		INIT_CONTEXT_OBJECT(M2MF_NVC0);
	else {
		INIT_CONTEXT_OBJECT(P2MF_NVE0);
		INIT_CONTEXT_OBJECT(COPY_NVE0);
	}

	/* 3D init */
	switch (pNv->Architecture) {
	case NV_FERMI:
	case NV_KEPLER:
	case NV_MAXWELL:
	case NV_PASCAL:
		INIT_CONTEXT_OBJECT(3D_NVC0);
		break;
	case NV_TESLA:
		INIT_CONTEXT_OBJECT(NV50TCL);
		break;
	case NV_ARCH_40:
		INIT_CONTEXT_OBJECT(NV40TCL);
		break;
	case NV_ARCH_30:
		INIT_CONTEXT_OBJECT(NV30TCL);
		break;
	case NV_ARCH_20:
	case NV_ARCH_10:
		INIT_CONTEXT_OBJECT(NV10TCL);
		break;
	default:
		break;
	}

	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Channel setup complete.\n");
	return TRUE;
}
/* AROS CODE */

BOOL HIDDNouveauAccelCommonInit(struct CardData * carddata)
{
    return NVAccelCommonInit(carddata);
}
