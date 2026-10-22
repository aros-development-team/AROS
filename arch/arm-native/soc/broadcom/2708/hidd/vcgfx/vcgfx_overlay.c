/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: cgxvideo overlays on the HVS overlay plane.

    The base class keeps the frame in our scanout memory, so the plane
    scans it in place: the 16-bit RGB formats straight from what the
    client writes, YUV after the base class has converted it. A layer
    showing on the plane takes it from a gallium driver, which falls back
    to blitting until the layer hides again.
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

/* Take the plane down if this overlay holds it; caller has vcsd_OvlLock. */
static void ovl_release(struct VideoCoreGfx_staticdata *xsd, OOP_Object *o)
{
    if (xsd->vcsd_OvlClient == o)
    {
        if (xsd->vcsd_OvlShown)
            vc4_hvs_overlay(xsd, NULL, NULL);
        xsd->vcsd_OvlShown  = FALSE;
        xsd->vcsd_OvlClient = NULL;
    }
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
    was = (xsd->vcsd_OvlClient == o) && xsd->vcsd_OvlShown;
    if ((xsd->vcsd_OvlClient == o) && xsd->vcsd_OvlShown
        && (xsd->vcsd_OvlBM == msg->bitMap)
        && !memcmp(&xsd->vcsd_OvlDesc, &desc, sizeof(desc)))
    {
        /* Up as asked; the plane scans the new pixels in place */
        shown = TRUE;
    }
    else if ((shown = vc4_hvs_overlay(xsd, &desc, msg->bitMap)))
    {
        xsd->vcsd_OvlDesc   = desc;
        xsd->vcsd_OvlBM     = msg->bitMap;
        xsd->vcsd_OvlBMData = OOP_INST_DATA(bmcl, msg->bitMap);
        xsd->vcsd_OvlShown  = TRUE;
        xsd->vcsd_OvlClient = o;
    }
    else
    {
        /* Whatever the attempt left on the plane goes */
        vc4_hvs_overlay(xsd, NULL, NULL);
        xsd->vcsd_OvlShown  = FALSE;
        xsd->vcsd_OvlClient = NULL;
    }
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

    if (data->sb.sb_Raw || !vcgfx_scanout_alloc(XSD(cl), &data->sb, msg->size))
        return NULL;
    return data->sb.sb_Pixels;
}

VOID VideoCoreGfxOverlay__Hidd_Overlay__FreeScanout(OOP_Class *cl, OOP_Object *o,
                                                    struct pHidd_Overlay_FreeScanout *msg)
{
    struct VideoCoreGfxOverlayData *data = OOP_INST_DATA(cl, o);

    vcgfx_scanout_free(XSD(cl), &data->sb);
}
