/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: cgxvideo overlays on HVS planes.

    The base class keeps the pages in our scanout memory, so the plane
    scans them in place: the 16-bit RGB formats straight from what the
    client writes, YUV after the base class has converted it. A new page
    goes on at the next vblank; nothing writes into the page it replaced
    until that has latched. Where the backend has a single overlay plane,
    a layer showing on it takes it from a gallium driver, which falls back
    to blitting until the layer hides.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <string.h>

#include "vcgfx_bitmap.h"
#include "vcgfx_hidd.h"

#include LC_LIBDEFS_FILE

/* Take the plane down if this overlay holds one; caller has vcsd_OvlLock. */
static void ovl_release(struct VideoCoreGfx_staticdata *xsd, OOP_Object *o)
{
    if (vcgfx_ovl_find(xsd, o))
        vcgfx_ovl_hide(xsd, o);
}

/* The page left by the last flip may still be on display until the
 * vblank; Lock, Unlock and Swap may write into it next. */
static void ovl_settle(struct VideoCoreGfx_staticdata *xsd, OOP_Object *o)
{
    struct vcgfx_ovl *e = vcgfx_ovl_find(xsd, o);

    if (e && e->ov_Shown)
        vcgfx_ovl_latch_wait(xsd, o);
}

APTR VideoCoreGfxOverlay__Hidd_Overlay__Lock(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ovl_settle(XSD(cl), o);
    return (APTR)OOP_DoSuperMethod(cl, o, msg);
}

VOID VideoCoreGfxOverlay__Hidd_Overlay__Unlock(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ovl_settle(XSD(cl), o);
    OOP_DoSuperMethod(cl, o, msg);
}

VOID VideoCoreGfxOverlay__Hidd_Overlay__Swap(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    ovl_settle(XSD(cl), o);
    OOP_DoSuperMethod(cl, o, msg);
}

VOID VideoCoreGfxOverlay__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);

    ObtainSemaphore(&xsd->vcsd_OvlLock);
    ovl_release(xsd, o);
    ReleaseSemaphore(&xsd->vcsd_OvlLock);

    OOP_DoSuperMethod(cl, o, msg);
}

BOOL VideoCoreGfxOverlay__Hidd_Overlay__Show(OOP_Class *cl, OOP_Object *o,
                                             struct pHidd_Overlay_Show *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    OOP_Class *bmcl = OOP_OCLASS(msg->bitMap);
    struct vc4gfx_overlay desc;
    struct vcgfx_ovl *e;
    IPTR frame = 0, fmt = 0, mod = 0, sw = 0, sh = 0;
    BOOL shown, was;

    /* x/y are in a screen of ours */
    if ((bmcl != xsd->vcsd_VideoCoreGfxOnBMClass)
        && (bmcl != xsd->vcsd_VideoCoreGfxOffBMClass))
        return FALSE;

    OOP_GetAttr(o, aHidd_Overlay_Frame, &frame);
    OOP_GetAttr(o, aHidd_Overlay_FrameFormat, &fmt);
    OOP_GetAttr(o, aHidd_Overlay_FrameModulo, &mod);
    OOP_GetAttr(o, aHidd_Overlay_SrcWidth, &sw);
    OOP_GetAttr(o, aHidd_Overlay_SrcHeight, &sh);

    memset(&desc, 0, sizeof(desc));
    desc.ovl_Phys   = (ULONG)frame;
    desc.ovl_Pitch  = mod;
    desc.ovl_Width  = sw;
    desc.ovl_Height = sh;
    desc.ovl_X      = msg->x;
    desc.ovl_Y      = msg->y;
    desc.ovl_DestW  = msg->width;
    desc.ovl_DestH  = msg->height;
    desc.ovl_Flags  = VC4GFX_OVL_NOWAIT;
    if (fmt == vHidd_StdPixFmt_RGB16_LE)
        desc.ovl_Flags |= VC4GFX_OVL_RGB565;
    else if (fmt == vHidd_StdPixFmt_RGB15_LE)
        desc.ovl_Flags |= VC4GFX_OVL_RGB555;

    ObtainSemaphore(&xsd->vcsd_OvlLock);
    e = vcgfx_ovl_find(xsd, o);
    was = e && e->ov_Shown;
    if (was && (e->ov_BM == msg->bitMap)
        && !memcmp(&e->ov_Desc, &desc, sizeof(desc)))
    {
        /* Up as asked; the plane scans the new pixels in place */
        shown = TRUE;
    }
    else
        shown = vcgfx_ovl_show(xsd, o, msg->bitMap, &desc);
    ReleaseSemaphore(&xsd->vcsd_OvlLock);

    if (shown != was)
        D(bug("[VideoCoreGfxOverlay] %s: %lux%lu fmt %lu -> %ldx%ld at %ld,%ld\n",
              shown ? "on the plane" : "refused, blitting", (unsigned long)sw,
              (unsigned long)sh, (unsigned long)fmt, (long)msg->width,
              (long)msg->height, (long)msg->x, (long)msg->y));
    return shown;
}

VOID VideoCoreGfxOverlay__Hidd_Overlay__Hide(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);

    ObtainSemaphore(&xsd->vcsd_OvlLock);
    ovl_release(xsd, o);
    ReleaseSemaphore(&xsd->vcsd_OvlLock);
}

APTR VideoCoreGfxOverlay__Hidd_Overlay__AllocScanout(OOP_Class *cl, OOP_Object *o,
                                                     struct pHidd_Overlay_AllocScanout *msg)
{
    struct VideoCoreGfxOverlayData *data = OOP_INST_DATA(cl, o);
    ULONG i;

    for (i = 0; i < 2; i++)
    {
        if (!data->sb[i].sb_Raw)
            return vcgfx_scanout_alloc(XSD(cl), &data->sb[i], msg->size)
                ? data->sb[i].sb_Pixels : NULL;
    }
    return NULL;
}

VOID VideoCoreGfxOverlay__Hidd_Overlay__FreeScanout(OOP_Class *cl, OOP_Object *o,
                                                    struct pHidd_Overlay_FreeScanout *msg)
{
    struct VideoCoreGfxOverlayData *data = OOP_INST_DATA(cl, o);
    ULONG i;

    for (i = 0; i < 2; i++)
    {
        if (data->sb[i].sb_Raw && (data->sb[i].sb_Pixels == msg->memory))
            vcgfx_scanout_free(XSD(cl), &data->sb[i]);
    }
}
