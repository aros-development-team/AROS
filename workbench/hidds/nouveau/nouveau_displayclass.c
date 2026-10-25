/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#include "nouveau_intern.h"

#include <graphics/displayinfo.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>

#include <libdrm/arosdrm.h>
#include <libdrm/arosdrmmode.h>
#include <uapi/drm/drm_fourcc.h>
#include <string.h>

#undef HiddAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddDisplayAttrBase
#undef HiddDMEnumAttrBase
#undef HiddGfxNouveauAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddBitMapNouveauAttrBase

#define HiddAttrBase          (SD(cl)->hiddAttrBase)
#define HiddPixFmtAttrBase          (SD(cl)->pixFmtAttrBase)
#define HiddGfxAttrBase             (SD(cl)->gfxAttrBase)
#define HiddDisplayAttrBase         (SD(cl)->displayAttrBase)
#define HiddDMEnumAttrBase          (SD(cl)->dmenumAttrBase)
#define HiddGfxNouveauAttrBase      (SD(cl)->gfxNouveauAttrBase)
#define HiddSyncAttrBase            (SD(cl)->syncAttrBase)
#define HiddBitMapAttrBase          (SD(cl)->bitMapAttrBase)
#define HiddBitMapNouveauAttrBase   (SD(cl)->bitMapNouveauAttrBase)

/* HELPER FUNCTIONS */
VOID HIDDNouveauShowCursor(OOP_Object * gfx, BOOL visible)
{
    OOP_Class * cl = OOP_OCLASS(gfx);
    struct HIDDNouveauData * gfxdata = OOP_INST_DATA(cl, gfx);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;

    LOCK_ENGINE

    if (visible)
    {
        drmModeSetCursor(NOUVEAU_DEV_FD(nvdev), gfxdata->selectedcrtcid, 
            gfxdata->cursor->handle, 64, 64);
    }
    else
    {
        drmModeSetCursor(NOUVEAU_DEV_FD(nvdev), gfxdata->selectedcrtcid, 
            0, 64, 64);
    }

    UNLOCK_ENGINE
}

/* This function assumes that the mode, crtc and output are already selected */
static BOOL HIDDNouveauShowBitmapForSelectedMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauData * gfxdata = NULL;
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;
    uint32_t output_ids[] = {0};
    uint32_t output_count = 1;
    IPTR e = (IPTR)NULL;
    OOP_Object * gfx = NULL;
    LONG ret;

    LOCK_ENGINE
    LOCK_BITMAP
    
    /* Check if passed bitmap has been registered as framebuffer */
    if (bmdata->fbid == 0)
    {
        UNLOCK_BITMAP
        UNLOCK_ENGINE
        return FALSE;
    }
    
    OOP_GetAttr(bm, aHidd_BitMap_Display, &e);
    OOP_GetAttr((OOP_Object *)e, aHidd_Display_GfxHidd, &e);
    gfx = (OOP_Object *)e;
    gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    output_ids[0] = ((drmModeConnectorPtr)gfxdata->selectedconnector)->connector_id;
    

    ret = drmModeSetCrtc(NOUVEAU_DEV_FD(nvdev), gfxdata->selectedcrtcid,
            bmdata->fbid, -bmdata->xoffset, -bmdata->yoffset, output_ids,
            output_count, gfxdata->selectedmode);
    nvlog("[Nouveau] setcrtc crtc %lu fb %lu conn %lu mode %s (%dx%d@%d) offset %ld,%ld pitch %ld bpp %ld: %ld\n",
        (unsigned long)gfxdata->selectedcrtcid, (unsigned long)bmdata->fbid, (unsigned long)output_ids[0],
        ((drmModeModeInfoPtr)gfxdata->selectedmode)->name, ((drmModeModeInfoPtr)gfxdata->selectedmode)->hdisplay,
        ((drmModeModeInfoPtr)gfxdata->selectedmode)->vdisplay, ((drmModeModeInfoPtr)gfxdata->selectedmode)->vrefresh,
        (long)-bmdata->xoffset, (long)-bmdata->yoffset, (long)bmdata->pitch, (long)bmdata->bytesperpixel * 8, (long)ret);

    UNLOCK_BITMAP
    UNLOCK_ENGINE

    if (ret) return FALSE; else return TRUE;
}

/*
 * Register a buffer object of the bitmap as a KMS framebuffer (once).
 * Needed to scan it out, on the crtc or on an overlay plane; Dispose()
 * unregisters it. The fourcc follows the bitmap: XRGB8888 for the 32-bit
 * RGB layouts (the pad byte is ignored, the plane blends opaque), RGB565 /
 * XRGB1555 for 16-bit, YUYV / UYVY for the packed video formats.
 */
static ULONG HIDDNouveauBitMapFourCC(struct HIDDNouveauBitMapData *bmdata)
{
    switch (bmdata->videoformat)
    {
    case vHidd_VideoFmt_YUYV:   return DRM_FORMAT_YUYV;
    case vHidd_VideoFmt_UYVY:   return DRM_FORMAT_UYVY;
    case vHidd_VideoFmt_NV12:   return DRM_FORMAT_NV12;
    case vHidd_VideoFmt_I420:   return DRM_FORMAT_YUV420;
    case vHidd_VideoFmt_None:   break;
    default:                    return 0;
    }
    switch (bmdata->stdfmt)
    {
    case vHidd_StdPixFmt_BGR032:
    case vHidd_StdPixFmt_BGRA32:    return DRM_FORMAT_XRGB8888;
    case vHidd_StdPixFmt_RGB032:
    case vHidd_StdPixFmt_RGBA32:    return DRM_FORMAT_XBGR8888;
    case vHidd_StdPixFmt_RGB16_LE:  return DRM_FORMAT_RGB565;
    case vHidd_StdPixFmt_RGB15_LE:  return DRM_FORMAT_XRGB1555;
    }
    return (bmdata->bytesperpixel == 4) ? DRM_FORMAT_XRGB8888 :
           (bmdata->bytesperpixel == 2) ? DRM_FORMAT_RGB565 : 0;
}

static BOOL HIDDNouveauAddFB(OOP_Class *cl, struct HIDDNouveauBitMapData *bmdata,
    struct nouveau_bo *bo, ULONG *fbid)
{
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;
    ULONG fourcc = HIDDNouveauBitMapFourCC(bmdata);
    uint32_t handles[4] = { bo->handle, 0, 0, 0 };
    uint32_t pitches[4] = { bmdata->pitch, 0, 0, 0 };
    uint32_t offsets[4] = { 0, 0, 0, 0 };
    LONG ret = -1;

    /* Planar video: the chroma planes are in the same buffer object */
    if (bmdata->videoformat == vHidd_VideoFmt_NV12)
    {
        handles[1] = bo->handle;
        pitches[1] = bmdata->pitch;
        offsets[1] = bmdata->planeoff[0];
    }
    else if (bmdata->videoformat == vHidd_VideoFmt_I420)
    {
        handles[1] = handles[2] = bo->handle;
        pitches[1] = pitches[2] = bmdata->pitch / 2;
        offsets[1] = bmdata->planeoff[0];
        offsets[2] = bmdata->planeoff[1];
    }

    LOCK_ENGINE
    if (fourcc)
        ret = drmModeAddFB2(NOUVEAU_DEV_FD(nvdev), bmdata->drawable.width, bmdata->drawable.height,
                    fourcc, handles, pitches, offsets, fbid, 0);
    if (ret && !bmdata->videoformat)
    {
        /* the legacy call derives the format from depth/bpp as before */
        ret = drmModeAddFB(NOUVEAU_DEV_FD(nvdev), bmdata->drawable.width, bmdata->drawable.height,
                    bmdata->drawable.depth, bmdata->bytesperpixel * 8,
                    bmdata->pitch, bo->handle, fbid);
    }
    UNLOCK_ENGINE
    if (ret)
    {
        nvlog("[Nouveau] Not able to add framebuffer (%lux%lu fourcc %08lx pitch %lu), %ld\n",
            (unsigned long)bmdata->drawable.width, (unsigned long)bmdata->drawable.height,
            (unsigned long)fourcc, (unsigned long)bmdata->pitch, (long)ret);
        *fbid = 0;
        return FALSE;
    }
    return TRUE;
}

BOOL HIDDNouveauEnsureFB(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);

    if (bmdata->fbid != 0)
        return TRUE;
    if (!bmdata->bo)
        return FALSE;
    if (!HIDDNouveauAddFB(cl, bmdata, bmdata->bo, &bmdata->fbid))
        return FALSE;
    if (bmdata->nbufs)
        bmdata->buf[bmdata->front].fbid = bmdata->fbid;
    return TRUE;
}

/* The framebuffer of one buffer of a scan-out bitmap (SwapBuffers) */
BOOL HIDDNouveauEnsureBufferFB(OOP_Object * bm, ULONG buffer)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);

    if (buffer >= bmdata->nbufs)
        return FALSE;
    if (buffer == bmdata->front)
        return HIDDNouveauEnsureFB(bm);
    if (bmdata->buf[buffer].fbid)
        return TRUE;
    return HIDDNouveauAddFB(cl, bmdata, bmdata->buf[buffer].bo, &bmdata->buf[buffer].fbid);
}

/*
 * The crtc's overlay planes that scan out RGB, and whether the kernel counts
 * its vblanks. Without the universal-planes capability the kernel lists the
 * overlays only: the primary is driven by the mode set, the cursor by the
 * cursor calls. nv04 overlays take YUV only and are therefore left out.
 */
/* The DRM layer returns Linux errno values; AROS's own differ for some. */
#define KMS_EBUSY       16
#define KMS_ENOSYS      38
#define KMS_EOPNOTSUPP  95
#define KMS_ETIMEDOUT   110

/*
 * The plane's DRM properties: "type" (0 overlay, 1 primary, 2 cursor, as
 * the DRM core's enum drm_plane_type) decides whether it is offered - the
 * AROS drm file has universal planes on, so the list also holds each crtc's
 * primary and cursor - and the ids of the atomic properties (FB_ID, CRTC_*,
 * SRC_*), "zpos" with its range and nouveau's "colorkey" are kept for the
 * atomic requests of CommitPlanes. Returns the type, -1 if unreadable.
 */
static LONG HIDDNouveauPlaneProps(LONG fd, struct HIDDNouveauPlane *plane)
{
    drmModeObjectPropertiesPtr props;
    LONG type = -1;
    ULONG i;

    props = drmModeObjectGetProperties(fd, plane->id, DRM_MODE_OBJECT_PLANE);
    if (!props)
        return -1;
    for (i = 0; i < props->count_props; i++)
    {
        drmModePropertyPtr prop = drmModeGetProperty(fd, props->props[i]);

        if (!prop)
            continue;
        if (!strcmp(prop->name, "type"))
            type = (LONG)props->prop_values[i];
        else if (!strcmp(prop->name, "FB_ID"))   plane->prop_fb_id   = prop->prop_id;
        else if (!strcmp(prop->name, "CRTC_ID")) plane->prop_crtc_id = prop->prop_id;
        else if (!strcmp(prop->name, "CRTC_X"))  plane->prop_crtc_x  = prop->prop_id;
        else if (!strcmp(prop->name, "CRTC_Y"))  plane->prop_crtc_y  = prop->prop_id;
        else if (!strcmp(prop->name, "CRTC_W"))  plane->prop_crtc_w  = prop->prop_id;
        else if (!strcmp(prop->name, "CRTC_H"))  plane->prop_crtc_h  = prop->prop_id;
        else if (!strcmp(prop->name, "SRC_X"))   plane->prop_src_x   = prop->prop_id;
        else if (!strcmp(prop->name, "SRC_Y"))   plane->prop_src_y   = prop->prop_id;
        else if (!strcmp(prop->name, "SRC_W"))   plane->prop_src_w   = prop->prop_id;
        else if (!strcmp(prop->name, "SRC_H"))   plane->prop_src_h   = prop->prop_id;
        else if (!strcmp(prop->name, "zpos"))
        {
            plane->prop_zpos = prop->prop_id;
            plane->zpos_mutable = !(prop->flags & DRM_MODE_PROP_IMMUTABLE);
            plane->zpos = (ULONG)props->prop_values[i];
            if ((prop->flags & DRM_MODE_PROP_RANGE) && (prop->count_values >= 2))
            {
                plane->zmin = (ULONG)prop->values[0];
                plane->zmax = (ULONG)prop->values[1];
            }
            else
                plane->zmin = plane->zmax = plane->zpos;
        }
        else if (!strcmp(prop->name, "colorkey"))
        {
            plane->prop_ckey = prop->prop_id;
            plane->ckey = (ULONG)props->prop_values[i];
        }
        drmModeFreeProperty(prop);
    }
    drmModeFreeObjectProperties(props);
    return type;
}

static ULONG HIDDNouveauPlaneVideoFmts(struct HIDDNouveauData *gfxdata, ULONG fmts);

static BOOL HIDDNouveauPlaneAtomic(struct HIDDNouveauPlane *p)
{
    return p->prop_fb_id && p->prop_crtc_id &&
           p->prop_crtc_x && p->prop_crtc_y && p->prop_crtc_w && p->prop_crtc_h &&
           p->prop_src_x && p->prop_src_y && p->prop_src_w && p->prop_src_h;
}

VOID HIDDNouveauEnumPlanes(LONG fd, struct HIDDNouveauData *gfxdata)
{
    extern int nouveau_plane_atomic;
    drmModeResPtr res;
    drmModePlaneResPtr pres;
    ULONG i;

    gfxdata->nplanes = 0;
    gfxdata->selectedcrtcindex = 0;
    gfxdata->atomic = nouveau_plane_atomic ? TRUE : FALSE;
    gfxdata->maxfbw = gfxdata->maxfbh = 0;
    LOCK_ENGINE
    res = drmModeGetResources(fd);
    if (res)
    {
        for (i = 0; i < res->count_crtcs; i++)
        {
            if (res->crtcs[i] == gfxdata->selectedcrtcid)
                gfxdata->selectedcrtcindex = i;
        }
        gfxdata->maxfbw = res->max_width;
        gfxdata->maxfbh = res->max_height;
        drmModeFreeResources(res);
    }
    pres = drmModeGetPlaneResources(fd);
    if (pres)
    {
        for (i = 0; (i < pres->count_planes) && (gfxdata->nplanes < NOUVEAU_MAX_PLANES); i++)
        {
            drmModePlanePtr p = drmModeGetPlane(fd, pres->planes[i]);
            struct HIDDNouveauPlane *plane = &gfxdata->planes[gfxdata->nplanes];
            ULONG f, fmts = 0;

            if (!p)
                continue;
            memset(plane, 0, sizeof(*plane));
            plane->id = p->plane_id;
            if ((p->possible_crtcs & (1UL << gfxdata->selectedcrtcindex)) &&
                (HIDDNouveauPlaneProps(fd, plane) == 0 /* overlay */))
            {
                for (f = 0; f < p->count_formats; f++)
                {
                    switch (p->formats[f])
                    {
                    case DRM_FORMAT_XRGB8888: fmts |= KMSFMT_XRGB8888; break;
                    case DRM_FORMAT_ARGB8888: fmts |= KMSFMT_ARGB8888; break;
                    case DRM_FORMAT_XBGR8888: fmts |= KMSFMT_XBGR8888; break;
                    case DRM_FORMAT_ABGR8888: fmts |= KMSFMT_ABGR8888; break;
                    case DRM_FORMAT_RGB565:   fmts |= KMSFMT_RGB565;   break;
                    case DRM_FORMAT_XRGB1555: fmts |= KMSFMT_XRGB1555; break;
                    case DRM_FORMAT_YUYV:     fmts |= KMSFMT_YUYV;     break;
                    case DRM_FORMAT_UYVY:     fmts |= KMSFMT_UYVY;     break;
                    case DRM_FORMAT_NV12:     fmts |= KMSFMT_NV12;     break;
                    case DRM_FORMAT_YUV420:   fmts |= KMSFMT_YUV420;   break;
                    }
                }
                if (fmts & ~KMSFMT_VIDEO)
                {
                    plane->formats = fmts;
                    plane->fbid    = 0;
                    if (!HIDDNouveauPlaneAtomic(plane))
                        gfxdata->atomic = FALSE;
                    nvlog("[Nouveau] plane %lu: formats %03lx (video mask %02lx) zpos %lu..%lu%s%s%s\n",
                        (unsigned long)plane->id, (unsigned long)fmts,
                        (unsigned long)HIDDNouveauPlaneVideoFmts(gfxdata, fmts),
                        (unsigned long)plane->zmin, (unsigned long)plane->zmax,
                        plane->zpos_mutable ? "" : " (fixed)",
                        plane->prop_ckey ? ", colorkey" : "",
                        HIDDNouveauPlaneAtomic(plane) ? ", atomic" : ", legacy");
                    gfxdata->nplanes++;
                }
            }
            drmModeFreePlane(p);
        }
        drmModeFreePlaneResources(pres);
    }
    UNLOCK_ENGINE

    /*
     * Vblank is decided on first use, not here: before the crtc's first
     * modeset the DRM core has no timing for it (a query then only draws a
     * warning). WaitVBlank retries -EINVAL (vblank not on yet) and turns
     * vblank off for good only when a wait times out (no interrupt).
     */
    {
        extern int nouveau_vblank_param;

        gfxdata->vblank_ok = nouveau_vblank_param ? TRUE : FALSE;
    }
    gfxdata->vblanks = 0;
    nvlog("[Nouveau] crtc %lu: %lu overlay plane(s), %s commits, vblank %s\n",
        (unsigned long)gfxdata->selectedcrtcid, (unsigned long)gfxdata->nplanes,
        gfxdata->atomic ? "atomic" : "legacy",
        gfxdata->vblank_ok ? "checked on first use" : "off (NOUVEAU_VBLANK=0)");
}

static BOOL HIDDNouveauWaitVBlank(LONG fd, struct HIDDNouveauData *gfxdata)
{
    drmVBlank vbl;
    int ret;

    if (!gfxdata->vblank_ok)
        return FALSE;
    memset(&vbl, 0, sizeof(vbl));
    vbl.request.type = DRM_VBLANK_RELATIVE | ((gfxdata->selectedcrtcindex << DRM_VBLANK_HIGH_CRTC_SHIFT) & DRM_VBLANK_HIGH_CRTC_MASK);
    vbl.request.sequence = 1;
    /* No engine lock: the DRM core's vblank wait sleeps up to a frame and
       has its own locking; holding the lock would stall all drawing. */
    ret = drmWaitVBlank(fd, &vbl);
    if (ret != 0)
    {
        /* -EINVAL: vblank not on (crtc off or mid-modeset), try again later.
           A timeout means no vblank interrupt arrives: stop waiting for good. */
        if ((ret == -KMS_EBUSY) || (ret == -KMS_ETIMEDOUT))
        {
            nvlog("[Nouveau] crtc %lu: vblank wait timed out (%d), vblank disabled\n",
                (unsigned long)gfxdata->selectedcrtcid, ret);
            gfxdata->vblank_ok = FALSE;
        }
        return FALSE;
    }
    {
        static BOOL first = TRUE;

        if (first)
        {
            first = FALSE;
            nvlog("[Nouveau] crtc %lu: first vblank wait completed (sequence %lu)\n",
                (unsigned long)gfxdata->selectedcrtcid, (unsigned long)vbl.reply.sequence);
        }
    }
    gfxdata->vblanks = vbl.reply.sequence;
    return TRUE;
}

static ULONG HIDDNouveauVBlankCount(LONG fd, struct HIDDNouveauData *gfxdata)
{
    drmVBlank vbl;

    if (!gfxdata->vblank_ok)
        return gfxdata->vblanks;
    memset(&vbl, 0, sizeof(vbl));
    vbl.request.type = DRM_VBLANK_RELATIVE | ((gfxdata->selectedcrtcindex << DRM_VBLANK_HIGH_CRTC_SHIFT) & DRM_VBLANK_HIGH_CRTC_MASK);
    vbl.request.sequence = 0;
    if (drmWaitVBlank(fd, &vbl) == 0)
        gfxdata->vblanks = vbl.reply.sequence;
    return gfxdata->vblanks;
}

/* vHidd_StdPixFmt_* bits for the DRM formats a plane takes (memory byte order) */
static ULONG HIDDNouveauPlanePixFmts(ULONG fmts)
{
    ULONG m = 0;

    if (fmts & KMSFMT_XRGB8888) m |= (1UL << vHidd_StdPixFmt_BGR032) | (1UL << vHidd_StdPixFmt_BGRA32);
    if (fmts & KMSFMT_ARGB8888) m |= (1UL << vHidd_StdPixFmt_BGRA32);
    if (fmts & KMSFMT_XBGR8888) m |= (1UL << vHidd_StdPixFmt_RGB032) | (1UL << vHidd_StdPixFmt_RGBA32);
    if (fmts & KMSFMT_ABGR8888) m |= (1UL << vHidd_StdPixFmt_RGBA32);
    if (fmts & KMSFMT_RGB565)   m |= (1UL << vHidd_StdPixFmt_RGB16_LE);
    if (fmts & KMSFMT_XRGB1555) m |= (1UL << vHidd_StdPixFmt_RGB15_LE);
    return m;
}

/*
 * vHidd_VideoFmt_* bits for the video formats a plane really scans out. The
 * DRM format list of the Volta+ window classes names more than the hardware
 * generations program (nvkms): Volta (C37E) no YUV at all, Turing (C57E)
 * semi-planar NV12, Ampere and later (C67E, CA7E) planar I420 as well.
 * Packed 4:2:2 on a window raised an UPDATE exception on the GB207 and is
 * offered only with NOUVEAU_WNDW_PACKED422=1; the nv50-style overlay planes
 * (pre-Volta) keep whatever they list, as upstream always used.
 */
static ULONG HIDDNouveauPlaneVideoFmts(struct HIDDNouveauData *gfxdata, ULONG fmts)
{
    extern unsigned int nouveau_wndw_yuv_mask(void);
    BOOL window = gfxdata->chipset >= 0x140;
    ULONG m = 0;

    /* Window planes (Volta+): one rule for what the windows may take (dispnv50/wndw.c) */
    if (window)
    {
        ULONG yuv = nouveau_wndw_yuv_mask();

        if (!(yuv & 0x1))
            fmts &= ~(KMSFMT_YUYV | KMSFMT_UYVY);
        if (!(yuv & 0x2))
            fmts &= ~KMSFMT_NV12;
        if (!(yuv & 0x4))
            fmts &= ~KMSFMT_YUV420;
    }
    if (fmts & KMSFMT_YUYV)   m |= (1UL << vHidd_VideoFmt_YUYV);
    if (fmts & KMSFMT_UYVY)   m |= (1UL << vHidd_VideoFmt_UYVY);
    if (fmts & KMSFMT_NV12)   m |= (1UL << vHidd_VideoFmt_NV12);
    if (fmts & KMSFMT_YUV420) m |= (1UL << vHidd_VideoFmt_I420);
    return m;
}

/* Whether an overlay plane scans out the bitmap's format */
static BOOL HIDDNouveauPlaneTakes(struct HIDDNouveauData *gfxdata, struct HIDDNouveauPlane *p,
    struct HIDDNouveauBitMapData *bmdata)
{
    if (p->dead)
        return FALSE;
    if (bmdata->videoformat)
        return (HIDDNouveauPlaneVideoFmts(gfxdata, p->formats) & (1UL << bmdata->videoformat)) != 0;
    if (bmdata->stdfmt < 32)
        return (HIDDNouveauPlanePixFmts(p->formats) & (1UL << bmdata->stdfmt)) != 0;
    return FALSE;
}

/* The plane supports destination colour keying and it is not switched off */
static BOOL HIDDNouveauPlaneKeys(struct HIDDNouveauPlane *p)
{
    extern int nouveau_colorkey_param;

    return p->prop_ckey && nouveau_colorkey_param;
}

static struct HIDDNouveauData *HIDDNouveauGfxData(OOP_Class *cl, OOP_Object *o)
{
    OOP_Object *gfx = NULL;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    return gfx ? OOP_INST_DATA(SD(cl)->gfxclass, gfx) : NULL;
}

BOOL HIDDNouveauSwitchToVideoMode(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);

    /* Shutting down for a reboot: the mode that is up stays up. */
    if (nouveau_shutting_down)
        return TRUE;
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    OOP_Object * gfx = NULL;
    struct HIDDNouveauData * gfxdata = NULL; 
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;
    LONG i;
    drmModeConnectorPtr selectedconnector = NULL;
    HIDDT_ModeID modeid;
    OOP_Object * sync;
    OOP_Object * pf;
    IPTR pixel, e;
    IPTR hdisp, vdisp, hstart, hend, htotal, vstart, vend, vtotal;
    LONG ret;

    LOCK_ENGINE

    OOP_GetAttr(bm, aHidd_BitMap_Display, &e);
    OOP_GetAttr((OOP_Object *)e, aHidd_Display_GfxHidd, &e);
    gfx = (OOP_Object *)e;
    gfxdata = OOP_INST_DATA(OOP_OCLASS(gfx), gfx);
    selectedconnector = (drmModeConnectorPtr)gfxdata->selectedconnector;

    D(bug("[Nouveau] HIDDNouveauSwitchToVideoMode, bm: 0x%x\n", bm));
    
    /* We should be able to get modeID from the bitmap */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);

    if (modeid == vHidd_ModeID_Invalid)
    {
        nvlog("[Nouveau] SwitchToVideoMode: bitmap %p has no ModeID\n", bm);
        UNLOCK_ENGINE
        return FALSE;
    }

    /* Get Sync and PixelFormat properties */
    HIDD_DMEnum_GetMode(SD(cl)->dmenum, modeid, &sync, &pf);

    OOP_GetAttr(sync, aHidd_Sync_PixelClock,    &pixel);
    OOP_GetAttr(sync, aHidd_Sync_HDisp,         &hdisp);
    OOP_GetAttr(sync, aHidd_Sync_VDisp,         &vdisp);
    OOP_GetAttr(sync, aHidd_Sync_HSyncStart,    &hstart);
    OOP_GetAttr(sync, aHidd_Sync_VSyncStart,    &vstart);
    OOP_GetAttr(sync, aHidd_Sync_HSyncEnd,      &hend);
    OOP_GetAttr(sync, aHidd_Sync_VSyncEnd,      &vend);
    OOP_GetAttr(sync, aHidd_Sync_HTotal,        &htotal);
    OOP_GetAttr(sync, aHidd_Sync_VTotal,        &vtotal);    
    
    nvlog("[Nouveau] SwitchToVideoMode: modeid %08lx sync %ld kHz %ldx%ld (%ld %ld %ld / %ld %ld %ld)\n",
        (unsigned long)modeid, (long)pixel, (long)hdisp, (long)vdisp, (long)hstart, (long)hend, (long)htotal,
        (long)vstart, (long)vend, (long)vtotal);

    D(bug("[Nouveau] Connector %d, CRTC %d\n", 
        selectedconnector->connector_id, gfxdata->selectedcrtcid));

    /* Select mode */
    gfxdata->selectedmode = NULL;
    for (i = 0; i < selectedconnector->count_modes; i++)
    {
        drmModeModeInfoPtr mode = &selectedconnector->modes[i];
        
        if ((mode->hdisplay == hdisp) && (mode->vdisplay == vdisp) &&
            (mode->hsync_start == hstart) && (mode->vsync_start == vstart) &&
            (mode->hsync_end == hend) && (mode->vsync_end == vend))
        {
            gfxdata->selectedmode = mode;
            break;
        }
    }
    
    if (!gfxdata->selectedmode)
    {
        nvlog("[Nouveau] SwitchToVideoMode: no connector mode matches\n");
        UNLOCK_ENGINE
        return FALSE;
    }

    /* For screen switching the bitmap might have already once been a framebuffer 
       - check bmdata->fbid. Also the bitmap itself needs to know whether it is 
       added as framebuffer so that it can unregister itself in Dispose */

    /* Add as frame buffer */
    if (!HIDDNouveauEnsureFB(bm))
    {
        UNLOCK_ENGINE
        return FALSE;
    }


    /* Switch mode */
    if (!HIDDNouveauShowBitmapForSelectedMode(bm))
    {
        nvlog("[Nouveau] SwitchToVideoMode: setcrtc failed\n");
        UNLOCK_ENGINE
        return FALSE;        
    }

    HIDDNouveauShowCursor(gfx, TRUE);

    UNLOCK_ENGINE
    return TRUE;
}

/*
 * What the monitor calls itself, asked for once and remembered. A display
 * that leaves the name descriptor out of its EDID has none to give, and
 * then the base class answers instead of us inventing one.
 */
static CONST_STRPTR HIDDNouveauDisplayName(OOP_Class * cl, OOP_Object * o)
{
    struct HIDDNouveauDisplayData * data = OOP_INST_DATA(cl, o);

    if (!data->nameasked)
    {
        struct CardData * carddata = &(SD(cl)->carddata);
        OOP_Object * gfx = NULL;

        data->nameasked = TRUE;

        OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
        if (gfx && carddata->dev)
        {
            struct HIDDNouveauData * gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);
            drmModeConnectorPtr connector =
                (drmModeConnectorPtr)gfxdata->selectedconnector;

            if (connector)
            {
                LOCK_ENGINE
                drmGetMonitorName(NOUVEAU_DEV_FD(carddata->dev), connector->connector_id,
                    data->name, sizeof(data->name));
                UNLOCK_ENGINE

                D(bug("[Nouveau] display names itself '%s'\n", data->name));
            }
        }
    }

    return data->name[0] ? (CONST_STRPTR)data->name : NULL;
}

/* PUBLIC METHODS */
VOID METHOD(NouveauDisplay, Root, Get)
{
    ULONG idx;

    Hidd_Switch(msg->attrID, idx)
    {
    case aoHidd_Name:
        {
            CONST_STRPTR name = HIDDNouveauDisplayName(cl, o);

            if (name)
            {
                *msg->storage = (IPTR)name;
                return;
            }
        }
        break;
    }

    Hidd_Display_Switch(msg->attrID, idx)
    {
    case aoHidd_Display_SpriteTypes:
        *msg->storage = vHidd_SpriteType_DirectColor;
        return;
    case aoHidd_Display_PlaneCount:
        {
            /* primary + overlays + the hardware cursor */
            struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, o);
            *msg->storage = 1 + (gfxdata ? gfxdata->nplanes : 0) + 1;
        }
        return;
    case aoHidd_Display_VBlankCount:
        {
            struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, o);
            *msg->storage = gfxdata ? HIDDNouveauVBlankCount(NOUVEAU_DEV_FD(SD(cl)->carddata.dev), gfxdata) : 0;
        }
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object * METHOD(NouveauDisplay, Hidd_Display, CreateObject)
{
    OOP_Object      *object = NULL;

    if (msg->cl == SD(cl)->basebm)
    {
        struct pHidd_Display_CreateObject mymsg;
        HIDDT_ModeID modeid;
        HIDDT_StdPixFmt stdpf;

        struct TagItem mytags [] =
        {
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_Align */
            { TAG_MORE, (IPTR)msg->attrList }
        };

        /* Check if user provided valid ModeID */
        /* Check for framebuffer - not needed as Nouveau is a NoFramebuffer driver */
        /* Check for displayable - not needed - displayable has ModeID and we don't
           distinguish between on-screen and off-screen bitmaps */
        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        if ((vHidd_ModeID_Invalid != modeid) ||
            GetTagData(aHidd_BitMap_Scanout, FALSE, msg->attrList) ||
            GetTagData(aHidd_BitMap_VideoFormat, vHidd_VideoFmt_None, msg->attrList))
        {
            /* User supplied a valid modeid, or wants a scan-out (plane) bitmap:
               our bitmap class, which allocates in video RAM */
            mytags[0].ti_Tag	= aHidd_BitMap_ClassPtr;
            mytags[0].ti_Data	= (IPTR)SD(cl)->bmclass;
        } 

        /* Check if bitmap is a planar bitmap */
        stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
        if (vHidd_StdPixFmt_Plane == stdpf)
        {
            mytags[1].ti_Tag    = aHidd_BitMap_Align;
            mytags[1].ti_Data   = 32;
        }
        
        /* We init a new message struct */
        mymsg.mID	= msg->mID;
        mymsg.cl	= msg->cl;
        mymsg.attrList	= mytags;

        /* Pass the new message to the superclass */
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    }
    else if (SD(cl)->basegallium && (msg->cl == SD(cl)->basegallium))
    {
        /* Create the gallium 3d driver object .. */
        object = OOP_NewObject(NULL, CLID_Hidd_Gallium_Nouveau, msg->attrList);
    }
    else if (SD(cl)->basei2c && (msg->cl == SD(cl)->basei2c))
    {
        /* Expose the i2c bus object .. */
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return object;
}

ULONG METHOD(NouveauDisplay, Hidd_Display, ShowViewPorts)
{
    /* Composition is the display's compositor's job; we only scan out */
    return FALSE;
}

/*
 * Scan out the given bitmap. The display's compositor calls this with
 * the bitmap it composes into, or with the top screen's bitmap when that
 * covers the whole display. Nothing is mirrored: the bitmap's own buffer
 * object becomes the CRTC's framebuffer.
 */
OOP_Object *METHOD(NouveauDisplay, Hidd_Display, Show)
{
    if (msg->bitMap)
    {
        nvlog("[Nouveau] Show bitmap %p\n", msg->bitMap);

        if (!HIDDNouveauSwitchToVideoMode(msg->bitMap))
            return NULL;
    }

    /* Base class tracks the shown bitmap as the pointer target */
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

#if AROS_BIG_ENDIAN
#define Machine_ARGB32 vHidd_StdPixFmt_ARGB32
#else
#define Machine_ARGB32 vHidd_StdPixFmt_BGRA32
#endif

BOOL METHOD(NouveauDisplay, Hidd_Display, SetCursorShape)
{
    OOP_Object *gfx = NULL;
    struct HIDDNouveauData * gfxdata;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);

    if (msg->shape == NULL)
    {
        /* Hide cursor */
        HIDDNouveauShowCursor(gfx, FALSE);
    }
    else
    {
        IPTR width, height;
        ULONG i;
        ULONG x, y;
        ULONG curimage[64 * 64];
        struct CardData * carddata = &(SD(cl)->carddata);
        
        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);


        if (width > 64) width = 64;
        if (height > 64) height = 64;

        LOCK_ENGINE

        /* Map the cursor buffer */
        nouveau_bo_map(gfxdata->cursor, NOUVEAU_BO_WR, carddata->client);

        /* Clear the matrix */
        for (i = 0; i < 64 * 64; i++)
            ((ULONG*)gfxdata->cursor->map)[i] = 0;

        /* Get data from the bitmap */
        HIDD_BM_GetImage(msg->shape, (UBYTE *)curimage, 64 * 4, 0, 0, 
            width, height, Machine_ARGB32);
        
        if (carddata->Architecture < NV_TESLA)
        {
            ULONG offset, pixel, blue, green, red, alpha;

            /* The image needs to be premultiplied */
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                {
                    offset = y * 64 + x;
                    pixel = curimage[offset];
                    blue  = (pixel & 0x000000FF);
                    green = (pixel & 0x0000FF00) >> 8;
                    red   = (pixel & 0x00FF0000) >> 16;
                    alpha = (pixel & 0xFF000000) >> 24;
                    
                    blue    = (blue * alpha) / 255;
                    green   = (green * alpha) / 255;
                    red     = (red * alpha) / 255;

                    curimage[offset]    = (alpha << 24) | (red << 16) | (green << 8) | blue;
                }
        }

        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                ULONG offset = y * 64 + x;
                hidd_writel(curimage[offset], ((ULONG *)gfxdata->cursor->map) + (offset));
            }

        /* Show updated cursor */
        HIDDNouveauShowCursor(gfx, TRUE);

        UNLOCK_ENGINE
    }

    return TRUE;
}

BOOL METHOD(NouveauDisplay, Hidd_Display, SetCursorPos)
{
    OOP_Object *gfx = NULL;
    struct HIDDNouveauData * gfxdata;
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);

    LOCK_ENGINE
    drmModeMoveCursor(NOUVEAU_DEV_FD(nvdev), gfxdata->selectedcrtcid, msg->x, msg->y);
    UNLOCK_ENGINE

    return TRUE;
}

VOID METHOD(NouveauDisplay, Hidd_Display, SetCursorVisible)
{
    OOP_Object *gfx = NULL;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    HIDDNouveauShowCursor(gfx, msg->visible);
}

/*
 * Plane layout: 0 = primary (the crtc's framebuffer, driven by Show()),
 * 1 .. nplanes = the crtc's overlay planes, last = the hardware cursor.
 * The overlays scan out at 1:1 only: the dispnv50 window code checks the
 * plane state with DRM_PLANE_NO_SCALING (the window scaler needs the
 * usage bounds, 5-tap coefficients and the CSC0/CSC1 pipelines that nvkms
 * programs around it), so ScaleMin == ScaleMax == unity is the truth.
 */
ULONG METHOD(NouveauDisplay, Hidd_Display, GetPlaneInfo)
{
    struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, o);
    ULONG overlays = gfxdata ? gfxdata->nplanes : 0;
    struct HIDD_PlaneInfo info;
    ULONG len = msg->infoLen;

    if (msg->index == 0)
        return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    memset(&info, 0, sizeof(info));
    info.ZMin = info.ZMax = msg->index;
    info.ScaleMin = info.ScaleMax = HIDD_PLANE_SCALE_UNITY;
    info.PosAlignX = info.PosAlignY = 1;
    if (msg->index <= overlays)
    {
        struct HIDDNouveauPlane *p = &gfxdata->planes[msg->index - 1];

        info.Type         = vHidd_Plane_Overlay;
        info.PixFmtMask   = p->dead ? 0 : HIDDNouveauPlanePixFmts(p->formats);
        info.VideoFmtMask = p->dead ? 0 : HIDDNouveauPlaneVideoFmts(gfxdata, p->formats);
        info.AlphaModes   = (1 << vHidd_CompAlpha_None);
        info.Flags        = PIF_NEEDS_DISPLAYMEM;     /* displayable bitmaps: buffer objects in video RAM */
        if (p->zpos_mutable && (p->zmax > p->zmin))
        {
            info.ZMin = p->zmin;
            info.ZMax = p->zmax;
        }
        if (HIDDNouveauPlaneKeys(p))
            info.Flags |= PIF_COLORKEY;
    }
    else if (msg->index == overlays + 1)
    {
        info.Type       = vHidd_Plane_Cursor;
        info.PixFmtMask = 0xFFFFFFFF;               /* converted to ARGB on the way */
        info.AlphaModes = (1 << vHidd_CompAlpha_PerPixel);
        info.MaxWidth   = 64;
        info.MaxHeight  = 64;
    }
    else
        return 0;

    if (len > sizeof(info))
        len = sizeof(info);
    CopyMem(&info, msg->info, len);
    return len;
}

/*
 * One overlay's part of an atomic request: the whole plane state when a
 * bitmap is given, FB_ID/CRTC_ID 0 to turn it off. zpos and colorkey only
 * where the plane has the (mutable) property.
 */
static int HIDDNouveauPlaneRequest(drmModeAtomicReqPtr req, struct HIDDNouveauData *gfxdata,
    struct HIDDNouveauPlane *p, ULONG fbid, struct HIDD_PlaneState *st)
{
    int ret = 0;

    if (!fbid)
    {
        ret |= drmModeAtomicAddProperty(req, p->id, p->prop_fb_id, 0);
        ret |= drmModeAtomicAddProperty(req, p->id, p->prop_crtc_id, 0);
        return (ret < 0) ? ret : 0;
    }
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_fb_id, fbid);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_crtc_id, gfxdata->selectedcrtcid);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_crtc_x, (uint64_t)(int64_t)st->Dest.MinX);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_crtc_y, (uint64_t)(int64_t)st->Dest.MinY);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_crtc_w, st->Dest.MaxX - st->Dest.MinX + 1);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_crtc_h, st->Dest.MaxY - st->Dest.MinY + 1);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_src_x, (uint64_t)(ULONG)st->Src.MinX << 16);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_src_y, (uint64_t)(ULONG)st->Src.MinY << 16);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_src_w, (uint64_t)(ULONG)(st->Src.MaxX - st->Src.MinX + 1) << 16);
    ret |= drmModeAtomicAddProperty(req, p->id, p->prop_src_h, (uint64_t)(ULONG)(st->Src.MaxY - st->Src.MinY + 1) << 16);
    if (p->prop_zpos && p->zpos_mutable)
    {
        ULONG z = st->ZOrder;

        if (z < p->zmin) z = p->zmin;
        if (z > p->zmax) z = p->zmax;
        ret |= drmModeAtomicAddProperty(req, p->id, p->prop_zpos, z);
    }
    if (p->prop_ckey)
    {
        ULONG key = (st->Flags & PSF_COLORKEY) ? (0x01000000 | (st->ColorKey & 0x00FFFFFF)) : 0;

        ret |= drmModeAtomicAddProperty(req, p->id, p->prop_ckey, key);
    }
    return (ret < 0) ? ret : 0;
}

/* Remember what a successful commit put on the plane (SwapBuffers re-uses it) */
static VOID HIDDNouveauPlaneRemember(struct HIDDNouveauPlane *p, ULONG fbid, struct HIDD_PlaneState *st)
{
    p->fbid = fbid;
    if (!fbid || !st)
        return;
    p->crtc_x = st->Dest.MinX;
    p->crtc_y = st->Dest.MinY;
    p->crtc_w = st->Dest.MaxX - st->Dest.MinX + 1;
    p->crtc_h = st->Dest.MaxY - st->Dest.MinY + 1;
    p->src_x  = (ULONG)st->Src.MinX << 16;
    p->src_y  = (ULONG)st->Src.MinY << 16;
    p->src_w  = (ULONG)(st->Src.MaxX - st->Src.MinX + 1) << 16;
    p->src_h  = (ULONG)(st->Src.MaxY - st->Src.MinY + 1) << 16;
    if (p->zpos_mutable)
    {
        p->zpos = st->ZOrder;
        if (p->zpos < p->zmin) p->zpos = p->zmin;
        if (p->zpos > p->zmax) p->zpos = p->zmax;
    }
    p->ckey = (st->Flags & PSF_COLORKEY) ? (0x01000000 | (st->ColorKey & 0x00FFFFFF)) : 0;
}

/*
 * Legacy programming of one overlay: the plane properties the SETPLANE
 * call does not carry are set first (each is an atomic commit of its own
 * in the DRM core), then the framebuffer and geometry.
 */
static int HIDDNouveauPlaneSetLegacy(LONG fd, struct HIDDNouveauData *gfxdata,
    struct HIDDNouveauPlane *p, ULONG fbid, struct HIDD_PlaneState *st)
{
    int ret;

    if (!fbid)
        return drmModeSetPlane(fd, p->id, gfxdata->selectedcrtcid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (p->prop_zpos && p->zpos_mutable)
    {
        ULONG z = st->ZOrder;

        if (z < p->zmin) z = p->zmin;
        if (z > p->zmax) z = p->zmax;
        if (z != p->zpos)
        {
            ret = drmModeObjectSetProperty(fd, p->id, DRM_MODE_OBJECT_PLANE, p->prop_zpos, z);
            if (ret)
                nvlog("[Nouveau] plane %lu: zpos %lu: %d\n", (unsigned long)p->id, (unsigned long)z, ret);
        }
    }
    if (p->prop_ckey)
    {
        ULONG key = (st->Flags & PSF_COLORKEY) ? (0x01000000 | (st->ColorKey & 0x00FFFFFF)) : 0;

        if (key != p->ckey)
        {
            ret = drmModeObjectSetProperty(fd, p->id, DRM_MODE_OBJECT_PLANE, p->prop_ckey, key);
            if (ret)
                nvlog("[Nouveau] plane %lu: colorkey %08lx: %d\n", (unsigned long)p->id, (unsigned long)key, ret);
        }
    }
    return drmModeSetPlane(fd, p->id, gfxdata->selectedcrtcid, fbid, 0,
                           st->Dest.MinX, st->Dest.MinY,
                           st->Dest.MaxX - st->Dest.MinX + 1, st->Dest.MaxY - st->Dest.MinY + 1,
                           st->Src.MinX << 16, st->Src.MinY << 16,
                           (st->Src.MaxX - st->Src.MinX + 1) << 16, (st->Src.MaxY - st->Src.MinY + 1) << 16);
}

/*
 * A window whose UPDATE did not complete (dispnv50 notifier timeout, after
 * an exception such as the GB207's Xid 56 reason 5 on a packed 4:2:2
 * surface) would cost every later commit that touches it a 2 s wait. The
 * DRM side remembers such planes; after each real commit the planes it
 * touched are checked, a timed-out one is turned off once (that single
 * attempt may itself take the 2 s) and never used again this boot: it
 * takes no format, so the compositor composes instead. Returns TRUE when
 * one of the given planes died.
 */
extern int nv50_disp_plane_timed_out(uint32_t plane_id);
extern ULONG nv50_disp_core_timeouts;

static BOOL HIDDNouveauPlanesCheckTimeouts(LONG fd, struct HIDDNouveauData *gfxdata, ULONG mask)
{
    static ULONG core_noted = 0;
    BOOL died = FALSE;
    ULONG i;

    if (nv50_disp_core_timeouts != core_noted)
    {
        core_noted = nv50_disp_core_timeouts;
        nvlog("[Nouveau] core notifier timeout(s) seen (%lu): the display engine may be stalled\n",
            (unsigned long)core_noted);
    }
    for (i = 0; i < gfxdata->nplanes; i++)
    {
        struct HIDDNouveauPlane *p = &gfxdata->planes[i];
        int ret;

        if (!(mask & (1UL << i)) || p->dead || !nv50_disp_plane_timed_out(p->id))
            continue;
        p->dead = TRUE;
        died = TRUE;
        nvlog("[Nouveau] plane %lu: window channel timed out; turning it off and not using it again (compositing instead)\n",
            (unsigned long)p->id);
        LOCK_ENGINE
        ret = drmModeSetPlane(fd, p->id, gfxdata->selectedcrtcid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        UNLOCK_ENGINE
        nvlog("[Nouveau] plane %lu: off after timeout: %d\n", (unsigned long)p->id, ret);
        p->fbid = 0;
    }
    return died;
}

/*
 * The compositor's plane assignment. A test commit (fHidd_Commit_Test) is
 * answered by the kernel: the overlay states go into one atomic request
 * committed with DRM_MODE_ATOMIC_TEST_ONLY, which runs the same checks as
 * the real commit (plane format, position, no scaling, the window code's
 * acquire) without touching the hardware; the static checks before it are
 * a fast pre-filter. The real commit uses the same request (one flip for
 * all overlays) unless NOUVEAU_PLANE_ATOMIC=0, which keeps the per-plane
 * SETPLANE calls. Either way it then waits a vblank as before.
 */
BOOL METHOD(NouveauDisplay, Hidd_Display, CommitPlanes)
{
    struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, o);
    struct nouveau_device *nvdev = SD(cl)->carddata.dev;
    ULONG overlays = gfxdata ? gfxdata->nplanes : 0;
    ULONG cursorplane = overlays + 1;
    struct HIDD_PlaneState *primary = NULL;
    ULONG i;
    BOOL changed = FALSE;
    BOOL overlaywork = FALSE;
    ULONG touched = 0;              /* overlay planes this commit programmed */

    if (!gfxdata)
        return FALSE;

    /* Validate first: a commit is all or nothing */
    for (i = 0; i < msg->count; i++)
    {
        struct HIDD_PlaneState *st = &msg->states[i];

        if (st->Plane == 0)
        {
            primary = st;
            continue;
        }
        if (st->Plane > cursorplane)
            return FALSE;
        if (st->Plane <= overlays)
        {
            struct HIDDNouveauPlane *p = &gfxdata->planes[st->Plane - 1];

            if (st->BitMap)
            {
                struct HIDDNouveauBitMapData *bmdata;

                if (OOP_OCLASS(st->BitMap) != SD(cl)->bmclass)
                    return FALSE;
                bmdata = OOP_INST_DATA(SD(cl)->bmclass, st->BitMap);
                if ((!bmdata->displayable && !bmdata->nbufs) || !bmdata->bo)
                    return FALSE;                       /* not in video RAM */
                if (!HIDDNouveauPlaneTakes(gfxdata, p, bmdata))
                    return FALSE;                       /* format, or a dead plane */
                if (((st->Dest.MaxX - st->Dest.MinX) != (st->Src.MaxX - st->Src.MinX)) ||
                    ((st->Dest.MaxY - st->Dest.MinY) != (st->Src.MaxY - st->Src.MinY)))
                    return FALSE;                       /* no scaling */
                if ((st->Src.MinX < 0) || (st->Src.MinY < 0) ||
                    (st->Src.MaxX >= (LONG)bmdata->drawable.width) ||
                    (st->Src.MaxY >= (LONG)bmdata->drawable.height))
                    return FALSE;                       /* source outside the bitmap */
                if (st->AlphaMode != vHidd_CompAlpha_None)
                    return FALSE;
                if ((st->Flags & PSF_COLORKEY) && !HIDDNouveauPlaneKeys(p))
                    return FALSE;
                overlaywork = TRUE;
            }
            else if (p->fbid)
                overlaywork = TRUE;
        }
    }

    if (primary)
    {
        struct pHidd_Display_CommitPlanes p = { msg->mID, primary, 1, msg->flags };

        if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)&p))
            return FALSE;
    }

    /*
     * Overlays: one atomic request. A test only asks the kernel; a real
     * commit flips every overlay of the display at once.
     */
    if (overlaywork && gfxdata->atomic)
    {
        drmModeAtomicReqPtr req = drmModeAtomicAlloc();
        int ret = 0;

        if (!req)
            return FALSE;
        for (i = 0; (i < msg->count) && !ret; i++)
        {
            struct HIDD_PlaneState *st = &msg->states[i];
            struct HIDDNouveauPlane *p;

            if ((st->Plane == 0) || (st->Plane > overlays))
                continue;
            p = &gfxdata->planes[st->Plane - 1];
            if (st->BitMap)
            {
                struct HIDDNouveauBitMapData *bmdata = OOP_INST_DATA(SD(cl)->bmclass, st->BitMap);

                if (!HIDDNouveauEnsureFB(st->BitMap))
                    ret = -1;
                else
                    ret = HIDDNouveauPlaneRequest(req, gfxdata, p, bmdata->fbid, st);
            }
            else if (p->fbid)
                ret = HIDDNouveauPlaneRequest(req, gfxdata, p, 0, NULL);
        }
        if (!ret)
        {
            LOCK_ENGINE
            ret = drmModeAtomicCommit(NOUVEAU_DEV_FD(nvdev), req,
                    (msg->flags & fHidd_Commit_Test) ? DRM_MODE_ATOMIC_TEST_ONLY : 0, NULL);
            UNLOCK_ENGINE
        }
        drmModeAtomicFree(req);
        if (msg->flags & fHidd_Commit_Test)
        {
            if (ret)
                nvlog("[Nouveau] atomic test of %lu plane state(s): %d\n", (unsigned long)msg->count, ret);
            return ret ? FALSE : TRUE;
        }
        if (ret)
        {
            nvlog("[Nouveau] atomic commit of %lu plane state(s) failed: %d\n", (unsigned long)msg->count, ret);
            HIDDNouveauPlanesCheckTimeouts(NOUVEAU_DEV_FD(nvdev), gfxdata, (1UL << overlays) - 1);
            return FALSE;
        }
        for (i = 0; i < msg->count; i++)
        {
            struct HIDD_PlaneState *st = &msg->states[i];
            struct HIDDNouveauPlane *p;

            if ((st->Plane == 0) || (st->Plane > overlays))
                continue;
            p = &gfxdata->planes[st->Plane - 1];
            if (st->BitMap)
            {
                struct HIDDNouveauBitMapData *bmdata = OOP_INST_DATA(SD(cl)->bmclass, st->BitMap);

                nvlog("[Nouveau] plane %lu: fb %lu (%lux%lu bo %p) at %d,%d %dx%d%s on crtc %lu (atomic)\n", (unsigned long)p->id,
                    (unsigned long)bmdata->fbid, (unsigned long)bmdata->drawable.width, (unsigned long)bmdata->drawable.height,
                    bmdata->bo, st->Dest.MinX, st->Dest.MinY, st->Dest.MaxX - st->Dest.MinX + 1, st->Dest.MaxY - st->Dest.MinY + 1,
                    (st->Flags & PSF_COLORKEY) ? " keyed" : "", (unsigned long)gfxdata->selectedcrtcid);
                HIDDNouveauPlaneRemember(p, bmdata->fbid, st);
                changed = TRUE;
                touched |= 1UL << (st->Plane - 1);
            }
            else if (p->fbid)
            {
                nvlog("[Nouveau] plane %lu: off (atomic)\n", (unsigned long)p->id);
                HIDDNouveauPlaneRemember(p, 0, NULL);
                changed = TRUE;
                touched |= 1UL << (st->Plane - 1);
            }
        }
    }
    else if (msg->flags & fHidd_Commit_Test)
        return TRUE;

    for (i = 0; i < msg->count; i++)
    {
        struct HIDD_PlaneState *st = &msg->states[i];
        struct HIDDNouveauPlane *p;
        int ret;

        if (st->Plane == 0)
            continue;
        if (st->Plane == cursorplane)
        {
            if (st->BitMap)
            {
                struct pHidd_Display_SetCursorShape shape = { OOP_GetMethodID(IID_Hidd_Display, moHidd_Display_SetCursorShape), st->BitMap, 0, 0 };
                struct pHidd_Display_SetCursorPos   pos   = { OOP_GetMethodID(IID_Hidd_Display, moHidd_Display_SetCursorPos), st->Dest.MinX, st->Dest.MinY };

                OOP_DoMethod(o, (OOP_Msg)&shape);
                OOP_DoMethod(o, (OOP_Msg)&pos);
            }
            else
            {
                struct pHidd_Display_SetCursorVisible vis = { OOP_GetMethodID(IID_Hidd_Display, moHidd_Display_SetCursorVisible), FALSE };

                OOP_DoMethod(o, (OOP_Msg)&vis);
            }
            continue;
        }
        if (gfxdata->atomic)
            continue;                                   /* overlays done above */
        /* Overlay (legacy): the bitmap is scanned out from its own buffer object */
        p = &gfxdata->planes[st->Plane - 1];
        if (st->BitMap)
        {
            struct HIDDNouveauBitMapData *bmdata = OOP_INST_DATA(SD(cl)->bmclass, st->BitMap);

            if (!HIDDNouveauEnsureFB(st->BitMap))
                return FALSE;
            LOCK_ENGINE
            ret = HIDDNouveauPlaneSetLegacy(NOUVEAU_DEV_FD(nvdev), gfxdata, p, bmdata->fbid, st);
            UNLOCK_ENGINE
            nvlog("[Nouveau] plane %lu: fb %lu (%lux%lu bo %p) at %d,%d %dx%d%s on crtc %lu: %d\n", (unsigned long)p->id,
                (unsigned long)bmdata->fbid, (unsigned long)bmdata->drawable.width, (unsigned long)bmdata->drawable.height,
                bmdata->bo, st->Dest.MinX, st->Dest.MinY, st->Dest.MaxX - st->Dest.MinX + 1, st->Dest.MaxY - st->Dest.MinY + 1,
                (st->Flags & PSF_COLORKEY) ? " keyed" : "", (unsigned long)gfxdata->selectedcrtcid, ret);
            touched |= 1UL << (st->Plane - 1);
            if (ret)
            {
                HIDDNouveauPlanesCheckTimeouts(NOUVEAU_DEV_FD(nvdev), gfxdata, touched);
                return FALSE;
            }
            HIDDNouveauPlaneRemember(p, bmdata->fbid, st);
            changed = TRUE;
        }
        else if (p->fbid)
        {
            LOCK_ENGINE
            ret = HIDDNouveauPlaneSetLegacy(NOUVEAU_DEV_FD(nvdev), gfxdata, p, 0, NULL);
            UNLOCK_ENGINE
            nvlog("[Nouveau] plane %lu: off: %d\n", (unsigned long)p->id, ret);
            HIDDNouveauPlaneRemember(p, 0, NULL);
            changed = TRUE;
            touched |= 1UL << (st->Plane - 1);
        }
    }
    /*
     * A window that did not arm (notifier timeout) is dead for this boot:
     * report the commit as failed so that the compositor composes.
     */
    if (touched && HIDDNouveauPlanesCheckTimeouts(NOUVEAU_DEV_FD(nvdev), gfxdata, touched))
        return FALSE;
    /*
     * Latch only when an overlay was reprogrammed: that is when a bitmap may
     * have stopped being scanned out. The compositor calls this with every
     * overlay off on each screen-stack change, from LoadView() with
     * graphics.library's view semaphore and Intuition's locks held; sleeping
     * a frame there for nothing stalled Intuition and left the desktop
     * partly drawn on the GB207 (2026-10-23).
     */
    if (changed && !(msg->flags & fHidd_Commit_NoWait))
        HIDDNouveauWaitVBlank(NOUVEAU_DEV_FD(nvdev), gfxdata);
    return TRUE;
}

/*
 * SwapBuffers of a scan-out bitmap: if one of the display's overlays shows
 * the old framebuffer, re-program it with the new one at the same geometry
 * and (unless told not to) wait for the vblank that retires the old buffer.
 * FALSE when no plane shows it (the bitmap is composed instead).
 */
BOOL HIDDNouveauPlaneSwap(OOP_Object *display, ULONG oldfbid, ULONG newfbid, BOOL wait)
{
    OOP_Class *cl = OOP_OCLASS(display);
    struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, display);
    struct nouveau_device *nvdev = SD(cl)->carddata.dev;
    struct HIDDNouveauPlane *p = NULL;
    ULONG i;
    int ret;

    if (!gfxdata || !oldfbid || !newfbid)
        return FALSE;
    for (i = 0; i < gfxdata->nplanes; i++)
    {
        if (gfxdata->planes[i].fbid == oldfbid)
            p = &gfxdata->planes[i];
    }
    if (!p)
        return FALSE;

    LOCK_ENGINE
    if (gfxdata->atomic)
    {
        drmModeAtomicReqPtr req = drmModeAtomicAlloc();

        ret = -1;
        if (req)
        {
            if (drmModeAtomicAddProperty(req, p->id, p->prop_fb_id, newfbid) > 0)
                ret = drmModeAtomicCommit(NOUVEAU_DEV_FD(nvdev), req, 0, NULL);
            drmModeAtomicFree(req);
        }
    }
    else
    {
        ret = drmModeSetPlane(NOUVEAU_DEV_FD(nvdev), p->id, gfxdata->selectedcrtcid, newfbid, 0,
                              p->crtc_x, p->crtc_y, p->crtc_w, p->crtc_h,
                              p->src_x, p->src_y, p->src_w, p->src_h);
    }
    UNLOCK_ENGINE
    if (ret)
    {
        nvlog("[Nouveau] plane %lu: swap fb %lu -> %lu failed: %d\n", (unsigned long)p->id,
            (unsigned long)oldfbid, (unsigned long)newfbid, ret);
        HIDDNouveauPlanesCheckTimeouts(NOUVEAU_DEV_FD(nvdev), gfxdata, 1UL << (p - gfxdata->planes));
        return TRUE;        /* it is on a plane; nothing to compose either way */
    }
    p->fbid = newfbid;
    if (HIDDNouveauPlanesCheckTimeouts(NOUVEAU_DEV_FD(nvdev), gfxdata, 1UL << (p - gfxdata->planes)))
        return FALSE;       /* the window died: the bitmap is composed from now on */
    if (wait)
        HIDDNouveauWaitVBlank(NOUVEAU_DEV_FD(nvdev), gfxdata);
    return TRUE;
}

VOID METHOD(NouveauDisplay, Hidd_Display, WaitLatch)
{
    struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, o);

    if (gfxdata)
        HIDDNouveauWaitVBlank(NOUVEAU_DEV_FD(SD(cl)->carddata.dev), gfxdata);
}

/*
 * Can a plane of this display show a client surface of that format and
 * size? Answered from the crtc's overlay planes: SCF_PLANE when one scans
 * the format out and the size is a framebuffer the kernel takes,
 * SCF_COLORKEY when that plane keys on the primary (dispnv50 windows),
 * never SCF_SCALE (see GetPlaneInfo). Up to NOUVEAU_MAX_BUFFERS buffers.
 */
ULONG METHOD(NouveauDisplay, Hidd_Display, QueryScanout)
{
    struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, o);
    struct HIDDNouveauBitMapData want;
    struct HIDDNouveauPlane *p = NULL;
    ULONG i, flags = 0;

    if (msg->caps)
        memset(msg->caps, 0, sizeof(*msg->caps));
    if (!gfxdata || !msg->width || !msg->height)
        return 0;
    if (gfxdata->maxfbw && ((msg->width > gfxdata->maxfbw) || (msg->height > gfxdata->maxfbh)))
        return 0;

    memset(&want, 0, sizeof(want));
    want.videoformat = msg->videoFmt;
    want.stdfmt = msg->videoFmt ? vHidd_StdPixFmt_Unknown : msg->pixFmt;
    for (i = 0; i < gfxdata->nplanes; i++)
    {
        if (HIDDNouveauPlaneTakes(gfxdata, &gfxdata->planes[i], &want))
        {
            p = &gfxdata->planes[i];
            flags |= SCF_PLANE;
            if (HIDDNouveauPlaneKeys(p))
            {
                flags |= SCF_COLORKEY;
                break;              /* the best there is */
            }
        }
    }
    if (!p)
        return 0;
    if (msg->caps)
    {
        IPTR key = HIDD_DISPLAY_DEFAULT_COLORKEY;

        OOP_GetAttr(o, aHidd_Display_ColorKey, &key);
        msg->caps->Flags      = flags;
        msg->caps->MaxWidth   = gfxdata->maxfbw;
        msg->caps->MaxHeight  = gfxdata->maxfbh;
        msg->caps->MaxBuffers = NOUVEAU_MAX_BUFFERS;
        msg->caps->ScaleMin   = HIDD_PLANE_SCALE_UNITY;
        msg->caps->ScaleMax   = HIDD_PLANE_SCALE_UNITY;
        msg->caps->ColorKey   = (flags & SCF_COLORKEY) ? (ULONG)key : 0;
    }
    return flags;
}

static struct HIDD_ModeProperties modeprops = 
{
    DIPF_IS_SPRITES,
    1,
    0   /* no composition in hardware: the display's compositor composes */
};

ULONG METHOD(NouveauDisplay, Hidd_Display, ModeProperties)
{
    ULONG len = msg->propsLen;

    if (len > sizeof(modeprops))
        len = sizeof(modeprops);
    CopyMem(&modeprops, msg->props, len);

    return len;
}

/*
 * What size of screen to open when nothing asks for a particular one.
 *
 * The monitor says which timing it was built for, and that is the one
 * it displays without complaint; anything else it merely tolerates, if
 * at all. Take its dimensions from that mode rather than from a fixed
 * pair, so the default screen is the one the display actually wants.
 */
VOID METHOD(NouveauDisplay, Hidd_Display, NominalDimensions)
{
    OOP_Object *gfx = NULL;
    ULONG width = 1024, height = 768;

    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)&gfx);
    if (gfx)
    {
        struct HIDDNouveauData *gfxdata = OOP_INST_DATA(SD(cl)->gfxclass, gfx);
        drmModeConnectorPtr connector = (drmModeConnectorPtr)gfxdata->selectedconnector;

        if (connector && connector->count_modes > 0)
        {
            drmModeModeInfoPtr mode = &connector->modes[0];
            int i;

            for (i = 0; i < connector->count_modes; i++)
            {
                if (connector->modes[i].type & DRM_MODE_TYPE_PREFERRED)
                {
                    mode = &connector->modes[i];
                    break;
                }
            }

            /* Nothing marked preferred: the list is ordered best first */
            width = mode->hdisplay;
            height = mode->vdisplay;

            D(bug("[Nouveau] Nominal dimensions %ux%u from '%s'\n",
                  width, height, mode->name));
        }
    }

    if (msg->width)
        *(msg->width) = width;
    if (msg->height)
        *(msg->height) = height;
    if (msg->depth)
        *(msg->depth) = 24;
}
