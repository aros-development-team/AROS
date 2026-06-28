/*
 * Copyright 2007 Arthur Huillet
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __SSE2__
#include <immintrin.h>
#endif

#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#include "exa.h"
#include "damage.h"
#include "dixstruct.h"
#include "fourcc.h"

#include "nv_include.h"
#include "nv_dma.h"

#include "vl_hwmc.h"
#else
#include "nv_include.h"
typedef signed char int8_t;
#endif

#include "hwdefs/nv_m2mf.xml.h"

#define IMAGE_MAX_W 2046
#define IMAGE_MAX_H 2046

#define TEX_IMAGE_MAX_W 4096
#define TEX_IMAGE_MAX_H 4096

#define OFF_DELAY 	500  /* milliseconds */
#define FREE_DELAY 	5000

#define NUM_BLIT_PORTS 16
#define NUM_TEXTURE_PORTS 32


#define NVStopOverlay(X) (((pNv->Architecture == NV_ARCH_04) ? NV04StopOverlay(X) : NV10StopOverlay(X)))

/* Value taken by pPriv -> currentHostBuffer when we failed to allocate the two private buffers in TT memory, so that we can catch this case
and attempt no other allocation afterwards (performance reasons) */
#define NO_PRIV_HOST_BUFFER_AVAILABLE 9999

/* NVPutImage action flags */
enum {
	IS_YV12 = 1,
	IS_YUY2 = 2,
	CONVERT_TO_YUY2=4,
	USE_OVERLAY=8,
	USE_TEXTURE=16,
	SWAP_UV=32,
	IS_RGB=64, //I am not sure how long we will support it
};

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

#if !defined(__AROS__)
Atom xvBrightness, xvContrast, xvColorKey, xvSaturation;
Atom xvHue, xvAutopaintColorKey, xvSetDefaults, xvDoubleBuffer;
Atom xvITURBT709, xvSyncToVBlank, xvOnCRTCNb;

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding =
{
	0,
	"XV_IMAGE",
	IMAGE_MAX_W, IMAGE_MAX_H,
	{1, 1}
};

static XF86VideoEncodingRec DummyEncodingTex =
{
	0,
	"XV_IMAGE",
	TEX_IMAGE_MAX_W, TEX_IMAGE_MAX_H,
	{1, 1}
};

static XF86VideoEncodingRec DummyEncodingNV50 =
{
	0,
	"XV_IMAGE",
	8192, 8192,
	{1, 1}
};

#define NUM_FORMATS_ALL 6

XF86VideoFormatRec NVFormats[NUM_FORMATS_ALL] =
{
	{15, TrueColor}, {16, TrueColor}, {24, TrueColor},
	{15, DirectColor}, {16, DirectColor}, {24, DirectColor}
};

#define NUM_FORMATS_NV50 8
XF86VideoFormatRec NV50Formats[NUM_FORMATS_NV50] =
{
	{15, TrueColor}, {16, TrueColor}, {24, TrueColor}, {30, TrueColor},
	{15, DirectColor}, {16, DirectColor}, {24, DirectColor}, {30, DirectColor}
};

#define NUM_NV04_OVERLAY_ATTRIBUTES 4
XF86AttributeRec NV04OverlayAttributes[NUM_NV04_OVERLAY_ATTRIBUTES] =
{
	    {XvSettable | XvGettable, -512, 511, "XV_BRIGHTNESS"},
	    {XvSettable | XvGettable, 0, (1 << 24) - 1, "XV_COLORKEY"},
	    {XvSettable | XvGettable, 0, 1, "XV_AUTOPAINT_COLORKEY"},
	    {XvSettable             , 0, 0, "XV_SET_DEFAULTS"},
};


#define NUM_NV10_OVERLAY_ATTRIBUTES 10
XF86AttributeRec NV10OverlayAttributes[NUM_NV10_OVERLAY_ATTRIBUTES] =
{
	{XvSettable | XvGettable, 0, 1, "XV_DOUBLE_BUFFER"},
	{XvSettable | XvGettable, 0, (1 << 24) - 1, "XV_COLORKEY"},
	{XvSettable | XvGettable, 0, 1, "XV_AUTOPAINT_COLORKEY"},
	{XvSettable             , 0, 0, "XV_SET_DEFAULTS"},
	{XvSettable | XvGettable, -512, 511, "XV_BRIGHTNESS"},
	{XvSettable | XvGettable, 0, 8191, "XV_CONTRAST"},
	{XvSettable | XvGettable, 0, 8191, "XV_SATURATION"},
	{XvSettable | XvGettable, 0, 360, "XV_HUE"},
	{XvSettable | XvGettable, 0, 1, "XV_ITURBT_709"},
	{XvSettable | XvGettable, 0, 1, "XV_ON_CRTC_NB"},
};

#define NUM_BLIT_ATTRIBUTES 2
XF86AttributeRec NVBlitAttributes[NUM_BLIT_ATTRIBUTES] =
{
	{XvSettable             , 0, 0, "XV_SET_DEFAULTS"},
	{XvSettable | XvGettable, 0, 1, "XV_SYNC_TO_VBLANK"}
};

#define NUM_TEXTURED_ATTRIBUTES 2
XF86AttributeRec NVTexturedAttributes[NUM_TEXTURED_ATTRIBUTES] =
{
	{XvSettable             , 0, 0, "XV_SET_DEFAULTS"},
	{XvSettable | XvGettable, 0, 1, "XV_SYNC_TO_VBLANK"}
};

#define NUM_TEXTURED_ATTRIBUTES_NV50 7
XF86AttributeRec NVTexturedAttributesNV50[NUM_TEXTURED_ATTRIBUTES_NV50] =
{
	{ XvSettable             , 0, 0, "XV_SET_DEFAULTS" },
	{ XvSettable | XvGettable, 0, 1, "XV_SYNC_TO_VBLANK" },
	{ XvSettable | XvGettable, -1000, 1000, "XV_BRIGHTNESS" },
	{ XvSettable | XvGettable, -1000, 1000, "XV_CONTRAST" },
	{ XvSettable | XvGettable, -1000, 1000, "XV_SATURATION" },
	{ XvSettable | XvGettable, -1000, 1000, "XV_HUE" },
	{ XvSettable | XvGettable, 0, 1, "XV_ITURBT_709" }
};

#define NUM_IMAGES_YUV 4
#define NUM_IMAGES_ALL 5

#define FOURCC_RGB 0x0000003
#define XVIMAGE_RGB \
   { \
        FOURCC_RGB, \
        XvRGB, \
        LSBFirst, \
        { 0x03, 0x00, 0x00, 0x00, \
          0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
        32, \
        XvPacked, \
        1, \
        24, 0x00ff0000, 0x0000ff00, 0x000000ff, \
        0, 0, 0, \
        0, 0, 0, \
        0, 0, 0, \
        {'B','G','R','X',\
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
        XvTopToBottom \
   }

static XF86ImageRec NVImages[NUM_IMAGES_ALL] =
{
	XVIMAGE_YUY2,
	XVIMAGE_YV12,
	XVIMAGE_UYVY,
	XVIMAGE_I420,
	XVIMAGE_RGB
};

static void
nouveau_box_intersect(BoxPtr dest, BoxPtr a, BoxPtr b)
{
    dest->x1 = a->x1 > b->x1 ? a->x1 : b->x1;
    dest->x2 = a->x2 < b->x2 ? a->x2 : b->x2;
    dest->y1 = a->y1 > b->y1 ? a->y1 : b->y1;
    dest->y2 = a->y2 < b->y2 ? a->y2 : b->y2;

    if (dest->x1 >= dest->x2 || dest->y1 >= dest->y2)
	dest->x1 = dest->x2 = dest->y1 = dest->y2 = 0;
}

static void
nouveau_crtc_box(xf86CrtcPtr crtc, BoxPtr crtc_box)
{
    if (crtc->enabled) {
	crtc_box->x1 = crtc->x;
	crtc_box->x2 = crtc->x + xf86ModeWidth(&crtc->mode, crtc->rotation);
	crtc_box->y1 = crtc->y;
	crtc_box->y2 = crtc->y + xf86ModeHeight(&crtc->mode, crtc->rotation);
    } else
	crtc_box->x1 = crtc_box->x2 = crtc_box->y1 = crtc_box->y2 = 0;
}

static int
nouveau_box_area(BoxPtr box)
{
    return (int) (box->x2 - box->x1) * (int) (box->y2 - box->y1);
}

xf86CrtcPtr
nouveau_pick_best_crtc(ScrnInfoPtr pScrn, Bool consider_disabled,
                       int x, int y, int w, int h)
{
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int                 coverage, best_coverage, c;
    BoxRec              box, crtc_box, cover_box;
    RROutputPtr         primary_output = NULL;
    xf86CrtcPtr         best_crtc = NULL, primary_crtc = NULL;

    if (!pScrn->vtSema)
	return NULL;

    box.x1 = x;
    box.x2 = x + w;
    box.y1 = y;
    box.y2 = y + h;
    best_coverage = 0;

    /* Prefer the CRTC of the primary output */
#ifdef HAS_DIXREGISTERPRIVATEKEY
    if (dixPrivateKeyRegistered(rrPrivKey))
#endif
    {
	primary_output = RRFirstOutput(pScrn->pScreen);
    }
    if (primary_output && primary_output->crtc)
	primary_crtc = primary_output->crtc->devPrivate;

    /* first consider only enabled CRTCs */
    for (c = 0; c < xf86_config->num_crtc; c++) {
	xf86CrtcPtr crtc = xf86_config->crtc[c];

	if (!crtc->enabled)
	    continue;

	nouveau_crtc_box(crtc, &crtc_box);
	nouveau_box_intersect(&cover_box, &crtc_box, &box);
	coverage = nouveau_box_area(&cover_box);
	if (coverage > best_coverage ||
	    (coverage == best_coverage && crtc == primary_crtc)) {
	    best_crtc = crtc;
	    best_coverage = coverage;
	}
    }
    if (best_crtc || !consider_disabled)
	return best_crtc;

    /* if we found nothing, repeat the search including disabled CRTCs */
    for (c = 0; c < xf86_config->num_crtc; c++) {
	xf86CrtcPtr crtc = xf86_config->crtc[c];

	nouveau_crtc_box(crtc, &crtc_box);
	nouveau_box_intersect(&cover_box, &crtc_box, &box);
	coverage = nouveau_box_area(&cover_box);
	if (coverage > best_coverage ||
	    (coverage == best_coverage && crtc == primary_crtc)) {
	    best_crtc = crtc;
	    best_coverage = coverage;
	}
    }
    return best_crtc;
}

unsigned int
nv_window_belongs_to_crtc(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
	unsigned int mask = 0;
	int i;

	for (i = 0; i < xf86_config->num_crtc; i++) {
		xf86CrtcPtr crtc = xf86_config->crtc[i];

		if (!drmmode_crtc_on(crtc))
			continue;

		if ((x < (crtc->x + crtc->mode.HDisplay)) &&
		    (y < (crtc->y + crtc->mode.VDisplay)) &&
		    ((x + w) > crtc->x) &&
		    ((y + h) > crtc->y))
		    mask |= 1 << i;
	}

	return mask;
}

/**
 * NVSetPortDefaults
 * set attributes of port "pPriv" to compiled-in (except for colorKey) defaults
 * this function does not care about the kind of adapter the port is for
 *
 * @param pScrn screen to get the default colorKey from
 * @param pPriv port to reset to defaults
 */
void
NVSetPortDefaults (ScrnInfoPtr pScrn, NVPortPrivPtr pPriv)
{
	NVPtr pNv = NVPTR(pScrn);

	pPriv->brightness		= 0;
	pPriv->contrast			= 4096;
	pPriv->saturation		= 4096;
	pPriv->hue			= 0;
	pPriv->colorKey			= pNv->videoKey;
	pPriv->autopaintColorKey	= TRUE;
	pPriv->doubleBuffer		= pNv->Architecture != NV_ARCH_04;
	pPriv->iturbt_709		= FALSE;
	pPriv->currentHostBuffer	= 0;
}

static int
nouveau_xv_bo_realloc(ScrnInfoPtr pScrn, unsigned flags, unsigned size,
		      struct nouveau_bo **pbo)
{
	union nouveau_bo_config config = {};
	NVPtr pNv = NVPTR(pScrn);

	if (*pbo) {
		if ((*pbo)->size >= size)
			return 0;
		nouveau_bo_ref(NULL, pbo);
	}

	if (flags & NOUVEAU_BO_VRAM) {
		if (pNv->Architecture == NV_TESLA)
			config.nv50.memtype = 0x70;
		else
		if (pNv->Architecture >= NV_FERMI)
			config.nvc0.memtype = 0xfe;
	}
	flags |= NOUVEAU_BO_MAP;

	return nouveau_bo_new(pNv->dev, flags, 0, size, &config, pbo);
}

/**
 * NVFreePortMemory
 * frees memory held by a given port
 *
 * @param pScrn screen whose port wants to free memory
 * @param pPriv port to free memory of
 */
static void
NVFreePortMemory(ScrnInfoPtr pScrn, NVPortPrivPtr pPriv)
{
	nouveau_bo_ref(NULL, &pPriv->video_mem);
	nouveau_bo_ref(NULL, &pPriv->TT_mem_chunk[0]);
	nouveau_bo_ref(NULL, &pPriv->TT_mem_chunk[1]);
}

/**
 * NVFreeOverlayMemory
 * frees memory held by the overlay port
 *
 * @param pScrn screen whose overlay port wants to free memory
 */
static void
NVFreeOverlayMemory(ScrnInfoPtr pScrn)
{
	NVPtr	pNv = NVPTR(pScrn);
	NVPortPrivPtr pPriv = GET_OVERLAY_PRIVATE(pNv);

	NVFreePortMemory(pScrn, pPriv);
#if NVOVL_SUPPORT
	/* "power cycle" the overlay */
	nvWriteMC(pNv, NV_PMC_ENABLE,
		  (nvReadMC(pNv, NV_PMC_ENABLE) & 0xEFFFFFFF));
	nvWriteMC(pNv, NV_PMC_ENABLE,
		  (nvReadMC(pNv, NV_PMC_ENABLE) | 0x10000000));
#endif
}

/**
 * NVFreeBlitMemory
 * frees memory held by the blit port
 *
 * @param pScrn screen whose blit port wants to free memory
 */
static void
NVFreeBlitMemory(ScrnInfoPtr pScrn)
{
	NVPtr	pNv = NVPTR(pScrn);
	NVPortPrivPtr pPriv = GET_BLIT_PRIVATE(pNv);

	NVFreePortMemory(pScrn, pPriv);
}

/**
 * NVVideoTimerCallback
 * callback function which perform cleanup tasks (stop overlay, free memory).
 * within the driver
 * purpose and use is unknown
 */
void
NVVideoTimerCallback(ScrnInfoPtr pScrn, Time currentTime)
{
	NVPtr         pNv = NVPTR(pScrn);
	NVPortPrivPtr pOverPriv = NULL;
	NVPortPrivPtr pBlitPriv = NULL;
	Bool needCallback = FALSE;

	if (!pScrn->vtSema)
		return;

	if (pNv->overlayAdaptor) {
		pOverPriv = GET_OVERLAY_PRIVATE(pNv);
		if (!pOverPriv->videoStatus)
			pOverPriv = NULL;
	}

	if (pNv->blitAdaptor) {
		pBlitPriv = GET_BLIT_PRIVATE(pNv);
		if (!pBlitPriv->videoStatus)
			pBlitPriv = NULL;
	}

	if (pOverPriv) {
		if (pOverPriv->videoTime < currentTime) {
			if (pOverPriv->videoStatus & OFF_TIMER) {
				NVStopOverlay(pScrn);
				pOverPriv->videoStatus = FREE_TIMER;
				pOverPriv->videoTime = currentTime + FREE_DELAY;
				needCallback = TRUE;
			} else
			if (pOverPriv->videoStatus & FREE_TIMER) {
				NVFreeOverlayMemory(pScrn);
				pOverPriv->videoStatus = 0;
			}
		} else {
			needCallback = TRUE;
		}
	}

	if (pBlitPriv) {
		if (pBlitPriv->videoTime < currentTime) {
			NVFreeBlitMemory(pScrn);
			pBlitPriv->videoStatus = 0;
		} else {
			needCallback = TRUE;
		}
	}

	pNv->VideoTimerCallback = needCallback ? NVVideoTimerCallback : NULL;
}

#ifndef ExaOffscreenMarkUsed
extern void ExaOffscreenMarkUsed(PixmapPtr);
#endif

/*
 * StopVideo
 */
static void
NVStopOverlayVideo(ScrnInfoPtr pScrn, pointer data, Bool Exit)
{
	NVPtr         pNv   = NVPTR(pScrn);
	NVPortPrivPtr pPriv = (NVPortPrivPtr)data;

	if (pPriv->grabbedByV4L)
		return;

	REGION_EMPTY(pScrn->pScreen, &pPriv->clip);

	if(Exit) {
		if (pPriv->videoStatus & CLIENT_VIDEO_ON)
			NVStopOverlay(pScrn);
		NVFreeOverlayMemory(pScrn);
		pPriv->videoStatus = 0;
	} else {
		if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
			pPriv->videoStatus = OFF_TIMER | CLIENT_VIDEO_ON;
			pPriv->videoTime = currentTime.milliseconds + OFF_DELAY;
			pNv->VideoTimerCallback = NVVideoTimerCallback;
		}
	}
}

/**
 * QueryBestSize
 * used by client applications to ask the driver:
 * how would you actually scale a video of dimensions
 * vid_w, vid_h, if i wanted you to scale it to dimensions
 * drw_w, drw_h?
 * function stores actual scaling size in pointers p_w, p_h.
 *
 *
 * @param pScrn unused
 * @param motion unused
 * @param vid_w width of source video
 * @param vid_h height of source video
 * @param drw_w desired scaled width as requested by client
 * @param drw_h desired scaled height as requested by client
 * @param p_w actual scaled width as the driver is capable of
 * @param p_h actual scaled height as the driver is capable of
 * @param data unused
 */
static void
NVQueryBestSize(ScrnInfoPtr pScrn, Bool motion,
		short vid_w, short vid_h,
		short drw_w, short drw_h,
		unsigned int *p_w, unsigned int *p_h,
		pointer data)
{
	if(vid_w > (drw_w << 3))
		drw_w = vid_w >> 3;
	if(vid_h > (drw_h << 3))
		drw_h = vid_h >> 3;

	*p_w = drw_w;
	*p_h = drw_h;
}

/**
 * NVCopyData420
 * used to convert YV12 to YUY2 for the blitter and NV04 overlay.
 * The U and V samples generated are linearly interpolated on the vertical
 * axis for better quality
 *
 * @param src1 source buffer of luma
 * @param src2 source buffer of chroma1
 * @param src3 source buffer of chroma2
 * @param dst1 destination buffer
 * @param srcPitch pitch of src1
 * @param srcPitch2 pitch of src2, src3
 * @param dstPitch pitch of dst1
 * @param h number of lines to copy
 * @param w length of lines to copy
 */
static inline void
NVCopyData420(unsigned char *src1, unsigned char *src2, unsigned char *src3,
	      unsigned char *dst1, int srcPitch, int srcPitch2, int dstPitch,
	      int h, int w)
{
	CARD32 *dst;
	CARD8 *s1, *s2, *s3;
	int i, j;

#define su(X) (((j & 1) && j < (h-1)) ? ((unsigned)((signed int)s2[X] +        \
		(signed int)(s2 + srcPitch2)[X]) / 2) : (s2[X]))
#define sv(X) (((j & 1) && j < (h-1)) ? ((unsigned)((signed int)s3[X] +        \
		(signed int)(s3 + srcPitch2)[X]) / 2) : (s3[X]))

	w >>= 1;

	for (j = 0; j < h; j++) {
		dst = (CARD32*)dst1;
		s1 = src1;  s2 = src2;  s3 = src3;
		i = w;

		while (i > 4) {
#if X_BYTE_ORDER == X_BIG_ENDIAN
		dst[0] = (s1[0] << 24) | (s1[1] << 8) | (sv(0) << 16) | su(0);
		dst[1] = (s1[2] << 24) | (s1[3] << 8) | (sv(1) << 16) | su(1);
		dst[2] = (s1[4] << 24) | (s1[5] << 8) | (sv(2) << 16) | su(2);
		dst[3] = (s1[6] << 24) | (s1[7] << 8) | (sv(3) << 16) | su(3);
#else
		dst[0] = s1[0] | (s1[1] << 16) | (sv(0) << 8) | (su(0) << 24);
		dst[1] = s1[2] | (s1[3] << 16) | (sv(1) << 8) | (su(1) << 24);
		dst[2] = s1[4] | (s1[5] << 16) | (sv(2) << 8) | (su(2) << 24);
		dst[3] = s1[6] | (s1[7] << 16) | (sv(3) << 8) | (su(3) << 24);
#endif
		dst += 4; s2 += 4; s3 += 4; s1 += 8;
		i -= 4;
		}

		while (i--) {
#if X_BYTE_ORDER == X_BIG_ENDIAN
		dst[0] = (s1[0] << 24) | (s1[1] << 8) | (sv(0) << 16) | su(0);
#else
		dst[0] = s1[0] | (s1[1] << 16) | (sv(0) << 8) | (su(0) << 24);
#endif
		dst++; s2++; s3++;
		s1 += 2;
		}

		dst1 += dstPitch;
		src1 += srcPitch;
		if (j & 1) {
			src2 += srcPitch2;
			src3 += srcPitch2;
		}
	}
}

/**
 * NVCopyNV12ColorPlanes
 * Used to convert YV12 color planes to NV12 (interleaved UV) for the overlay
 *
 * @param src1 source buffer of chroma1
 * @param dst1 destination buffer
 * @param h number of lines to copy
 * @param w length of lines to copy
 * @param id source pixel format (YV12 or I420)
 */
static inline void
NVCopyNV12ColorPlanes(unsigned char *src1, unsigned char *src2,
		      unsigned char *dst, int dstPitch, int srcPitch2,
		      int h, int w)
{
	int i, j, l, e;

	w >>= 1;
	h >>= 1;
#ifdef __SSE2__
	l = w >> 3;
	e = w & 7;
#else
	l = w >> 1;
	e = w & 1;
#endif

	for (j = 0; j < h; j++) {
		unsigned char *us = src1;
		unsigned char *vs = src2;
		unsigned int *vuvud = (unsigned int *) dst;
		unsigned short *vud;

		for (i = 0; i < l; i++) {
#ifdef __SSE2__
			_mm_storeu_si128(
				(void*)vuvud,
				_mm_unpacklo_epi8(
					_mm_loadl_epi64((void*)vs),
					_mm_loadl_epi64((void*)us)));
			vuvud+=4;
			us+=8;
			vs+=8;
#else /* __SSE2__ */
#  if X_BYTE_ORDER == X_BIG_ENDIAN
			*vuvud++ = (vs[0]<<24) | (us[0]<<16) | (vs[1]<<8) | us[1];
#  else
			*vuvud++ = vs[0] | (us[0]<<8) | (vs[1]<<16) | (us[1]<<24);
#  endif
			us+=2;
			vs+=2;
#endif /* __SSE2__ */
		}

		vud = (unsigned short *)vuvud;
		for (i = 0; i < e; i++) {
#if X_BYTE_ORDER == X_BIG_ENDIAN
			vud[i] = us[i] | (vs[i]<<8);
#else
			vud[i] = vs[i] | (us[i]<<8);
#endif
		}

		dst += dstPitch;
		src1 += srcPitch2;
		src2 += srcPitch2;
	}

}


static int
NV_set_dimensions(ScrnInfoPtr pScrn, int action_flags, INT32 *xa, INT32 *xb,
		  INT32 *ya, INT32 *yb, short *src_x, short *src_y,
		  short *src_w, short *src_h, short *drw_x, short *drw_y,
		  short *drw_w, short *drw_h, int *left, int *top, int *right,
		  int *bottom, BoxRec *dstBox, int *npixels, int *nlines,
		  RegionPtr clipBoxes, short width, short height)
{
	NVPtr pNv = NVPTR(pScrn);

	if (action_flags & USE_OVERLAY) {
		switch (pNv->Architecture) {
		case NV_ARCH_04:
			/* NV0x overlay can't scale down. at all. */
			if (*drw_w < *src_w)
				*drw_w = *src_w;
			if (*drw_h < *src_h)
				*drw_h = *src_h;
			break;
		case NV_ARCH_30:
			/* According to DirectFB, NV3x can't scale down by
			 * a ratio > 2
			 */
			if (*drw_w < (*src_w) >> 1)
				*drw_w = *src_w;
			if (*drw_h < (*src_h) >> 1)
				*drw_h = *src_h;
			break;
		default: /*NV10, NV20*/
			/* NV1x overlay can't scale down by a ratio > 8 */
			if (*drw_w < (*src_w) >> 3)
				*drw_w = *src_w >> 3;
			if (*drw_h < (*src_h >> 3))
				*drw_h = *src_h >> 3;
		}
	}

	/* Clip */
	*xa = *src_x;
	*xb = *src_x + *src_w;
	*ya = *src_y;
	*yb = *src_y + *src_h;

	dstBox->x1 = *drw_x;
	dstBox->x2 = *drw_x + *drw_w;
	dstBox->y1 = *drw_y;
	dstBox->y2 = *drw_y + *drw_h;

	/* In randr 1.2 mode VIDEO_CLIP_TO_VIEWPORT is broken (hence it is not
	 * set in the overlay adapter flags) since pScrn->frame{X,Y}1 do not get
	 * updated. Hence manual clipping against the CRTC dimensions
	 */
	if (action_flags & USE_OVERLAY) {
		NVPortPrivPtr pPriv = GET_OVERLAY_PRIVATE(pNv);
		unsigned id = pPriv->overlayCRTC;
		xf86CrtcPtr crtc = XF86_CRTC_CONFIG_PTR(pScrn)->crtc[id];
		RegionRec VPReg;
		BoxRec VPBox;

		VPBox.x1 = crtc->x;
		VPBox.y1 = crtc->y;
		VPBox.x2 = crtc->x + crtc->mode.HDisplay;
		VPBox.y2 = crtc->y + crtc->mode.VDisplay;

		REGION_INIT(pScreen, &VPReg, &VPBox, 1);
		REGION_INTERSECT(pScreen, clipBoxes, clipBoxes, &VPReg);
		REGION_UNINIT(pScreen, &VPReg);
	}

	if (!xf86XVClipVideoHelper(dstBox, xa, xb, ya, yb, clipBoxes,
				   width, height))
		return -1;

	if (action_flags & USE_OVERLAY)	{
		xf86CrtcConfigPtr xf86_config =
			XF86_CRTC_CONFIG_PTR(pScrn);
		NVPortPrivPtr pPriv = GET_OVERLAY_PRIVATE(pNv);

		dstBox->x1 -= xf86_config->crtc[pPriv->overlayCRTC]->x;
		dstBox->x2 -= xf86_config->crtc[pPriv->overlayCRTC]->x;
		dstBox->y1 -= xf86_config->crtc[pPriv->overlayCRTC]->y;
		dstBox->y2 -= xf86_config->crtc[pPriv->overlayCRTC]->y;
	}

	/* Convert fixed point to integer, as xf86XVClipVideoHelper probably
	 * turns its parameter into fixed point values
	 */
	*left = (*xa) >> 16;
	if (*left < 0)
		*left = 0;

	*top = (*ya) >> 16;
	if (*top < 0)
		*top = 0;

	*right = (*xb) >> 16;
	if (*right > width)
		*right = width;

	*bottom = (*yb) >> 16;
	if (*bottom > height)
		*bottom = height;

	if (action_flags & IS_YV12) {
		/* even "left", even "top", even number of pixels per line
		 * and even number of lines
		 */
		*left &= ~1;
		*npixels = ((*right + 1) & ~1) - *left;
		*top &= ~1;
		*nlines = ((*bottom + 1) & ~1) - *top;
	} else
	if (action_flags & IS_YUY2) {
		/* even "left" */
		*left &= ~1;
		/* even number of pixels per line */
		*npixels = ((*right + 1) & ~1) - *left;
		*nlines = *bottom - *top;
		/* 16bpp */
		*left <<= 1;
	} else
	if (action_flags & IS_RGB) {
		*npixels = *right - *left;
		*nlines = *bottom - *top;
		/* 32bpp */
		*left <<= 2;
	}

	return 0;
}

static int
NV_calculate_pitches_and_mem_size(NVPtr pNv, int action_flags, int *srcPitch,
				  int *srcPitch2, int *dstPitch, int *s2offset,
				  int *s3offset, int *uv_offset,
				  int *newFBSize, int *newTTSize,
				  int *line_len, int npixels, int nlines,
				  int width, int height)
{
	int tmp;

	if (pNv->Architecture >= NV_TESLA) {
		npixels = (npixels + 7) & ~7;
		nlines = (nlines + 7) & ~7;
	}

	if (action_flags & IS_YV12) {
		*srcPitch = (width + 3) & ~3;	/* of luma */
		*s2offset = *srcPitch * height;
		*srcPitch2 = ((width >> 1) + 3) & ~3; /*of chroma*/
		*s3offset = (*srcPitch2 * (height >> 1)) + *s2offset;
		*dstPitch = (npixels + 63) & ~63; /*luma and chroma pitch*/
		*line_len = npixels;
		*uv_offset = nlines * *dstPitch;
		*newFBSize = *uv_offset + (nlines >> 1) * *dstPitch;
		*newTTSize = *uv_offset + (nlines >> 1) * *dstPitch;
	} else
	if (action_flags & IS_YUY2) {
		*srcPitch = width << 1; /* one luma, one chroma per pixel */
		*dstPitch = ((npixels << 1) + 63) & ~63;
		*line_len = npixels << 1;
		*newFBSize = nlines * *dstPitch;
		*newTTSize = nlines * *line_len;
	} else
	if (action_flags & IS_RGB) {
		/* one R, one G, one B, one X per pixel */
		*srcPitch = width << 2;
		*dstPitch = ((npixels << 2) + 63) & ~63;
		*line_len = npixels << 2;
		*newFBSize = nlines * *dstPitch;
		*newTTSize = nlines * *dstPitch;
	}

	if (action_flags & CONVERT_TO_YUY2) {
		*dstPitch = ((npixels << 1) + 63) & ~63;
		*line_len = npixels << 1;
		*newFBSize = nlines * *dstPitch;
		*newTTSize = nlines * *line_len;
		*uv_offset = 0;
	}

	if (action_flags & SWAP_UV)  {
		/* I420 swaps U and V */
		tmp = *s2offset;
		*s2offset = *s3offset;
		*s3offset = tmp;
	}

	/* Overlay double buffering... */
	if (action_flags & USE_OVERLAY)
                (*newFBSize) <<= 1;

	return 0;
}


/**
 * NV_set_action_flags
 * This function computes the action flags from the input image,
 * that is, it decides what NVPutImage and its helpers must do.
 * This eases readability by avoiding lots of switch-case statements in the
 * core NVPutImage
 */
static void
NV_set_action_flags(ScrnInfoPtr pScrn, DrawablePtr pDraw, NVPortPrivPtr pPriv,
		    int id, short drw_x, short drw_y, short drw_w, short drw_h,
		    int *action_flags)
{
	NVPtr pNv = NVPTR(pScrn);

#define USING_OVERLAY (*action_flags & USE_OVERLAY)
#define USING_TEXTURE (*action_flags & USE_TEXTURE)
#define USING_BLITTER ((!(*action_flags & USE_OVERLAY)) &&                     \
		       (!(*action_flags & USE_TEXTURE)))

	*action_flags = 0;

	/* Pixel format-related bits */
	if (id == FOURCC_YUY2 || id == FOURCC_UYVY)
		*action_flags |= IS_YUY2;

	if (id == FOURCC_YV12 || id == FOURCC_I420)
		*action_flags |= IS_YV12;

	if (id == FOURCC_RGB) /*How long will we support it?*/
		*action_flags |= IS_RGB;

	if (id == FOURCC_I420) /* I420 is YV12 with swapped UV */
		*action_flags |= SWAP_UV;

	/* Desired adapter */
	if (!pPriv->blitter && !pPriv->texture)
		*action_flags |= USE_OVERLAY;

	if (!pPriv->blitter && pPriv->texture)
		*action_flags |= USE_TEXTURE;

	/* Adapter fallbacks (when the desired one can't be used)*/
#ifdef COMPOSITE
	{
		PixmapPtr ppix = NVGetDrawablePixmap(pDraw);

		/* this is whether ppix is in the viewable fb, not related to
		   the EXA "offscreen" stuff */
		if (!nouveau_exa_pixmap_is_onscreen(ppix))
			*action_flags &= ~USE_OVERLAY;
	}
#endif

#ifdef NVOVL_SUPPORT
	if (USING_OVERLAY) {
		char crtc = nv_window_belongs_to_crtc(pScrn, drw_x, drw_y,
						      drw_w, drw_h);

		if ((crtc & (1 << 0)) && (crtc & (1 << 1))) {
			/* The overlay cannot be used on two CRTCs at a time,
			 * so we need to fallback on the blitter
			 */
			*action_flags &= ~USE_OVERLAY;
		} else
		if ((crtc & (1 << 0))) {
			/* We need to put the overlay on CRTC0 - if it's not
			 * already here
			 */
			if (pPriv->overlayCRTC == 1) {
				NVWriteCRTC(pNv, 0, NV_PCRTC_ENGINE_CTRL,
					    NVReadCRTC(pNv, 0, NV_PCRTC_ENGINE_CTRL) |
					    NV_CRTC_FSEL_OVERLAY);
				NVWriteCRTC(pNv, 1, NV_PCRTC_ENGINE_CTRL,
					    NVReadCRTC(pNv, 1, NV_PCRTC_ENGINE_CTRL) &
					    ~NV_CRTC_FSEL_OVERLAY);
				pPriv->overlayCRTC = 0;
			}
		} else
		if ((crtc & (1 << 1))) {
			if (pPriv->overlayCRTC == 0) {
				NVWriteCRTC(pNv, 1, NV_PCRTC_ENGINE_CTRL,
					    NVReadCRTC(pNv, 1, NV_PCRTC_ENGINE_CTRL) |
					    NV_CRTC_FSEL_OVERLAY);
				NVWriteCRTC(pNv, 0, NV_PCRTC_ENGINE_CTRL,
					    NVReadCRTC(pNv, 0, NV_PCRTC_ENGINE_CTRL) &
					    ~NV_CRTC_FSEL_OVERLAY);
				pPriv->overlayCRTC = 1;
			}
		}

		if (XF86_CRTC_CONFIG_PTR(pScrn)->crtc[pPriv->overlayCRTC]
						 ->rotation != RR_Rotate_0)
			*action_flags &= ~USE_OVERLAY;
	}
#endif

	/* At this point the adapter we're going to use is _known_.
	 * You cannot change it now.
	 */

	/* Card/adapter format restrictions */
	if (USING_BLITTER) {
		/* The blitter does not handle YV12 natively */
		if (id == FOURCC_YV12 || id == FOURCC_I420)
			*action_flags |= CONVERT_TO_YUY2;
	}

	if (USING_OVERLAY && (pNv->Architecture == NV_ARCH_04)) {
		/* NV04-05 don't support YV12, only YUY2 and ITU-R BT.601 */
		if (*action_flags & IS_YV12)
			*action_flags |= CONVERT_TO_YUY2;
	}

	if (USING_OVERLAY && (pNv->Architecture == NV_ARCH_10 ||
			      pNv->Architecture == NV_ARCH_20)) {
		/* No YV12 overlay on NV10, 11, 15, 20, NFORCE */
		switch (pNv->dev->chipset) {
		case 0x10:
		case 0x11:
		case 0x15:
		case 0x1a: /*XXX: unsure about nforce */
		case 0x20:
			*action_flags |= CONVERT_TO_YUY2;
			break;
		default:
			break;
		}
	}
}


/**
 * NVPutImage
 * PutImage is "the" important function of the Xv extension.
 * a client (e.g. video player) calls this function for every
 * image (of the video) to be displayed. this function then
 * scales and displays the image.
 *
 * @param pScrn screen which hold the port where the image is put
 * @param src_x source point in the source image to start displaying from
 * @param src_y see above
 * @param src_w width of the source image to display
 * @param src_h see above
 * @param drw_x  screen point to display to
 * @param drw_y
 * @param drw_w width of the screen drawable
 * @param drw_h
 * @param id pixel format of image
 * @param buf pointer to buffer containing the source image
 * @param width total width of the source image we are passed
 * @param height
 * @param Sync unused
 * @param clipBoxes ??
 * @param data pointer to port
 * @param pDraw drawable pointer
 */
static int
NVPutImage(ScrnInfoPtr pScrn, short src_x, short src_y, short drw_x,
	   short drw_y, short src_w, short src_h, short drw_w, short drw_h,
	   int id, unsigned char *buf, short width, short height,
	   Bool Sync, RegionPtr clipBoxes, pointer data, DrawablePtr pDraw)
{
	NVPortPrivPtr pPriv = (NVPortPrivPtr)data;
	NVPtr pNv = NVPTR(pScrn);
	PixmapPtr ppix;
	/* source box */
	INT32 xa = 0, xb = 0, ya = 0, yb = 0;
	/* size to allocate in VRAM and in GART respectively */
	int newFBSize = 0, newTTSize = 0;
	/* card VRAM offsets, source offsets for U and V planes */
	int offset = 0, uv_offset = 0, s2offset = 0, s3offset = 0;
	/* source pitch, source pitch of U and V planes in case of YV12,
	 * VRAM destination pitch
	 */
	int srcPitch = 0, srcPitch2 = 0, dstPitch = 0;
	/* position of the given source data (using src_*), number of pixels
	 * and lines we are interested in
	 */
	int top = 0, left = 0, right = 0, bottom = 0, npixels = 0, nlines = 0;
	Bool skip = FALSE;
	BoxRec dstBox;
	CARD32 tmp = 0;
	int line_len = 0; /* length of a line, like npixels, but in bytes */
	struct nouveau_bo *destination_buffer = NULL;
	int action_flags; /* what shall we do? */
	unsigned char *map;
	int ret, i;

	if (pPriv->grabbedByV4L)
		return Success;

	if (width > pPriv->max_image_dim || height > pPriv->max_image_dim)
		return BadMatch;

	NV_set_action_flags(pScrn, pDraw, pPriv, id, drw_x, drw_y, drw_w,
			    drw_h, &action_flags);

	if (NV_set_dimensions(pScrn, action_flags, &xa, &xb, &ya, &yb,
			      &src_x,  &src_y, &src_w, &src_h,
			      &drw_x, &drw_y, &drw_w, &drw_h,
			      &left, &top, &right, &bottom, &dstBox,
			      &npixels, &nlines, clipBoxes, width, height))
		return Success;

	if (NV_calculate_pitches_and_mem_size(pNv, action_flags, &srcPitch,
					      &srcPitch2, &dstPitch, &s2offset,
					      &s3offset, &uv_offset,
					      &newFBSize, &newTTSize,
					      &line_len, npixels, nlines,
					      width, height))
		return BadImplementation;

	/* There are some cases (tvtime with overscan for example) where the
	 * input image is larger (width/height) than the source rectangle for
	 * the overlay (src_w, src_h). In those cases, we try to do something
	 * optimal by uploading only the necessary data.
	 */
	if (action_flags & IS_YUY2 || action_flags & IS_RGB)
		buf += (top * srcPitch) + left;

	if (action_flags & IS_YV12) {
		tmp = ((top >> 1) * srcPitch2) + (left >> 1);
		s2offset += tmp;
		s3offset += tmp;
	}

	ret = nouveau_xv_bo_realloc(pScrn, NOUVEAU_BO_VRAM, newFBSize,
				    &pPriv->video_mem);
	if (ret)
		return BadAlloc;

#ifdef NVOVL_SUPPORT
	if (action_flags & USE_OVERLAY) {
		ret = nouveau_bo_pin(pPriv->video_mem, NOUVEAU_BO_VRAM);
		if (ret) {
			nouveau_bo_ref(NULL, &pPriv->video_mem);
			return BadAlloc;
		}
	}
#endif

	/* The overlay supports hardware double buffering. We handle this here*/
	offset = 0;
#ifdef NVOVL_SUPPORT
	if (pPriv->doubleBuffer) {
		int mask = 1 << (pPriv->currentBuffer << 2);

		/* overwrite the newest buffer if there's not one free */
		if (nvReadVIDEO(pNv, NV_PVIDEO_BUFFER) & mask) {
			if (!pPriv->currentBuffer)
				offset += newFBSize >> 1;
			skip = TRUE;
		} else {
			if (pPriv->currentBuffer)
				offset += newFBSize >> 1;
		}
	}
#endif

	/* Now we take a decision regarding the way we send the data to the
	 * card.
	 *
	 * Either we use double buffering of "private" TT memory
	 * Either we rely on X's GARTScratch
	 * Either we fallback on CPU copy
	 */

	/* Try to allocate host-side double buffers, unless we have already
	 * failed
	 */

	/* We take only nlines * line_len bytes - that is, only the pixel
	 * data we are interested in - because the stuff in the GART is
	 * written contiguously
	 */
	if (pPriv->currentHostBuffer != NO_PRIV_HOST_BUFFER_AVAILABLE) {
		ret = nouveau_xv_bo_realloc(pScrn, NOUVEAU_BO_GART, newTTSize,
					    &pPriv->TT_mem_chunk[0]);
		if (ret == 0) {
			ret = nouveau_xv_bo_realloc(pScrn, NOUVEAU_BO_GART,
						    newTTSize,
						    &pPriv->TT_mem_chunk[1]);
			if (ret) {
				nouveau_bo_ref(NULL, &pPriv->TT_mem_chunk[0]);
				pPriv->currentHostBuffer =
					NO_PRIV_HOST_BUFFER_AVAILABLE;
			}
		} else {
			pPriv->currentHostBuffer =
				NO_PRIV_HOST_BUFFER_AVAILABLE;
		}
	}

	if (pPriv->currentHostBuffer != NO_PRIV_HOST_BUFFER_AVAILABLE) {
		destination_buffer =
			pPriv->TT_mem_chunk[pPriv->currentHostBuffer];
	}
	if (!destination_buffer) {
		if (pNv->Architecture >= NV_TESLA) {
			NOUVEAU_ERR("No scratch buffer for tiled upload\n");
			return BadAlloc;
		}

		goto CPU_copy;
	}

	if (newTTSize <= destination_buffer->size) {
		unsigned char *dst;

		/* Upload to GART */
		nouveau_bo_map(destination_buffer, NOUVEAU_BO_WR, pNv->client);
		dst = destination_buffer->map;

		if (action_flags & IS_YV12) {
			if (action_flags & CONVERT_TO_YUY2) {
				NVCopyData420(buf + (top * srcPitch) + left,
					      buf + s2offset, buf + s3offset,
					      dst, srcPitch, srcPitch2,
					      line_len, nlines, npixels);
			} else {
				/* Native YV12 */
				unsigned char *tbuf = buf + top *
						      srcPitch + left;
				unsigned char *tdst = dst;

				/* luma upload */
				for (i = 0; i < nlines; i++) {
					memcpy(tdst, tbuf, line_len);
					tdst += line_len;
					tbuf += srcPitch;
				}
				dst += line_len * nlines;

				NVCopyNV12ColorPlanes(buf + s2offset,
						      buf + s3offset, dst,
						      line_len, srcPitch2,
						      nlines, npixels);
			}
		} else {
			for (i = 0; i < nlines; i++) {
				memcpy(dst, buf, line_len);
				dst += line_len;
				buf += srcPitch;
			}
		}

		if (uv_offset) {
			NVAccelM2MF(pNv, line_len, nlines / 2, 1,
				    line_len * nlines, uv_offset,
				    destination_buffer, NOUVEAU_BO_GART,
				    line_len, nlines >> 1, 0, 0,
				    pPriv->video_mem, NOUVEAU_BO_VRAM,
				    dstPitch, nlines >> 1, 0, 0);
		}

		NVAccelM2MF(pNv, line_len, nlines, 1, 0, 0,
			    destination_buffer, NOUVEAU_BO_GART,
			    line_len, nlines, 0, 0,
			    pPriv->video_mem, NOUVEAU_BO_VRAM,
			    dstPitch, nlines, 0, 0);

	} else {
CPU_copy:
		nouveau_bo_map(pPriv->video_mem, NOUVEAU_BO_WR, pNv->client);
		map = pPriv->video_mem->map + offset;

		if (action_flags & IS_YV12) {
			if (action_flags & CONVERT_TO_YUY2) {
				NVCopyData420(buf + (top * srcPitch) + left,
					      buf + s2offset, buf + s3offset,
					      map, srcPitch, srcPitch2,
					      dstPitch, nlines, npixels);
			} else {
				unsigned char *tbuf =
					buf + left + top * srcPitch;

				for (i = 0; i < nlines; i++) {
					int dwords = npixels << 1;

					while (dwords & ~0x03) {
						*map = *tbuf;
						*(map + 1) = *(tbuf + 1);
						*(map + 2) = *(tbuf + 2);
						*(map + 3) = *(tbuf + 3);
						map += 4;
						tbuf += 4;
						dwords -= 4;
					}

					switch (dwords) {
					case 3: *(map + 2) = *(tbuf + 2);
					case 2: *(map + 1) = *(tbuf + 1);
					case 1: *map = *tbuf;
					}

					map += dstPitch - (npixels << 1);
					tbuf += srcPitch - (npixels << 1);
				}

				NVCopyNV12ColorPlanes(buf + s2offset,
						      buf + s3offset,
						      map, dstPitch, srcPitch2,
						      nlines, npixels);
			}
		} else {
			/* YUY2 and RGB */
			for (i = 0; i < nlines; i++) {
				int dwords = npixels << 1;

				while (dwords & ~0x03) {
					*map = *buf;
					*(map + 1) = *(buf + 1);
					*(map + 2) = *(buf + 2);
					*(map + 3) = *(buf + 3);
					map += 4;
					buf += 4;
					dwords -= 4;
				}

				switch (dwords) {
				case 3: *(map + 2) = *(buf + 2);
				case 2: *(map + 1) = *(buf + 1);
				case 1: *map = *buf;
				}

				map += dstPitch - (npixels << 1);
				buf += srcPitch - (npixels << 1);
			}
		}
	}

	if (skip)
		return Success;

	if (pPriv->currentHostBuffer != NO_PRIV_HOST_BUFFER_AVAILABLE)
		pPriv->currentHostBuffer ^= 1;

	/* If we're not using the hw overlay, we're rendering into a pixmap
	 * and need to take a couple of additional steps...
	 */
	if (!(action_flags & USE_OVERLAY)) {
		ppix = NVGetDrawablePixmap(pDraw);

		/* Ensure pixmap is in offscreen memory */
		pNv->exa_force_cp = TRUE;
		exaMoveInPixmap(ppix);
		pNv->exa_force_cp = FALSE;

		if (!exaGetPixmapDriverPrivate(ppix))
			return BadAlloc;

#ifdef COMPOSITE
		/* Convert screen coords to pixmap coords */
		if (ppix->screen_x || ppix->screen_y) {
			REGION_TRANSLATE(pScrn->pScreen, clipBoxes,
					 -ppix->screen_x, -ppix->screen_y);
			dstBox.x1 -= ppix->screen_x;
			dstBox.x2 -= ppix->screen_x;
			dstBox.y1 -= ppix->screen_y;
			dstBox.y2 -= ppix->screen_y;
		}
#endif
	}

	if (action_flags & USE_OVERLAY) {
		if (pNv->Architecture == NV_ARCH_04) {
			NV04PutOverlayImage(pScrn, pPriv->video_mem, offset,
					    id, dstPitch, &dstBox, 0, 0,
					    xb, yb, npixels, nlines,
					    src_w, src_h, drw_w, drw_h,
					    clipBoxes);
		} else {
			NV10PutOverlayImage(pScrn, pPriv->video_mem, offset,
					    uv_offset, id, dstPitch, &dstBox,
					    0, 0, xb, yb,
					    npixels, nlines, src_w, src_h,
					    drw_w, drw_h, clipBoxes);
		}

		pPriv->currentBuffer ^= 1;
	} else 
	if (action_flags & USE_TEXTURE) {
		ret = BadImplementation;

		if (pNv->Architecture == NV_ARCH_30) {
			ret = NV30PutTextureImage(pScrn, pPriv->video_mem,
						  offset, uv_offset,
						  id, dstPitch, &dstBox, 0, 0,
						  xb, yb, npixels, nlines,
						  src_w, src_h, drw_w, drw_h,
						  clipBoxes, ppix, pPriv);
		} else
		if (pNv->Architecture == NV_ARCH_40) {
			ret = NV40PutTextureImage(pScrn, pPriv->video_mem, 
						  offset, uv_offset,
						  id, dstPitch, &dstBox, 0, 0,
						  xb, yb, npixels, nlines,
						  src_w, src_h, drw_w, drw_h,
						  clipBoxes, ppix, pPriv);
		} else
		if (pNv->Architecture == NV_TESLA) {
			ret = nv50_xv_image_put(pScrn, pPriv->video_mem,
						offset, uv_offset,
						id, dstPitch, &dstBox, 0, 0,
						xb, yb, npixels, nlines,
						src_w, src_h, drw_w, drw_h,
						clipBoxes, ppix, pPriv);
		} else {
			ret = nvc0_xv_image_put(pScrn, pPriv->video_mem,
						offset, uv_offset,
						id, dstPitch, &dstBox, 0, 0,
						xb, yb, npixels, nlines,
						src_w, src_h, drw_w, drw_h,
						clipBoxes, ppix, pPriv);
		}

		if (ret != Success)
			return ret;
	} else {
		ret = NVPutBlitImage(pScrn, pPriv->video_mem, offset, id,
				     dstPitch, &dstBox, 0, 0, xb, yb, npixels,
				     nlines, src_w, src_h, drw_w, drw_h,
				     clipBoxes, ppix);
		if (ret != Success)
			return ret;
	}

#ifdef COMPOSITE
	/* Damage tracking */
	if (!(action_flags & USE_OVERLAY))
		DamageDamageRegion(&ppix->drawable, clipBoxes);
#endif

	return Success;
}

/**
 * QueryImageAttributes
 *
 * calculates
 * - size (memory required to store image),
 * - pitches,
 * - offsets
 * of image
 * depending on colorspace (id) and dimensions (w,h) of image
 * values of
 * - w,
 * - h
 * may be adjusted as needed
 *
 * @param pScrn unused
 * @param id colorspace of image
 * @param w pointer to width of image
 * @param h pointer to height of image
 * @param pitches pitches[i] = length of a scanline in plane[i]
 * @param offsets offsets[i] = offset of plane i from the beginning of the image
 * @return size of the memory required for the XvImage queried
 */
static int
NVQueryImageAttributes(ScrnInfoPtr pScrn, int id,
		       unsigned short *w, unsigned short *h,
		       int *pitches, int *offsets)
{
	int size, tmp;

	*w = (*w + 1) & ~1; // width rounded up to an even number
	if (offsets)
		offsets[0] = 0;

	switch (id) {
	case FOURCC_YV12:
	case FOURCC_I420:
		*h = (*h + 1) & ~1; // height rounded up to an even number
		size = (*w + 3) & ~3; // width rounded up to a multiple of 4
		if (pitches)
			pitches[0] = size; // width rounded up to a multiple of 4
		size *= *h;
		if (offsets)
			offsets[1] = size; // number of pixels in "rounded up" image
		tmp = ((*w >> 1) + 3) & ~3; // width/2 rounded up to a multiple of 4
		if (pitches)
			pitches[1] = pitches[2] = tmp; // width/2 rounded up to a multiple of 4
		tmp *= (*h >> 1); // 1/4*number of pixels in "rounded up" image
		size += tmp; // 5/4*number of pixels in "rounded up" image
		if (offsets)
			offsets[2] = size; // 5/4*number of pixels in "rounded up" image
		size += tmp; // = 3/2*number of pixels in "rounded up" image
		break;
	case FOURCC_UYVY:
	case FOURCC_YUY2:
		size = *w << 1; // 2*width
		if (pitches)
			pitches[0] = size; // 2*width
		size *= *h; // 2*width*height
		break;
	case FOURCC_RGB:
		size = *w << 2; // 4*width (32 bit per pixel)
		if (pitches)
			pitches[0] = size; // 4*width
		size *= *h; // 4*width*height
		break;
	case FOURCC_AI44:
	case FOURCC_IA44:
		size = *w; // width
		if (pitches)
			pitches[0] = size; // width
		size *= *h; // width*height
		break;
	default:
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Unknown colorspace: %x\n", id);
		*w = *h = size = 0;
		break;
	}

	return size;
}

/***** Exported offscreen surface stuff ****/


static int
NVAllocSurface(ScrnInfoPtr pScrn, int id,
	       unsigned short w, unsigned short h,
	       XF86SurfacePtr surface)
{
	NVPtr pNv = NVPTR(pScrn);
	NVPortPrivPtr pPriv = GET_OVERLAY_PRIVATE(pNv);
	int size, bpp, ret;

	bpp = pScrn->bitsPerPixel >> 3;

	if (pPriv->grabbedByV4L)
		return BadAlloc;

	if ((w > IMAGE_MAX_W) || (h > IMAGE_MAX_H))
		return BadValue;

	w = (w + 1) & ~1;
	pPriv->pitch = ((w << 1) + 63) & ~63;
	size = h * pPriv->pitch / bpp;

	ret = nouveau_xv_bo_realloc(pScrn, NOUVEAU_BO_VRAM, size,
				    &pPriv->video_mem);
	if (ret)
		return BadAlloc;
	pPriv->offset = 0;

	surface->width = w;
	surface->height = h;
	surface->pScrn = pScrn;
	surface->pitches = &pPriv->pitch;
	surface->offsets = &pPriv->offset;
	surface->devPrivate.ptr = (pointer)pPriv;
	surface->id = id;

	/* grab the video */
	NVStopOverlay(pScrn);
	pPriv->videoStatus = 0;
	REGION_EMPTY(pScrn->pScreen, &pPriv->clip);
	pPriv->grabbedByV4L = TRUE;

	return Success;
}

static int
NVStopSurface(XF86SurfacePtr surface)
{
	NVPortPrivPtr pPriv = (NVPortPrivPtr)(surface->devPrivate.ptr);

	if (pPriv->grabbedByV4L && pPriv->videoStatus) {
		NV10StopOverlay(surface->pScrn);
		pPriv->videoStatus = 0;
	}

	return Success;
}

static int
NVFreeSurface(XF86SurfacePtr surface)
{
	NVPortPrivPtr pPriv = (NVPortPrivPtr)(surface->devPrivate.ptr);

	if (pPriv->grabbedByV4L) {
		NVStopSurface(surface);
		NVFreeOverlayMemory(surface->pScrn);
		pPriv->grabbedByV4L = FALSE;
	}

	return Success;
}

static int
NVGetSurfaceAttribute(ScrnInfoPtr pScrn, Atom attribute, INT32 *value)
{
	NVPtr pNv = NVPTR(pScrn);
	NVPortPrivPtr pPriv = GET_OVERLAY_PRIVATE(pNv);

	return NV10GetOverlayPortAttribute(pScrn, attribute,
					 value, (pointer)pPriv);
}

static int
NVSetSurfaceAttribute(ScrnInfoPtr pScrn, Atom attribute, INT32 value)
{
	NVPtr pNv = NVPTR(pScrn);
	NVPortPrivPtr pPriv = GET_OVERLAY_PRIVATE(pNv);

	return NV10SetOverlayPortAttribute(pScrn, attribute,
					 value, (pointer)pPriv);
}

static int
NVDisplaySurface(XF86SurfacePtr surface,
		 short src_x, short src_y,
		 short drw_x, short drw_y,
		 short src_w, short src_h,
		 short drw_w, short drw_h,
		 RegionPtr clipBoxes)
{
	ScrnInfoPtr pScrn = surface->pScrn;
	NVPortPrivPtr pPriv = (NVPortPrivPtr)(surface->devPrivate.ptr);
	INT32 xa, xb, ya, yb;
	BoxRec dstBox;

	if (!pPriv->grabbedByV4L)
		return Success;

	if (src_w > (drw_w << 3))
		drw_w = src_w >> 3;
	if (src_h > (drw_h << 3))
		drw_h = src_h >> 3;

	/* Clip */
	xa = src_x;
	xb = src_x + src_w;
	ya = src_y;
	yb = src_y + src_h;

	dstBox.x1 = drw_x;
	dstBox.x2 = drw_x + drw_w;
	dstBox.y1 = drw_y;
	dstBox.y2 = drw_y + drw_h;

	if(!xf86XVClipVideoHelper(&dstBox, &xa, &xb, &ya, &yb, clipBoxes,
				  surface->width, surface->height))
		return Success;

	dstBox.x1 -= pScrn->frameX0;
	dstBox.x2 -= pScrn->frameX0;
	dstBox.y1 -= pScrn->frameY0;
	dstBox.y2 -= pScrn->frameY0;

	pPriv->currentBuffer = 0;

	NV10PutOverlayImage(pScrn, pPriv->video_mem, surface->offsets[0],
			    0, surface->id, surface->pitches[0], &dstBox,
			    xa, ya, xb, yb, surface->width, surface->height,
			    src_w, src_h, drw_w, drw_h, clipBoxes);

	return Success;
}

/**
 * NVSetupBlitVideo
 * this function does all the work setting up a blit port
 *
 * @return blit port
 */
static XF86VideoAdaptorPtr
NVSetupBlitVideo (ScreenPtr pScreen)
{
	ScrnInfoPtr         pScrn = xf86ScreenToScrn(pScreen);
	NVPtr               pNv       = NVPTR(pScrn);
	XF86VideoAdaptorPtr adapt;
	NVPortPrivPtr       pPriv;
	int i;

	if (!(adapt = calloc(1, sizeof(XF86VideoAdaptorRec) +
					sizeof(NVPortPrivRec) +
					(sizeof(DevUnion) * NUM_BLIT_PORTS)))) {
		return NULL;
	}

	adapt->type		= XvWindowMask | XvInputMask | XvImageMask;
	adapt->flags		= 0;
	adapt->name		= "NV Video Blitter";
	adapt->nEncodings	= 1;
	adapt->pEncodings	= &DummyEncoding;
	adapt->nFormats		= NUM_FORMATS_ALL;
	adapt->pFormats		= NVFormats;
	adapt->nPorts		= NUM_BLIT_PORTS;
	adapt->pPortPrivates	= (DevUnion*)(&adapt[1]);

	pPriv = (NVPortPrivPtr)(&adapt->pPortPrivates[NUM_BLIT_PORTS]);
	for(i = 0; i < NUM_BLIT_PORTS; i++)
		adapt->pPortPrivates[i].ptr = (pointer)(pPriv);

	if (pNv->dev->chipset >= 0x11) {
		adapt->pAttributes = NVBlitAttributes;
		adapt->nAttributes = NUM_BLIT_ATTRIBUTES;
	} else {
		adapt->pAttributes = NULL;
		adapt->nAttributes = 0;
	}

	adapt->pImages			= NVImages;
	adapt->nImages			= NUM_IMAGES_ALL;
	adapt->PutVideo			= NULL;
	adapt->PutStill			= NULL;
	adapt->GetVideo			= NULL;
	adapt->GetStill			= NULL;
	adapt->StopVideo		= NVStopBlitVideo;
	adapt->SetPortAttribute		= NVSetBlitPortAttribute;
	adapt->GetPortAttribute		= NVGetBlitPortAttribute;
	adapt->QueryBestSize		= NVQueryBestSize;
	adapt->PutImage			= NVPutImage;
	adapt->QueryImageAttributes	= NVQueryImageAttributes;

	pPriv->videoStatus		= 0;
	pPriv->grabbedByV4L		= FALSE;
	pPriv->blitter			= TRUE;
	pPriv->texture			= FALSE;
	pPriv->bicubic			= FALSE;
	pPriv->doubleBuffer		= FALSE;
	pPriv->SyncToVBlank		= (pNv->dev->chipset >= 0x11);
	pPriv->max_image_dim            = 2048;

	pNv->blitAdaptor		= adapt;

	return adapt;
}

/**
 * NVSetupOverlayVideo
 * this function does all the work setting up an overlay port
 *
 * @return overlay port
 */
static XF86VideoAdaptorPtr
NVSetupOverlayVideoAdapter(ScreenPtr pScreen)
{
	ScrnInfoPtr         pScrn = xf86ScreenToScrn(pScreen);
	NVPtr               pNv       = NVPTR(pScrn);
	XF86VideoAdaptorPtr adapt;
	NVPortPrivPtr       pPriv;

	if (!(adapt = calloc(1, sizeof(XF86VideoAdaptorRec) +
					sizeof(NVPortPrivRec) +
					sizeof(DevUnion)))) {
		return NULL;
	}

	adapt->type		= XvWindowMask | XvInputMask | XvImageMask;
	adapt->flags		= VIDEO_OVERLAID_IMAGES;
	adapt->name		= "NV Video Overlay";
	adapt->nEncodings	= 1;
	adapt->pEncodings	= &DummyEncoding;
	adapt->nFormats		= NUM_FORMATS_ALL;
	adapt->pFormats		= NVFormats;
	adapt->nPorts		= 1;
	adapt->pPortPrivates	= (DevUnion*)(&adapt[1]);

	pPriv = (NVPortPrivPtr)(&adapt->pPortPrivates[1]);
	adapt->pPortPrivates[0].ptr	= (pointer)(pPriv);

	adapt->pAttributes		= (pNv->Architecture != NV_ARCH_04) ? NV10OverlayAttributes : NV04OverlayAttributes;
	adapt->nAttributes		= (pNv->Architecture != NV_ARCH_04) ? NUM_NV10_OVERLAY_ATTRIBUTES : NUM_NV04_OVERLAY_ATTRIBUTES;
	adapt->pImages			= NVImages;
	adapt->nImages			= NUM_IMAGES_YUV;
	adapt->PutVideo			= NULL;
	adapt->PutStill			= NULL;
	adapt->GetVideo			= NULL;
	adapt->GetStill			= NULL;
	adapt->StopVideo		= NVStopOverlayVideo;
	adapt->SetPortAttribute		= (pNv->Architecture != NV_ARCH_04) ? NV10SetOverlayPortAttribute : NV04SetOverlayPortAttribute;
	adapt->GetPortAttribute		= (pNv->Architecture != NV_ARCH_04) ? NV10GetOverlayPortAttribute : NV04GetOverlayPortAttribute;
	adapt->QueryBestSize		= NVQueryBestSize;
	adapt->PutImage			= NVPutImage;
	adapt->QueryImageAttributes	= NVQueryImageAttributes;

	pPriv->videoStatus		= 0;
	pPriv->currentBuffer		= 0;
	pPriv->grabbedByV4L		= FALSE;
	pPriv->blitter			= FALSE;
	pPriv->texture			= FALSE;
	pPriv->bicubic			= FALSE;
	pPriv->max_image_dim            = 2048;

	NVSetPortDefaults (pScrn, pPriv);

	/* gotta uninit this someplace */
	REGION_NULL(pScreen, &pPriv->clip);

	pNv->overlayAdaptor	= adapt;

	xvBrightness		= MAKE_ATOM("XV_BRIGHTNESS");
	xvColorKey		= MAKE_ATOM("XV_COLORKEY");
	xvAutopaintColorKey     = MAKE_ATOM("XV_AUTOPAINT_COLORKEY");
	xvSetDefaults           = MAKE_ATOM("XV_SET_DEFAULTS");

	if ( pNv->Architecture != NV_ARCH_04 )
		{
		xvDoubleBuffer		= MAKE_ATOM("XV_DOUBLE_BUFFER");
		xvContrast		= MAKE_ATOM("XV_CONTRAST");
		xvSaturation		= MAKE_ATOM("XV_SATURATION");
		xvHue			= MAKE_ATOM("XV_HUE");
		xvITURBT709		= MAKE_ATOM("XV_ITURBT_709");
		xvOnCRTCNb		= MAKE_ATOM("XV_ON_CRTC_NB");
		NV10WriteOverlayParameters(pScrn);
		}

	return adapt;
}


XF86OffscreenImageRec NVOffscreenImages[2] = {
	{
		&NVImages[0],
		VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT,
		NVAllocSurface,
		NVFreeSurface,
		NVDisplaySurface,
		NVStopSurface,
		NVGetSurfaceAttribute,
		NVSetSurfaceAttribute,
		IMAGE_MAX_W, IMAGE_MAX_H,
		NUM_NV10_OVERLAY_ATTRIBUTES - 1,
		&NV10OverlayAttributes[1]
	},
	{
		&NVImages[2],
		VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT,
		NVAllocSurface,
		NVFreeSurface,
		NVDisplaySurface,
		NVStopSurface,
		NVGetSurfaceAttribute,
		NVSetSurfaceAttribute,
		IMAGE_MAX_W, IMAGE_MAX_H,
		NUM_NV10_OVERLAY_ATTRIBUTES - 1,
		&NV10OverlayAttributes[1]
	}
};

static void
NVInitOffscreenImages (ScreenPtr pScreen)
{
	xf86XVRegisterOffscreenImages(pScreen, NVOffscreenImages, 2);
}

/**
 * NVChipsetHasOverlay
 *
 * newer chips don't support overlay anymore.
 * overlay feature is emulated via textures.
 *
 * @param pNv
 * @return true, if chipset supports overlay
 */
static Bool
NVChipsetHasOverlay(NVPtr pNv)
{
	switch (pNv->Architecture) {
	case NV_ARCH_04: /*NV04 has a different overlay than NV10+*/
	case NV_ARCH_10:
	case NV_ARCH_20:
	case NV_ARCH_30:
		return TRUE;
	case NV_ARCH_40:
		if (pNv->dev->chipset == 0x40)
			return TRUE;
		break;
	default:
		break;
	}

	return FALSE;
}

/**
 * NVSetupOverlayVideo
 * check if chipset supports Overla
 * if so, setup overlay port
 *
 * @return overlay port
 * @see NVChipsetHasOverlay(NVPtr pNv)
 * @see NV10SetupOverlayVideo(ScreenPtr pScreen)
 * @see NVInitOffscreenImages(ScreenPtr pScreen)
 */
static XF86VideoAdaptorPtr
NVSetupOverlayVideo(ScreenPtr pScreen)
{
	ScrnInfoPtr          pScrn = xf86ScreenToScrn(pScreen);
	XF86VideoAdaptorPtr  overlayAdaptor = NULL;
	NVPtr                pNv   = NVPTR(pScrn);

	if (1 /*pNv->kms_enable*/ || !NVChipsetHasOverlay(pNv))
		return NULL;

	overlayAdaptor = NVSetupOverlayVideoAdapter(pScreen);
	/* I am not sure what this call does. */
	if (overlayAdaptor && pNv->Architecture != NV_ARCH_04 )
		NVInitOffscreenImages(pScreen);

	#ifdef COMPOSITE
	if (!noCompositeExtension) {
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			   "Xv: Composite is enabled, enabling overlay with "
			   "smart blitter fallback\n");
		overlayAdaptor->name = "NV Video Overlay with Composite";
	}
	#endif

	return overlayAdaptor;
}

/**
 * NV30 texture adapter.
 */

#define NUM_FORMAT_TEXTURED 2

static XF86ImageRec NV30TexturedImages[NUM_FORMAT_TEXTURED] =
{
	XVIMAGE_YV12,
	XVIMAGE_I420,
};

/**
 * NV30SetupTexturedVideo
 * this function does all the work setting up textured video port
 *
 * @return texture port
 */
static XF86VideoAdaptorPtr
NV30SetupTexturedVideo (ScreenPtr pScreen, Bool bicubic)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	NVPtr pNv = NVPTR(pScrn);
	XF86VideoAdaptorPtr adapt;
	NVPortPrivPtr pPriv;
	int i;

	if (!(adapt = calloc(1, sizeof(XF86VideoAdaptorRec) +
				 sizeof(NVPortPrivRec) +
				 (sizeof(DevUnion) * NUM_TEXTURE_PORTS)))) {
		return NULL;
	}

	adapt->type		= XvWindowMask | XvInputMask | XvImageMask;
	adapt->flags		= 0;
	if (bicubic)
		adapt->name		= "NV30 high quality adapter";
	else
		adapt->name		= "NV30 texture adapter";
	adapt->nEncodings	= 1;
	adapt->pEncodings	= &DummyEncodingTex;
	adapt->nFormats		= NUM_FORMATS_ALL;
	adapt->pFormats		= NVFormats;
	adapt->nPorts		= NUM_TEXTURE_PORTS;
	adapt->pPortPrivates	= (DevUnion*)(&adapt[1]);

	pPriv = (NVPortPrivPtr)(&adapt->pPortPrivates[NUM_TEXTURE_PORTS]);
	for(i = 0; i < NUM_TEXTURE_PORTS; i++)
		adapt->pPortPrivates[i].ptr = (pointer)(pPriv);

	adapt->pAttributes		= NVTexturedAttributes;
	adapt->nAttributes		= NUM_TEXTURED_ATTRIBUTES;
	adapt->pImages			= NV30TexturedImages;
	adapt->nImages			= NUM_FORMAT_TEXTURED;
	adapt->PutVideo			= NULL;
	adapt->PutStill			= NULL;
	adapt->GetVideo			= NULL;
	adapt->GetStill			= NULL;
	adapt->StopVideo		= NV30StopTexturedVideo;
	adapt->SetPortAttribute		= NV30SetTexturePortAttribute;
	adapt->GetPortAttribute		= NV30GetTexturePortAttribute;
	adapt->QueryBestSize		= NVQueryBestSize;
	adapt->PutImage			= NVPutImage;
	adapt->QueryImageAttributes	= NVQueryImageAttributes;

	pPriv->videoStatus		= 0;
	pPriv->grabbedByV4L		= FALSE;
	pPriv->blitter			= FALSE;
	pPriv->texture			= TRUE;
	pPriv->bicubic			= bicubic;
	pPriv->doubleBuffer		= FALSE;
	pPriv->SyncToVBlank		= TRUE;
	pPriv->max_image_dim            = 4096;

	if (bicubic)
		pNv->textureAdaptor[1]	= adapt;
	else
		pNv->textureAdaptor[0]	= adapt;

	return adapt;
}

/**
 * NV40 texture adapter.
 */

#define NUM_FORMAT_TEXTURED 2

static XF86ImageRec NV40TexturedImages[NUM_FORMAT_TEXTURED] =
{
	XVIMAGE_YV12,
	XVIMAGE_I420,
};

/**
 * NV40SetupTexturedVideo
 * this function does all the work setting up textured video port
 *
 * @return texture port
 */
static XF86VideoAdaptorPtr
NV40SetupTexturedVideo (ScreenPtr pScreen, Bool bicubic)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	NVPtr pNv = NVPTR(pScrn);
	XF86VideoAdaptorPtr adapt;
	NVPortPrivPtr pPriv;
	int i;

	if (!(adapt = calloc(1, sizeof(XF86VideoAdaptorRec) +
				 sizeof(NVPortPrivRec) +
				 (sizeof(DevUnion) * NUM_TEXTURE_PORTS)))) {
		return NULL;
	}

	adapt->type		= XvWindowMask | XvInputMask | XvImageMask;
	adapt->flags		= 0;
	if (bicubic)
		adapt->name		= "NV40 high quality adapter";
	else
		adapt->name		= "NV40 texture adapter";
	adapt->nEncodings	= 1;
	adapt->pEncodings	= &DummyEncodingTex;
	adapt->nFormats		= NUM_FORMATS_ALL;
	adapt->pFormats		= NVFormats;
	adapt->nPorts		= NUM_TEXTURE_PORTS;
	adapt->pPortPrivates	= (DevUnion*)(&adapt[1]);

	pPriv = (NVPortPrivPtr)(&adapt->pPortPrivates[NUM_TEXTURE_PORTS]);
	for(i = 0; i < NUM_TEXTURE_PORTS; i++)
		adapt->pPortPrivates[i].ptr = (pointer)(pPriv);

	adapt->pAttributes		= NVTexturedAttributes;
	adapt->nAttributes		= NUM_TEXTURED_ATTRIBUTES;
	adapt->pImages			= NV40TexturedImages;
	adapt->nImages			= NUM_FORMAT_TEXTURED;
	adapt->PutVideo			= NULL;
	adapt->PutStill			= NULL;
	adapt->GetVideo			= NULL;
	adapt->GetStill			= NULL;
	adapt->StopVideo		= NV40StopTexturedVideo;
	adapt->SetPortAttribute		= NV40SetTexturePortAttribute;
	adapt->GetPortAttribute		= NV40GetTexturePortAttribute;
	adapt->QueryBestSize		= NVQueryBestSize;
	adapt->PutImage			= NVPutImage;
	adapt->QueryImageAttributes	= NVQueryImageAttributes;

	pPriv->videoStatus		= 0;
	pPriv->grabbedByV4L		= FALSE;
	pPriv->blitter			= FALSE;
	pPriv->texture			= TRUE;
	pPriv->bicubic			= bicubic;
	pPriv->doubleBuffer		= FALSE;
	pPriv->SyncToVBlank		= TRUE;
	pPriv->max_image_dim            = 4096;

	if (bicubic)
		pNv->textureAdaptor[1]	= adapt;
	else
		pNv->textureAdaptor[0]	= adapt;

	return adapt;
}

static XF86ImageRec
NV50TexturedImages[] =
{
	XVIMAGE_YV12,
	XVIMAGE_I420,
	XVIMAGE_YUY2,
	XVIMAGE_UYVY
};

static XF86VideoAdaptorPtr
NV50SetupTexturedVideo (ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	NVPtr pNv = NVPTR(pScrn);
	XF86VideoAdaptorPtr adapt;
	NVPortPrivPtr pPriv;
	int i;

	if (!(adapt = calloc(1, sizeof(XF86VideoAdaptorRec) +
				 sizeof(NVPortPrivRec) +
				 (sizeof(DevUnion) * NUM_TEXTURE_PORTS)))) {
		return NULL;
	}

	adapt->type		= XvWindowMask | XvInputMask | XvImageMask;
	adapt->flags		= 0;
	adapt->name		= "Nouveau GeForce 8/9 Textured Video";
	adapt->nEncodings	= 1;
	adapt->pEncodings	= &DummyEncodingNV50;
	adapt->nFormats		= NUM_FORMATS_NV50;
	adapt->pFormats		= NV50Formats;
	adapt->nPorts		= NUM_TEXTURE_PORTS;
	adapt->pPortPrivates	= (DevUnion*)(&adapt[1]);

	pPriv = (NVPortPrivPtr)(&adapt->pPortPrivates[NUM_TEXTURE_PORTS]);
	for(i = 0; i < NUM_TEXTURE_PORTS; i++)
		adapt->pPortPrivates[i].ptr = (pointer)(pPriv);

	adapt->pAttributes		= NVTexturedAttributesNV50;
	adapt->nAttributes		= NUM_TEXTURED_ATTRIBUTES_NV50;
	adapt->pImages			= NV50TexturedImages;
	adapt->nImages			= sizeof(NV50TexturedImages) /
					  sizeof(NV50TexturedImages[0]);
	adapt->PutVideo			= NULL;
	adapt->PutStill			= NULL;
	adapt->GetVideo			= NULL;
	adapt->GetStill			= NULL;
	adapt->StopVideo		= nv50_xv_video_stop;
	adapt->SetPortAttribute		= nv50_xv_port_attribute_set;
	adapt->GetPortAttribute		= nv50_xv_port_attribute_get;
	adapt->QueryBestSize		= NVQueryBestSize;
	adapt->PutImage			= NVPutImage;
	adapt->QueryImageAttributes	= NVQueryImageAttributes;

	pNv->textureAdaptor[0]		= adapt;

	nv50_xv_set_port_defaults(pScrn, pPriv);
	nv50_xv_csc_update(pScrn, pPriv);

	xvBrightness = MAKE_ATOM("XV_BRIGHTNESS");
	xvContrast   = MAKE_ATOM("XV_CONTRAST");
	xvSaturation = MAKE_ATOM("XV_SATURATION");
	xvHue        = MAKE_ATOM("XV_HUE");
	xvITURBT709  = MAKE_ATOM("XV_ITURBT_709");
	return adapt;
}

static void
NVSetupTexturedVideo (ScreenPtr pScreen, XF86VideoAdaptorPtr *textureAdaptor)
{
	ScrnInfoPtr          pScrn = xf86ScreenToScrn(pScreen);
	NVPtr                pNv = NVPTR(pScrn);

	if (!pNv->Nv3D)
		return;

	if (pNv->Architecture == NV_ARCH_30) {
		textureAdaptor[0] = NV30SetupTexturedVideo(pScreen, FALSE);
		textureAdaptor[1] = NV30SetupTexturedVideo(pScreen, TRUE);
	} else
	if (pNv->Architecture == NV_ARCH_40) {
		textureAdaptor[0] = NV40SetupTexturedVideo(pScreen, FALSE);
		textureAdaptor[1] = NV40SetupTexturedVideo(pScreen, TRUE);
	} else
	if (pNv->Architecture >= NV_TESLA) {
		textureAdaptor[0] = NV50SetupTexturedVideo(pScreen);
	}
}

/**
 * NVInitVideo
 * tries to initialize the various supported adapters
 * and add them to the list of ports on screen "pScreen".
 *
 * @param pScreen
 * @see NVSetupOverlayVideo(ScreenPtr pScreen)
 * @see NVSetupBlitVideo(ScreenPtr pScreen)
 */
void
NVInitVideo(ScreenPtr pScreen)
{
	ScrnInfoPtr          pScrn = xf86ScreenToScrn(pScreen);
	NVPtr                pNv = NVPTR(pScrn);
	XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
	XF86VideoAdaptorPtr  overlayAdaptor = NULL;
	XF86VideoAdaptorPtr  blitAdaptor = NULL;
	XF86VideoAdaptorPtr  textureAdaptor[2] = {NULL, NULL};
	int                  num_adaptors;

	/*
	 * Driving the blitter requires the DMA FIFO. Using the FIFO
	 * without accel causes DMA errors. While the overlay might
	 * might work without accel, we also disable it for now when
	 * acceleration is disabled:
	 */
	if (pScrn->bitsPerPixel != 8 && pNv->AccelMethod == EXA) {
		xvSyncToVBlank = MAKE_ATOM("XV_SYNC_TO_VBLANK");

		if (pNv->Architecture < NV_TESLA) {
			overlayAdaptor = NVSetupOverlayVideo(pScreen);
			blitAdaptor    = NVSetupBlitVideo(pScreen);
		}

		NVSetupTexturedVideo(pScreen, textureAdaptor);
	}

	num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);
	if (blitAdaptor || overlayAdaptor || textureAdaptor[0]) {
		int size = num_adaptors;

		if(overlayAdaptor) size++;
		if(blitAdaptor)    size++;
		if(textureAdaptor[0]) size++;
		if(textureAdaptor[1]) size++;

		newAdaptors = malloc(size * sizeof(XF86VideoAdaptorPtr *));
		if(newAdaptors) {
			if(num_adaptors) {
				memcpy(newAdaptors, adaptors, num_adaptors *
						sizeof(XF86VideoAdaptorPtr));
			}

			if(overlayAdaptor) {
				newAdaptors[num_adaptors] = overlayAdaptor;
				num_adaptors++;
			}

			if (textureAdaptor[0]) { /* bilinear */
				newAdaptors[num_adaptors] = textureAdaptor[0];
				num_adaptors++;
			}

			if (textureAdaptor[1]) { /* bicubic */
				newAdaptors[num_adaptors] = textureAdaptor[1];
				num_adaptors++;
			}

			if(blitAdaptor) {
				newAdaptors[num_adaptors] = blitAdaptor;
				num_adaptors++;
			}

			adaptors = newAdaptors;
		}
	}

	if (num_adaptors)
		xf86XVScreenInit(pScreen, adaptors, num_adaptors);
	if (newAdaptors)
		free(newAdaptors);
	
	/*
	 * For now we associate with the plain texture adapter since it is logical, but we can
	 * associate with any/all adapters since VL doesn't depend on Xv for color conversion.
	 */
	if (textureAdaptor[0]) {
		XF86MCAdaptorPtr *adaptorsXvMC = malloc(sizeof(XF86MCAdaptorPtr));
		
		if (adaptorsXvMC) {
			adaptorsXvMC[0] = vlCreateAdaptorXvMC(pScreen, (char *)textureAdaptor[0]->name);
			
			if (adaptorsXvMC[0]) {
				vlInitXvMC(pScreen, 1, adaptorsXvMC);
				vlDestroyAdaptorXvMC(adaptorsXvMC[0]);
			}
			
			free(adaptorsXvMC);
		}
	}
}

void
NVTakedownVideo(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);

	if (pNv->blitAdaptor)
		NVFreePortMemory(pScrn, GET_BLIT_PRIVATE(pNv));
	if (pNv->textureAdaptor[0]) {
		NVFreePortMemory(pScrn,
				 pNv->textureAdaptor[0]->pPortPrivates[0].ptr);
	}
	if (pNv->textureAdaptor[1]) {
		NVFreePortMemory(pScrn,
				 pNv->textureAdaptor[1]->pPortPrivates[0].ptr);
	}
}
#endif

/* The filtering function used for video scaling. We use a cubic filter as
 * defined in  "Reconstruction Filters in Computer Graphics" Mitchell &
 * Netravali in SIGGRAPH '88
 */
static float filter_func(float x)
{
	const double B=0.75;
	const double C=(1.0-B)/2.0;
	double x1=fabs(x);
	double x2=fabs(x)*x1;
	double x3=fabs(x)*x2;

	if (fabs(x)<1.0)
		return ( (12.0-9.0*B-6.0*C)*x3+(-18.0+12.0*B+6.0*C)*x2+(6.0-2.0*B) )/6.0; 
	else
		return ( (-B-6.0*C)*x3+(6.0*B+30.0*C)*x2+(-12.0*B-48.0*C)*x1+(8.0*B+24.0*C) )/6.0;
}

static int8_t f32tosb8(float v)
{
	return (int8_t)(v*127.0);
}

void
NVXVComputeBicubicFilter(struct nouveau_bo *bo, unsigned offset, unsigned size)
{
	int8_t *t = (int8_t *)(bo->map + offset);
	int i;

	for(i = 0; i < size; i++) {
		float  x = (i + 0.5) / size;
		float w0 = filter_func(x+1.0);
		float w1 = filter_func(x);
		float w2 = filter_func(x-1.0);
		float w3 = filter_func(x-2.0);

		t[4*i+2]=f32tosb8(1.0+x-w1/(w0+w1));
		t[4*i+1]=f32tosb8(1.0-x+w3/(w2+w3));
		t[4*i+0]=f32tosb8(w0+w1);
		t[4*i+3]=f32tosb8(0.0);
	}
}
