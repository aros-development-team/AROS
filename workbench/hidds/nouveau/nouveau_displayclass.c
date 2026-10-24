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
 * Register the bitmap's buffer object as a KMS framebuffer (once). Needed to
 * scan it out, on the crtc or on an overlay plane; Dispose() unregisters it.
 */
BOOL HIDDNouveauEnsureFB(OOP_Object * bm)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    struct CardData * carddata = &(SD(cl)->carddata);
    struct nouveau_device *nvdev = carddata->dev;
    LONG ret;

    if (bmdata->fbid != 0)
        return TRUE;
    if (!bmdata->bo)
        return FALSE;
    LOCK_ENGINE
    ret = drmModeAddFB(NOUVEAU_DEV_FD(nvdev), bmdata->drawable.width, bmdata->drawable.height,
                bmdata->drawable.depth, bmdata->bytesperpixel * 8,
                bmdata->pitch, bmdata->bo->handle, &bmdata->fbid);
    UNLOCK_ENGINE
    if (ret)
    {
        nvlog("[Nouveau] Not able to add framebuffer, %ld\n", (long)ret);
        bmdata->fbid = 0;
        return FALSE;
    }
    return TRUE;
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
 * The plane's "type" property (0 overlay, 1 primary, 2 cursor, as the DRM
 * core's enum drm_plane_type). The AROS drm file has universal planes on,
 * so the plane list also holds each crtc's primary and cursor; only real
 * overlays may be offered to the compositor. -1 if it cannot be read.
 */
static LONG HIDDNouveauPlaneType(LONG fd, ULONG plane_id)
{
    struct drm_mode_obj_get_properties op;
    uint32_t ids[32];
    uint64_t values[32];
    ULONG i;

    memset(&op, 0, sizeof(op));
    op.obj_id = plane_id;
    op.obj_type = DRM_MODE_OBJECT_PLANE;
    if (drmIoctl(fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &op) || !op.count_props)
        return -1;
    if (op.count_props > 32)
        op.count_props = 32;
    op.props_ptr = (uint64_t)(IPTR)ids;
    op.prop_values_ptr = (uint64_t)(IPTR)values;
    if (drmIoctl(fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &op))
        return -1;
    for (i = 0; i < op.count_props && i < 32; i++)
    {
        struct drm_mode_get_property gp;

        memset(&gp, 0, sizeof(gp));
        gp.prop_id = ids[i];
        if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPERTY, &gp))
            continue;
        if (!strcmp(gp.name, "type"))
            return (LONG)values[i];
    }
    return -1;
}

VOID HIDDNouveauEnumPlanes(LONG fd, struct HIDDNouveauData *gfxdata)
{
    drmModeResPtr res;
    drmModePlaneResPtr pres;
    ULONG i;

    gfxdata->nplanes = 0;
    gfxdata->selectedcrtcindex = 0;
    LOCK_ENGINE
    res = drmModeGetResources(fd);
    if (res)
    {
        for (i = 0; i < res->count_crtcs; i++)
        {
            if (res->crtcs[i] == gfxdata->selectedcrtcid)
                gfxdata->selectedcrtcindex = i;
        }
        drmModeFreeResources(res);
    }
    pres = drmModeGetPlaneResources(fd);
    if (pres)
    {
        for (i = 0; (i < pres->count_planes) && (gfxdata->nplanes < NOUVEAU_MAX_PLANES); i++)
        {
            drmModePlanePtr p = drmModeGetPlane(fd, pres->planes[i]);
            ULONG f, fmts = 0;

            if (!p)
                continue;
            if ((p->possible_crtcs & (1UL << gfxdata->selectedcrtcindex)) &&
                (HIDDNouveauPlaneType(fd, p->plane_id) == 0 /* overlay */))
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
                    }
                }
                if (fmts)
                {
                    gfxdata->planes[gfxdata->nplanes].id      = p->plane_id;
                    gfxdata->planes[gfxdata->nplanes].formats = fmts;
                    gfxdata->planes[gfxdata->nplanes].fbid    = 0;
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
    nvlog("[Nouveau] crtc %lu: %lu overlay plane(s) for RGB scan-out, vblank %s\n",
        (unsigned long)gfxdata->selectedcrtcid, (unsigned long)gfxdata->nplanes,
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
        if (vHidd_ModeID_Invalid != modeid) 
        {
            /* User supplied a valid modeid. We can use our bitmap class */
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
        info.Type       = vHidd_Plane_Overlay;
        info.PixFmtMask = HIDDNouveauPlanePixFmts(gfxdata->planes[msg->index - 1].formats);
        info.AlphaModes = (1 << vHidd_CompAlpha_None);
        info.Flags      = PIF_NEEDS_DISPLAYMEM;     /* displayable bitmaps: buffer objects in video RAM */
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

BOOL METHOD(NouveauDisplay, Hidd_Display, CommitPlanes)
{
    struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, o);
    struct nouveau_device *nvdev = SD(cl)->carddata.dev;
    ULONG overlays = gfxdata ? gfxdata->nplanes : 0;
    ULONG cursorplane = overlays + 1;
    struct HIDD_PlaneState *primary = NULL;
    ULONG i;
    BOOL changed = FALSE;

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
        if (st->BitMap && (st->Plane <= overlays))
        {
            struct HIDDNouveauBitMapData *bmdata;

            if (OOP_OCLASS(st->BitMap) != SD(cl)->bmclass)
                return FALSE;
            bmdata = OOP_INST_DATA(SD(cl)->bmclass, st->BitMap);
            if (!bmdata->displayable || !bmdata->bo)
                return FALSE;                       /* not in video RAM */
            if (((st->Dest.MaxX - st->Dest.MinX) != (st->Src.MaxX - st->Src.MinX)) ||
                ((st->Dest.MaxY - st->Dest.MinY) != (st->Src.MaxY - st->Src.MinY)))
                return FALSE;                       /* no scaling */
            if (st->AlphaMode != vHidd_CompAlpha_None)
                return FALSE;
        }
    }

    if (primary)
    {
        struct pHidd_Display_CommitPlanes p = { msg->mID, primary, 1, msg->flags };

        if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)&p))
            return FALSE;
    }
    if (msg->flags & fHidd_Commit_Test)
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
        /* Overlay: the screen bitmap is scanned out from its own buffer object */
        p = &gfxdata->planes[st->Plane - 1];
        if (st->BitMap)
        {
            struct HIDDNouveauBitMapData *bmdata = OOP_INST_DATA(SD(cl)->bmclass, st->BitMap);

            if (!HIDDNouveauEnsureFB(st->BitMap))
                return FALSE;
            LOCK_ENGINE
            ret = drmModeSetPlane(NOUVEAU_DEV_FD(nvdev), p->id, gfxdata->selectedcrtcid, bmdata->fbid, 0,
                                  st->Dest.MinX, st->Dest.MinY,
                                  st->Dest.MaxX - st->Dest.MinX + 1, st->Dest.MaxY - st->Dest.MinY + 1,
                                  st->Src.MinX << 16, st->Src.MinY << 16,
                                  (st->Src.MaxX - st->Src.MinX + 1) << 16, (st->Src.MaxY - st->Src.MinY + 1) << 16);
            UNLOCK_ENGINE
            nvlog("[Nouveau] plane %lu: fb %lu (%lux%lu bo %p) at %d,%d %dx%d on crtc %lu: %d\n", (unsigned long)p->id,
                (unsigned long)bmdata->fbid, (unsigned long)bmdata->drawable.width, (unsigned long)bmdata->drawable.height,
                bmdata->bo, st->Dest.MinX, st->Dest.MinY, st->Dest.MaxX - st->Dest.MinX + 1, st->Dest.MaxY - st->Dest.MinY + 1,
                (unsigned long)gfxdata->selectedcrtcid, ret);
            if (ret)
                return FALSE;
            p->fbid = bmdata->fbid;
            changed = TRUE;
        }
        else if (p->fbid)
        {
            LOCK_ENGINE
            ret = drmModeSetPlane(NOUVEAU_DEV_FD(nvdev), p->id, gfxdata->selectedcrtcid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            UNLOCK_ENGINE
            nvlog("[Nouveau] plane %lu: off: %d\n", (unsigned long)p->id, ret);
            p->fbid = 0;
            changed = TRUE;
        }
    }
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

VOID METHOD(NouveauDisplay, Hidd_Display, WaitLatch)
{
    struct HIDDNouveauData *gfxdata = HIDDNouveauGfxData(cl, o);

    if (gfxdata)
        HIDDNouveauWaitVBlank(NOUVEAU_DEV_FD(SD(cl)->carddata.dev), gfxdata);
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
