/*
 * Copyright 2009 Nouveau Project
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
#if !defined(__AROS__)
#include "exa.h"
#endif

#include "hwdefs/nv_m2mf.xml.h"

static inline Bool
NVAccelMemcpyRect(char *dst, const char *src, int height, int dst_pitch,
		  int src_pitch, int line_len)
{
	if ((src_pitch == line_len) && (src_pitch == dst_pitch)) {
		memcpy(dst, src, line_len*height);
	} else {
		while (height--) {
			memcpy(dst, src, line_len);
			src += src_pitch;
			dst += dst_pitch;
		}
	}

	return TRUE;
}

Bool
NVAccelM2MF(NVPtr pNv, int w, int h, int cpp, uint32_t srcoff, uint32_t dstoff,
	    struct nouveau_bo *src, int sd, int sp, int sh, int sx, int sy,
	    struct nouveau_bo *dst, int dd, int dp, int dh, int dx, int dy)
{
	if (pNv->ce_rect && pNv->ce_enabled)
		return pNv->ce_rect(pNv->ce_pushbuf, pNv->NvCopy, w, h, cpp,
				    src, srcoff, sd, sp, sh, sx, sy,
				    dst, dstoff, dd, dp, dh, dx, dy);
	else
	if (pNv->Architecture >= NV_KEPLER)
		return NVE0EXARectCopy(pNv, w, h, cpp,
				       src, srcoff, sd, sp, sh, sx, sy,
				       dst, dstoff, dd, dp, dh, dx, dy);
	else
	if (pNv->Architecture >= NV_FERMI)
		return NVC0EXARectM2MF(pNv, w, h, cpp,
				       src, srcoff, sd, sp, sh, sx, sy,
				       dst, dstoff, dd, dp, dh, dx, dy);
	else
	if (pNv->Architecture >= NV_TESLA)
		return NV50EXARectM2MF(pNv, w, h, cpp,
				       src, srcoff, sd, sp, sh, sx, sy,
				       dst, dstoff, dd, dp, dh, dx, dy);
	else
		return NV04EXARectM2MF(pNv, w, h, cpp,
				       src, srcoff, sd, sp, sh, sx, sy,
				       dst, dstoff, dd, dp, dh, dx, dy);
	return FALSE;
}

#if !defined(__AROS__)
static int
nouveau_exa_mark_sync(ScreenPtr pScreen)
{
	return 0;
}

static void
nouveau_exa_wait_marker(ScreenPtr pScreen, int marker)
{
}

static Bool
nouveau_exa_prepare_access(PixmapPtr ppix, int index)
{
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
	NVPtr pNv = NVPTR(xf86ScreenToScrn(ppix->drawable.pScreen));

	if (nv50_style_tiled_pixmap(ppix) && !pNv->wfb_enabled)
		return FALSE;
	if (nouveau_bo_map(bo, NOUVEAU_BO_RDWR, pNv->client))
		return FALSE;
	ppix->devPrivate.ptr = bo->map;
	return TRUE;
}

static void
nouveau_exa_finish_access(PixmapPtr ppix, int index)
{
}

static Bool
nouveau_exa_pixmap_is_offscreen(PixmapPtr ppix)
{
	return nouveau_pixmap_bo(ppix) != NULL;
}

static void *
nouveau_exa_create_pixmap(ScreenPtr pScreen, int width, int height, int depth,
			  int usage_hint, int bitsPerPixel, int *new_pitch)
{
	ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
	NVPtr pNv = NVPTR(scrn);
	struct nouveau_pixmap *nvpix;
	int ret;

	if (!width || !height)
		return calloc(1, sizeof(*nvpix));

	if (!pNv->exa_force_cp && pNv->dev->vram_size <= 32 * 1024 * 1024)
		return NULL;

	nvpix = calloc(1, sizeof(*nvpix));
	if (!nvpix)
		return NULL;

	ret = nouveau_allocate_surface(scrn, width, height, bitsPerPixel,
				       usage_hint, new_pitch, &nvpix->bo);
	if (!ret) {
		free(nvpix);
		return NULL;
	}

#ifdef NOUVEAU_PIXMAP_SHARING
	if ((usage_hint & 0xffff) == CREATE_PIXMAP_USAGE_SHARED)
		nvpix->shared = TRUE;
#endif

	return nvpix;
}

static void
nouveau_exa_destroy_pixmap(ScreenPtr pScreen, void *priv)
{
	struct nouveau_pixmap *nvpix = priv;

	if (!nvpix)
		return;

	nouveau_bo_ref(NULL, &nvpix->bo);
	free(nvpix);
}
#endif

#ifdef NOUVEAU_PIXMAP_SHARING
static Bool
nouveau_exa_share_pixmap_backing(PixmapPtr ppix, ScreenPtr slave, void **handle_p)
{
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
	struct nouveau_pixmap *nvpix = nouveau_pixmap(ppix);
	int ret;
	int handle;

	ret = nouveau_bo_set_prime(bo, &handle);
	if (ret != 0) {
		ErrorF("%s: ret is %d errno is %d\n", __func__, ret, errno);
		return FALSE;
	}
	nvpix->shared = TRUE;
	*handle_p = (void *)(long)handle;
	return TRUE;
}

static Bool
nouveau_exa_set_shared_pixmap_backing(PixmapPtr ppix, void *handle)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(ppix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_bo *bo = nouveau_pixmap_bo(ppix);
	struct nouveau_pixmap *nvpix = nouveau_pixmap(ppix);
	int ret;
	int ihandle = (int)(long)(handle);

	ret = nouveau_bo_prime_handle_ref(pNv->dev, ihandle, &bo);
	if (ret) {
		ErrorF("failed to get BO with handle %d\n", ihandle);
		return FALSE;
	}
	nvpix->bo = bo;
	nvpix->shared = TRUE;
	close(ihandle);
	return TRUE;
}
#endif

Bool
nv50_style_tiled_pixmap(PixmapPtr ppix)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(ppix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);

	return pNv->Architecture >= NV_TESLA &&
	       nouveau_pixmap_bo(ppix)->config.nv50.memtype;
}

#if !defined(__AROS__)
static int
nouveau_exa_scratch(NVPtr pNv, int size, struct nouveau_bo **pbo, int *off)
{
	struct nouveau_bo *bo;
	int ret;

	if (!pNv->transfer ||
	     pNv->transfer->size <= pNv->transfer_offset + size) {
		ret = nouveau_bo_new(pNv->dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
				     0, NOUVEAU_ALIGN(size, 1 * 1024 * 1024),
				     NULL, &bo);
		if (ret != 0)
			return ret;

		ret = nouveau_bo_map(bo, NOUVEAU_BO_RDWR, pNv->client);
		if (ret != 0) {
			nouveau_bo_ref(NULL, &bo);
			return ret;
		}

		nouveau_bo_ref(bo, &pNv->transfer);
		nouveau_bo_ref(NULL, &bo);
		pNv->transfer_offset = 0;
	}

	*off = pNv->transfer_offset;
	*pbo = pNv->transfer;

	pNv->transfer_offset += size;
	return 0;
}

static Bool
nouveau_exa_download_from_screen(PixmapPtr pspix, int x, int y, int w, int h,
				 char *dst, int dst_pitch)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pspix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	struct nouveau_bo *bo;
	int src_pitch, tmp_pitch, cpp, i;
	const char *src;
	Bool ret;

	cpp = pspix->drawable.bitsPerPixel >> 3;
	src_pitch  = exaGetPixmapPitch(pspix);
	tmp_pitch = w * cpp;

	while (h) {
		const int lines = (h > 2047) ? 2047 : h;
		struct nouveau_bo *tmp;
		int tmp_offset;

		if (nouveau_exa_scratch(pNv, lines * tmp_pitch,
					&tmp, &tmp_offset))
			goto memcpy;

		if (!NVAccelM2MF(pNv, w, lines, cpp, 0, tmp_offset,
				 nouveau_pixmap_bo(pspix), NOUVEAU_BO_VRAM,
				 src_pitch, pspix->drawable.height, x, y,
				 tmp, NOUVEAU_BO_GART, tmp_pitch,
				 lines, 0, 0))
			goto memcpy;

		nouveau_bo_wait(tmp, NOUVEAU_BO_RD, pNv->client);
		if (dst_pitch == tmp_pitch) {
			memcpy(dst, tmp->map + tmp_offset, dst_pitch * lines);
			dst += dst_pitch * lines;
		} else {
			src = tmp->map + tmp_offset;
			for (i = 0; i < lines; i++) {
				memcpy(dst, src, tmp_pitch);
				src += tmp_pitch;
				dst += dst_pitch;
			}
		}

		/* next! */
		h -= lines;
		y += lines;
	}
	return TRUE;

memcpy:
	bo = nouveau_pixmap_bo(pspix);
	if (nv50_style_tiled_pixmap(pspix))
		ErrorF("%s:%d - falling back to memcpy ignores tiling\n",
		       __func__, __LINE__);

	if (nouveau_bo_map(bo, NOUVEAU_BO_RD, pNv->client))
		return FALSE;
	src = (char *)bo->map + (y * src_pitch) + (x * cpp);
	ret = NVAccelMemcpyRect(dst, src, h, dst_pitch, src_pitch, w*cpp);
	return ret;
}

static Bool
nouveau_exa_upload_to_screen(PixmapPtr pdpix, int x, int y, int w, int h,
			     char *src, int src_pitch)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pdpix->drawable.pScreen);
	NVPtr pNv = NVPTR(pScrn);
	int dst_pitch, tmp_pitch, cpp, i;
	struct nouveau_bo *bo;
	char *dst;
	Bool ret;

	cpp = pdpix->drawable.bitsPerPixel >> 3;
	dst_pitch  = exaGetPixmapPitch(pdpix);
	tmp_pitch = w * cpp;

	/* try hostdata transfer */
	if (w * h * cpp < 16*1024) /* heuristic */
	{
		if (pNv->Architecture < NV_TESLA) {
			if (NV04EXAUploadIFC(pScrn, src, src_pitch, pdpix,
					     x, y, w, h, cpp)) {
				return TRUE;
			}
		} else
		if (pNv->Architecture < NV_FERMI) {
			if (NV50EXAUploadSIFC(src, src_pitch, pdpix,
					      x, y, w, h, cpp)) {
				return TRUE;
			}
		} else {
			if (NVC0EXAUploadSIFC(src, src_pitch, pdpix,
					      x, y, w, h, cpp)) {
				return TRUE;
			}
		}
	}

	while (h) {
		const int lines = (h > 2047) ? 2047 : h;
		struct nouveau_bo *tmp;
		int tmp_offset;

		if (nouveau_exa_scratch(pNv, lines * tmp_pitch,
					&tmp, &tmp_offset))
			goto memcpy;

		if (src_pitch == tmp_pitch) {
			memcpy(tmp->map + tmp_offset, src, src_pitch * lines);
			src += src_pitch * lines;
		} else {
			dst = tmp->map + tmp_offset;
			for (i = 0; i < lines; i++) {
				memcpy(dst, src, tmp_pitch);
				src += src_pitch;
				dst += tmp_pitch;
			}
		}

		if (!NVAccelM2MF(pNv, w, lines, cpp, tmp_offset, 0, tmp,
				 NOUVEAU_BO_GART, tmp_pitch, lines, 0, 0,
				 nouveau_pixmap_bo(pdpix), NOUVEAU_BO_VRAM,
				 dst_pitch, pdpix->drawable.height, x, y))
			goto memcpy;

		/* next! */
		h -= lines;
		y += lines;
	}

	return TRUE;

	/* fallback to memcpy-based transfer */
memcpy:
	bo = nouveau_pixmap_bo(pdpix);
	if (nv50_style_tiled_pixmap(pdpix))
		ErrorF("%s:%d - falling back to memcpy ignores tiling\n",
		       __func__, __LINE__);

	if (nouveau_bo_map(bo, NOUVEAU_BO_WR, pNv->client))
		return FALSE;
	dst = (char *)bo->map + (y * dst_pitch) + (x * cpp);
	ret = NVAccelMemcpyRect(dst, src, h, dst_pitch, src_pitch, w*cpp);
	return ret;
}

Bool
nouveau_exa_pixmap_is_onscreen(PixmapPtr ppix)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(ppix->drawable.pScreen);

	if (pScrn->pScreen->GetScreenPixmap(pScrn->pScreen) == ppix)
		return TRUE;

	return FALSE;
}

static void
nouveau_exa_flush(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	nouveau_pushbuf_kick(pNv->pushbuf, pNv->pushbuf->channel);
}

Bool
nouveau_exa_init(ScreenPtr pScreen) 
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	NVPtr pNv = NVPTR(pScrn);
	ExaDriverPtr exa;

	if (!xf86LoadSubModule(pScrn, "exa"))
		return FALSE;

	exa = exaDriverAlloc();
	if (!exa)
		return FALSE;

	exa->exa_major = EXA_VERSION_MAJOR;
	exa->exa_minor = EXA_VERSION_MINOR;
	exa->flags = EXA_OFFSCREEN_PIXMAPS;

#ifdef EXA_SUPPORTS_PREPARE_AUX
	exa->flags |= EXA_SUPPORTS_PREPARE_AUX;
#endif

	exa->PixmapIsOffscreen = nouveau_exa_pixmap_is_offscreen;
	exa->PrepareAccess = nouveau_exa_prepare_access;
	exa->FinishAccess = nouveau_exa_finish_access;

	exa->flags |= (EXA_HANDLES_PIXMAPS | EXA_MIXED_PIXMAPS);
	exa->pixmapOffsetAlign = 256;
	exa->pixmapPitchAlign = 64;

	exa->CreatePixmap2 = nouveau_exa_create_pixmap;
	exa->DestroyPixmap = nouveau_exa_destroy_pixmap;
#ifdef NOUVEAU_PIXMAP_SHARING
	exa->SharePixmapBacking = nouveau_exa_share_pixmap_backing;
	exa->SetSharedPixmapBacking = nouveau_exa_set_shared_pixmap_backing;
#endif

	if (pNv->Architecture >= NV_TESLA) {
		exa->maxX = 8192;
		exa->maxY = 8192;
	} else
	if (pNv->Architecture >= NV_ARCH_10) {
		exa->maxX = 4096;
		exa->maxY = 4096;
	} else {
		exa->maxX = 2048;
		exa->maxY = 2048;
	}

	exa->MarkSync = nouveau_exa_mark_sync;
	exa->WaitMarker = nouveau_exa_wait_marker;

	exa->DownloadFromScreen = nouveau_exa_download_from_screen;
	exa->UploadToScreen = nouveau_exa_upload_to_screen;

	if (pNv->Architecture < NV_TESLA) {
		exa->PrepareCopy = NV04EXAPrepareCopy;
		exa->Copy = NV04EXACopy;
		exa->DoneCopy = NV04EXADoneCopy;

		exa->PrepareSolid = NV04EXAPrepareSolid;
		exa->Solid = NV04EXASolid;
		exa->DoneSolid = NV04EXADoneSolid;
	} else
	if (pNv->Architecture < NV_FERMI) {
		exa->PrepareCopy = NV50EXAPrepareCopy;
		exa->Copy = NV50EXACopy;
		exa->DoneCopy = NV50EXADoneCopy;

		exa->PrepareSolid = NV50EXAPrepareSolid;
		exa->Solid = NV50EXASolid;
		exa->DoneSolid = NV50EXADoneSolid;
	} else {
		exa->PrepareCopy = NVC0EXAPrepareCopy;
		exa->Copy        = NVC0EXACopy;
		exa->DoneCopy    = NVC0EXADoneCopy;

		exa->PrepareSolid = NVC0EXAPrepareSolid;
		exa->Solid        = NVC0EXASolid;
		exa->DoneSolid    = NVC0EXADoneSolid;
	}

	switch (pNv->Architecture) {	
	case NV_ARCH_10:
	case NV_ARCH_20:
 		exa->CheckComposite   = NV10EXACheckComposite;
 		exa->PrepareComposite = NV10EXAPrepareComposite;
 		exa->Composite        = NV10EXAComposite;
 		exa->DoneComposite    = NV10EXADoneComposite;
		break;
	case NV_ARCH_30:
		exa->CheckComposite   = NV30EXACheckComposite;
		exa->PrepareComposite = NV30EXAPrepareComposite;
		exa->Composite        = NV30EXAComposite;
		exa->DoneComposite    = NV30EXADoneComposite;
		break;
	case NV_ARCH_40:
		exa->CheckComposite   = NV40EXACheckComposite;
		exa->PrepareComposite = NV40EXAPrepareComposite;
		exa->Composite        = NV40EXAComposite;
		exa->DoneComposite    = NV40EXADoneComposite;
		break;
	case NV_TESLA:
		exa->CheckComposite   = NV50EXACheckComposite;
		exa->PrepareComposite = NV50EXAPrepareComposite;
		exa->Composite        = NV50EXAComposite;
		exa->DoneComposite    = NV50EXADoneComposite;
		break;
	case NV_FERMI:
	case NV_KEPLER:
	case NV_MAXWELL:
	case NV_PASCAL:
		exa->CheckComposite   = NVC0EXACheckComposite;
		exa->PrepareComposite = NVC0EXAPrepareComposite;
		exa->Composite        = NVC0EXAComposite;
		exa->DoneComposite    = NVC0EXADoneComposite;
		break;
	default:
		break;
	}

	if (!exaDriverInit(pScreen, exa))
		return FALSE;

	pNv->EXADriverPtr = exa;
	pNv->Flush = nouveau_exa_flush;
	return TRUE;
}
#endif

/* AROS CODE */

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes lock on GART object is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HiddNouveauNVAccelUploadM2MF(
    UBYTE * srcpixels, ULONG srcpitch, HIDDT_StdPixFmt srcPixFmt,
    LONG x, LONG y, LONG width, LONG height,
    OOP_Class *cl, OOP_Object *o)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    PixmapPtr pdpix = bmdata;
    ScrnInfoPtr pScrn = carddata;
    NVPtr pNv = carddata;

    int dst_pitch, tmp_pitch, cpp;
    char *dst;
    Bool ret;

    cpp = pdpix->drawable.depth > 16 ? 4 : 2;
    dst_pitch  = exaGetPixmapPitch(pdpix);
    tmp_pitch = width * cpp;

    /* try hostdata transfer */
    if (srcPixFmt == vHidd_StdPixFmt_Native || (srcPixFmt == vHidd_StdPixFmt_Native32 && cpp == 4))
    if (width * height * cpp < 16*1024) /* heuristic */
    {
        if (pNv->Architecture < NV_TESLA) {
            if (NV04EXAUploadIFC(pScrn, srcpixels, srcpitch, pdpix,
                            x, y, width, height, cpp)) {
                return TRUE;
            }
        } else
        if (pNv->Architecture < NV_FERMI) {
            if (NV50EXAUploadSIFC(srcpixels, srcpitch, pdpix,
                            x, y, width, height, cpp)) {
                return TRUE;
            }
        } else {
            if (NVC0EXAUploadSIFC(srcpixels, srcpitch, pdpix,
                            x, y, width, height, cpp)) {
                return TRUE;
            }
        }
    }

    while (height) {
        const int lines = (height > 2047) ? 2047 : height;
        int tmp_offset = 0;

        /* RAM -> CPU -> GART */
        nouveau_bo_wait(pNv->GART, NOUVEAU_BO_WR, pNv->client);

        if (nouveau_bo_map(pNv->GART, NOUVEAU_BO_WR, pNv->client))
            return FALSE;
        dst = pNv->GART->map;

        HiddNouveauWriteFromRAM( (APTR)srcpixels, srcpitch, srcPixFmt, dst, tmp_pitch,
            width, lines, cl, o);
        srcpixels += srcpitch * lines;

        /* GART -> GPU -> VRAM */
        if (!NVAccelM2MF(pNv, width, lines, cpp, tmp_offset, 0, pNv->GART,
                    NOUVEAU_BO_GART, tmp_pitch, lines, 0, 0,
                    nouveau_pixmap_bo(pdpix), NOUVEAU_BO_VRAM,
                    dst_pitch, pdpix->drawable.height, x, y))
            return FALSE;

        /* next! */
        height -= lines;
        y += lines;
    }

    return TRUE;
}

/* NOTE: Assumes lock on bitmap is already made */
/* NOTE: Assumes lock on GART object is already made */
/* NOTE: Assumes buffer is not mapped */
BOOL HiddNouveauNVAccelDownloadM2MF(
    UBYTE * dstpixels, ULONG dstpitch, HIDDT_StdPixFmt dstPixFmt,
    LONG x, LONG y, LONG width, LONG height,
    OOP_Class *cl, OOP_Object *o)
{
    struct HIDDNouveauBitMapData *bmdata = OOP_INST_DATA(cl, o);
    struct CardData *carddata = &(SD(cl)->carddata);

    PixmapPtr pspix = bmdata;
    NVPtr pNv = carddata;

    int src_pitch, tmp_pitch, cpp;
    char *src;
    Bool ret;

    cpp = pspix->drawable.depth > 16 ? 4 : 2;
    src_pitch  = exaGetPixmapPitch(pspix);
    tmp_pitch = width * cpp;

    while (height) {
        const int lines = (height > 2047) ? 2047 : height;
        int tmp_offset = 0;

        /* VRAM -> GPU -> GART */
        if (!NVAccelM2MF(pNv, width, lines, cpp, 0, tmp_offset,
                    nouveau_pixmap_bo(pspix), NOUVEAU_BO_VRAM,
                    src_pitch, pspix->drawable.height, x, y,
                    pNv->GART, NOUVEAU_BO_GART, tmp_pitch,
                    lines, 0, 0))
            return FALSE;

        nouveau_bo_wait(pNv->GART, NOUVEAU_BO_RD, pNv->client);

        /* GART -> CPU -> RAM */
        if (nouveau_bo_map(pNv->GART, NOUVEAU_BO_RD, pNv->client))
            return FALSE;
        src = pNv->GART->map;

        HiddNouveauReadIntoRAM(src, tmp_pitch, dstpixels, dstpitch,
            dstPixFmt, width, lines, cl, o);
        dstpixels += dstpitch * lines;

        /* next! */
        height -= lines;
        y += lines;
    }

    return TRUE;
}