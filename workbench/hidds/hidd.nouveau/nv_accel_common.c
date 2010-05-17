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

#if !defined(__AROS__)
#include "nv_include.h"
#else
#include "nouveau_intern.h"
#include <aros/debug.h>
#include "nouveau/nouveau_class.h"

/* Some overriding defines for AROS */
#define Bool            BOOL
#define ScrnInfoPtr     struct CardData *
#define NVPTR(x)        x
#define NVPtr           struct CardData *
#define Architecture    architecture
#define PixmapPtr       struct HIDDNouveauBitMapData *
#endif

static Bool
NVAccelInitDmaNotifier0(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);

	if (!pNv->notify0) {
		if (nouveau_notifier_alloc(pNv->chan, NvDmaNotifier0, 1,
					   &pNv->notify0))
			return FALSE;
	}

	return TRUE;
}

/* FLAGS_ROP_AND, DmaFB, DmaFB, 0 */
static Bool
NVAccelInitContextSurfaces(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *surf2d;
	uint32_t class;

	class = (pNv->Architecture >= NV_ARCH_10) ? NV10_CONTEXT_SURFACES_2D :
						    NV04_CONTEXT_SURFACES_2D;

	if (!pNv->NvContextSurfaces) {
		if (nouveau_grobj_alloc(chan, NvContextSurfaces, class,
					&pNv->NvContextSurfaces))
			return FALSE;
	}
	surf2d = pNv->NvContextSurfaces;

	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_DMA_NOTIFY, 1);
	OUT_RING  (chan, chan->nullobj->handle);
	BEGIN_RING(chan, surf2d,
		   NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RING  (chan, pNv->chan->vram->handle);
	OUT_RING  (chan, pNv->chan->vram->handle);

	return TRUE;
}

#if !defined(__AROS__)
/* FLAGS_ROP_AND, DmaFB, DmaFB, 0 */
static Bool
NVAccelInitContextBeta1(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *beta1;

	if (!pNv->NvContextBeta1) {
		if (nouveau_grobj_alloc(chan, NvContextBeta1, 0x12,
					&pNv->NvContextBeta1))
			return FALSE;
	}
	beta1 = pNv->NvContextBeta1;

	BEGIN_RING(chan, beta1, 0x300, 1); /*alpha factor*/
	OUT_RING  (chan, 0xff << 23);

	return TRUE;
}


static Bool
NVAccelInitContextBeta4(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *beta4;
	
	if (!pNv->NvContextBeta4) {
		if (nouveau_grobj_alloc(chan, NvContextBeta4, 0x72,
					&pNv->NvContextBeta4))
			return FALSE;
	}
	beta4 = pNv->NvContextBeta4;

	BEGIN_RING(chan, beta4, 0x300, 1); /*RGBA factor*/
	OUT_RING  (chan, 0xffff0000);
	return TRUE;
}
#endif

Bool
NVAccelGetCtxSurf2DFormatFromPixmap(PixmapPtr pPix, int *fmt_ret)
{
#if !defined(__AROS__)
	switch (pPix->drawable.bitsPerPixel) {
#else
    switch (pPix->bytesperpixel * 8) {
#endif
	case 32:
		*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_A8R8G8B8;
		break;
	case 24:
		*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_X8R8G8B8_Z8R8G8B8;
		break;
	case 16:
#if !defined(__AROS__)
		if (pPix->drawable.depth == 16)
#else
        if (pPix->depth == 16)
#endif
			*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;
		else
			*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_X1R5G5B5_Z1R5G5B5;
		break;
	case 8:
		*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;
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
		*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_A8R8G8B8;
		break;
	case PICT_x8r8g8b8:
		*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_X8R8G8B8_Z8R8G8B8;
		break;
	case PICT_r5g6b5:
		*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;
		break;
	case PICT_a8:
		*fmt_ret = NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;
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
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *patt;

	if (!pNv->NvImagePattern) {
		if (nouveau_grobj_alloc(chan, NvImagePattern,
					NV04_IMAGE_PATTERN,
					&pNv->NvImagePattern))
			return FALSE;
	}
	patt = pNv->NvImagePattern;

	BEGIN_RING(chan, patt, NV04_IMAGE_PATTERN_DMA_NOTIFY, 1);
	OUT_RING  (chan, chan->nullobj->handle);
	BEGIN_RING(chan, patt, NV04_IMAGE_PATTERN_MONOCHROME_FORMAT, 3);
#if X_BYTE_ORDER == X_BIG_ENDIAN
	OUT_RING  (chan, NV04_IMAGE_PATTERN_MONOCHROME_FORMAT_LE);
#else
	OUT_RING  (chan, NV04_IMAGE_PATTERN_MONOCHROME_FORMAT_CGA6);
#endif
	OUT_RING  (chan, NV04_IMAGE_PATTERN_MONOCHROME_SHAPE_8X8);
	OUT_RING  (chan, NV04_IMAGE_PATTERN_PATTERN_SELECT_MONO);

	return TRUE;
}

static Bool
NVAccelInitRasterOp(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *rop;

	if (!pNv->NvRop) {
		if (nouveau_grobj_alloc(chan, NvRop, NV03_CONTEXT_ROP,
					&pNv->NvRop))
			return FALSE;
	}
	rop = pNv->NvRop;

	BEGIN_RING(chan, rop, NV03_CONTEXT_ROP_DMA_NOTIFY, 1);
	OUT_RING  (chan, chan->nullobj->handle);

//FIXME	pNv->currentRop = ~0;
	return TRUE;
}

#if !defined(__AROS__)
static Bool
NVAccelInitRectangle(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *rect;

	if (!pNv->NvRectangle) {
		if (nouveau_grobj_alloc(chan, NvRectangle,
					NV04_GDI_RECTANGLE_TEXT,
					&pNv->NvRectangle))
			return FALSE;
	}
	rect = pNv->NvRectangle;

	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_DMA_NOTIFY, 1);
	OUT_RING  (chan, pNv->notify0->handle);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_DMA_FONTS, 1);
	OUT_RING  (chan, chan->nullobj->handle);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_SURFACE, 1);
	OUT_RING  (chan, pNv->NvContextSurfaces->handle);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_ROP, 1);
	OUT_RING  (chan, pNv->NvRop->handle);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_PATTERN, 1);
	OUT_RING  (chan, pNv->NvImagePattern->handle);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_OPERATION_ROP_AND);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT, 1);
	/* XXX why putting 1 like renouveau dump, swap the text */
#if 1 || X_BYTE_ORDER == X_BIG_ENDIAN
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_LE);
#else
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_CGA6);
#endif

	return TRUE;
}
#endif

static Bool
NVAccelInitImageBlit(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *blit;
	uint32_t class;

	class = (pNv->dev->chipset >= 0x11) ? NV12_IMAGE_BLIT : NV04_IMAGE_BLIT;

	if (!pNv->NvImageBlit) {
		if (nouveau_grobj_alloc(chan, NvImageBlit, class,
					&pNv->NvImageBlit))
			return FALSE;
	}
	blit = pNv->NvImageBlit;

	BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_DMA_NOTIFY, 1);
	OUT_RING  (chan, pNv->notify0->handle);
	BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_COLOR_KEY, 1);
	OUT_RING  (chan, chan->nullobj->handle);
	BEGIN_RING(chan, blit, NV04_IMAGE_BLIT_SURFACE, 1);
	OUT_RING  (chan, pNv->NvContextSurfaces->handle);
	BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_CLIP_RECTANGLE, 3);
	OUT_RING  (chan, chan->nullobj->handle);
	OUT_RING  (chan, pNv->NvImagePattern->handle);
	OUT_RING  (chan, pNv->NvRop->handle);
	BEGIN_RING(chan, blit, NV01_IMAGE_BLIT_OPERATION, 1);
	OUT_RING  (chan, NV01_IMAGE_BLIT_OPERATION_ROP_AND);

	if (blit->grclass == NV12_IMAGE_BLIT) {
		BEGIN_RING(chan, blit, 0x0120, 3);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, 1);
		OUT_RING  (chan, 2);
	}

	return TRUE;
}

#if !defined(__AROS__)
static Bool
NVAccelInitScaledImage(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *sifm;
	uint32_t class;

	switch (pNv->Architecture) {
	case NV_ARCH_04:
		class = NV04_SCALED_IMAGE_FROM_MEMORY;
		break;
	case NV_ARCH_10:
	case NV_ARCH_20:
	case NV_ARCH_30:
		class = NV10_SCALED_IMAGE_FROM_MEMORY;
		break;
	case NV_ARCH_40:
	default:
		class = NV10_SCALED_IMAGE_FROM_MEMORY | 0x3000;
		break;
	}

	if (!pNv->NvScaledImage) {
		if (nouveau_grobj_alloc(chan, NvScaledImage, class,
					&pNv->NvScaledImage))
			return FALSE;
	}
	sifm = pNv->NvScaledImage;

	BEGIN_RING(chan, sifm,
			 NV03_SCALED_IMAGE_FROM_MEMORY_DMA_NOTIFY, 7);
	OUT_RING  (chan, pNv->notify0->handle);
	OUT_RING  (chan, pNv->chan->vram->handle);
	OUT_RING  (chan, chan->nullobj->handle);
	OUT_RING  (chan, chan->nullobj->handle);
	OUT_RING  (chan, pNv->NvContextBeta1->handle);
	OUT_RING  (chan, pNv->NvContextBeta4->handle);
	OUT_RING  (chan, pNv->NvContextSurfaces->handle);
	if (pNv->Architecture>=NV_ARCH_10) {
	BEGIN_RING(chan, sifm,
			 NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION, 1);
	OUT_RING  (chan, NV05_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION_DITHER);
	}
	BEGIN_RING(chan, sifm, NV03_SCALED_IMAGE_FROM_MEMORY_OPERATION, 1);
	OUT_RING  (chan, NV03_SCALED_IMAGE_FROM_MEMORY_OPERATION_SRCCOPY);

	return TRUE;
}

static Bool
NVAccelInitClipRectangle(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *clip;

	if (!pNv->NvClipRectangle) {
		if (nouveau_grobj_alloc(pNv->chan, NvClipRectangle,
					NV01_CONTEXT_CLIP_RECTANGLE,
					&pNv->NvClipRectangle))
			return FALSE;
	}
	clip = pNv->NvClipRectangle;

	BEGIN_RING(chan, clip, NV01_CONTEXT_CLIP_RECTANGLE_DMA_NOTIFY, 1);
	OUT_RING  (chan, chan->nullobj->handle);

	return TRUE;
}

/* FLAGS_NONE, NvDmaFB, NvDmaAGP, NvDmaNotifier0 */
static Bool
NVAccelInitMemFormat(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *m2mf;
	uint32_t class;

	if (pNv->Architecture < NV_ARCH_50)
		class = NV04_MEMORY_TO_MEMORY_FORMAT;
	else
		class = NV50_MEMORY_TO_MEMORY_FORMAT;

	if (!pNv->NvMemFormat) {
		if (nouveau_grobj_alloc(chan, NvMemFormat, class,
					&pNv->NvMemFormat))
			return FALSE;
	}
	m2mf = pNv->NvMemFormat;

	BEGIN_RING(chan, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_NOTIFY, 1);
	OUT_RING  (chan, pNv->notify0->handle);
	BEGIN_RING(chan, m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_BUFFER_IN, 2);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);

	return TRUE;
}

static Bool
NVAccelInitImageFromCpu(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *ifc;
	uint32_t class;

	switch (pNv->Architecture) {
	case NV_ARCH_04:
		class = NV04_IMAGE_FROM_CPU;
		break;
	case NV_ARCH_10:
	case NV_ARCH_20:
	case NV_ARCH_30:
	case NV_ARCH_40:
	default:
		class = NV10_IMAGE_FROM_CPU;
		break;
	}

	if (!pNv->NvImageFromCpu) {
		if (nouveau_grobj_alloc(chan, NvImageFromCpu, class,
					&pNv->NvImageFromCpu))
			return FALSE;
	}
	ifc = pNv->NvImageFromCpu;

	BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_DMA_NOTIFY, 1);
	OUT_RING  (chan, pNv->notify0->handle);
	BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_CLIP_RECTANGLE, 1);
	OUT_RING  (chan, chan->nullobj->handle);
	BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_PATTERN, 1);
	OUT_RING  (chan, chan->nullobj->handle);
	BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_ROP, 1);
	OUT_RING  (chan, chan->nullobj->handle);
	if (pNv->Architecture >= NV_ARCH_10) {
		BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_BETA1, 1);
		OUT_RING  (chan, chan->nullobj->handle);
		BEGIN_RING(chan, ifc, NV04_IMAGE_FROM_CPU_BETA4, 1);
		OUT_RING  (chan, chan->nullobj->handle);
	}
	BEGIN_RING(chan, ifc, NV04_IMAGE_FROM_CPU_SURFACE, 1);
	OUT_RING  (chan, pNv->NvContextSurfaces->handle);
	BEGIN_RING(chan, ifc, NV01_IMAGE_FROM_CPU_OPERATION, 1);
	OUT_RING  (chan, NV01_IMAGE_FROM_CPU_OPERATION_SRCCOPY);

	return TRUE;
}
#endif

static Bool
NVAccelInit2D_NV50(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_channel *chan = pNv->chan;
	struct nouveau_grobj *eng2d;

	if (!pNv->Nv2D) {
		if (nouveau_grobj_alloc(chan, Nv2D, NV50_2D, &pNv->Nv2D))
			return FALSE;
	}
	eng2d = pNv->Nv2D;

	BEGIN_RING(chan, eng2d, NV50_2D_DMA_NOTIFY, 3);
	OUT_RING  (chan, pNv->notify0->handle);
	OUT_RING  (chan, pNv->chan->vram->handle);
	OUT_RING  (chan, pNv->chan->vram->handle);

	/* Magics from nv, no clue what they do, but at least some
	 * of them are needed to avoid crashes.
	 */
	BEGIN_RING(chan, eng2d, 0x260, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, eng2d, NV50_2D_CLIP_ENABLE, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, eng2d, NV50_2D_COLOR_KEY_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, eng2d, 0x58c, 1);
	OUT_RING  (chan, 0x111);

//FIXME	pNv->currentRop = 0xfffffffa;
	return TRUE;
}

#if !defined(__AROS__)
#define INIT_CONTEXT_OBJECT(name) do {                                        \
	ret = NVAccelInit##name(pScrn);                                       \
	if (!ret) {                                                           \
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,                         \
			   "Failed to initialise context object: "#name       \
			   " (%d)\n", ret);                                   \
		return FALSE;                                                 \
	}                                                                     \
} while(0)
#else
#define INIT_CONTEXT_OBJECT(name) do {                                      \
	ret = NVAccelInit##name(pScrn);                                         \
	if (!ret) {                                                             \
		bug("[nouveau] Failed to initialise context object: "#name          \
			   " (%d)\n", ret);                                             \
		return FALSE;                                                       \
	}                                                                       \
} while(0)
#endif

Bool
NVAccelCommonInit(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	Bool ret;

#if !defined(__AROS__)
	if (pNv->NoAccel)
		return TRUE;
#endif
	/* General engine objects */
	INIT_CONTEXT_OBJECT(DmaNotifier0);

	/* 2D engine */
	if (pNv->Architecture < NV_ARCH_50) {
		INIT_CONTEXT_OBJECT(ContextSurfaces);
//		INIT_CONTEXT_OBJECT(ContextBeta1);
//		INIT_CONTEXT_OBJECT(ContextBeta4);
		INIT_CONTEXT_OBJECT(ImagePattern);
		INIT_CONTEXT_OBJECT(RasterOp);
//		INIT_CONTEXT_OBJECT(Rectangle);
		INIT_CONTEXT_OBJECT(ImageBlit);
//		INIT_CONTEXT_OBJECT(ScaledImage);
//		INIT_CONTEXT_OBJECT(ClipRectangle);
//		INIT_CONTEXT_OBJECT(ImageFromCpu);
	} else {
		INIT_CONTEXT_OBJECT(2D_NV50);
	}
//	INIT_CONTEXT_OBJECT(MemFormat);

	/* 3D init */
//	switch (pNv->Architecture) {
//	case NV_ARCH_50:
//		INIT_CONTEXT_OBJECT(NV50TCL);
//		break;
//	case NV_ARCH_40:
//		INIT_CONTEXT_OBJECT(NV40TCL);
//		break;
//	case NV_ARCH_30:
//		INIT_CONTEXT_OBJECT(NV30TCL);
//		break;
//	case NV_ARCH_20:
//	case NV_ARCH_10:
//		INIT_CONTEXT_OBJECT(NV10TCL);
//		break;
//	default:
//		break;
//	}

	return TRUE;
}

void NVAccelFree(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);

#if !defined(__AROS__)
	if (pNv->NoAccel)
		return;
#endif

	nouveau_notifier_free(&pNv->notify0);
//	nouveau_notifier_free(&pNv->vblank_sem);

	if (pNv->Architecture < NV_ARCH_50) {
		nouveau_grobj_free(&pNv->NvContextSurfaces);
//		nouveau_grobj_free(&pNv->NvContextBeta1);
//		nouveau_grobj_free(&pNv->NvContextBeta4);
		nouveau_grobj_free(&pNv->NvImagePattern);
		nouveau_grobj_free(&pNv->NvRop);
//		nouveau_grobj_free(&pNv->NvRectangle);
		nouveau_grobj_free(&pNv->NvImageBlit);
//		nouveau_grobj_free(&pNv->NvScaledImage);
//		nouveau_grobj_free(&pNv->NvClipRectangle);
//		nouveau_grobj_free(&pNv->NvImageFromCpu);
	} else {
		nouveau_grobj_free(&pNv->Nv2D);
	}
//	nouveau_grobj_free(&pNv->NvMemFormat);

//	nouveau_grobj_free(&pNv->NvSW);
//	nouveau_grobj_free(&pNv->Nv3D);

//	nouveau_bo_ref(NULL, &pNv->tesla_scratch);
//	nouveau_bo_ref(NULL, &pNv->shader_mem);
}
