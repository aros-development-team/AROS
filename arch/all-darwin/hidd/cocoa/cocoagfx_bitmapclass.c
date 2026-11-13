/*
 * cocoagfx_bitmapclass.c - Cocoa BitMap HIDD for AROS on macOS Apple Silicon
 *
 * This bitmap class wraps the IOSurface framebuffer memory directly.
 */

#include <aros/debug.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>


#include "cocoa_intern.h"
#include "hostinterface.h"

extern struct HostInterface *HostIFace;

/* ======== BitMap::New ======== */

OOP_Object *CocoaBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL is_fb = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

    /* Just call super - ChunkyBM allocates its own buffer.
     * We copy from ChunkyBM's buffer to the IOSurface in UpdateRect. */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o) {
        struct CocoaBMData *data = OOP_INST_DATA(cl, o);
        data->is_fb = is_fb;
    }
    return o;
}

/* ======== BitMap::Dispose ======== */

VOID CocoaBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, msg);
}

/* ======== BitMap::Get ======== */

VOID CocoaBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* ======== BitMap::UpdateRect ======== */

VOID CocoaBM__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o,
                                       struct pHidd_BitMap_UpdateRect *msg)
{
    struct CocoaBMData *data = OOP_INST_DATA(cl, o);
    struct CocoaGfx_staticdata *csd = CSD(cl);

    if (data->is_fb && csd->fb_base) {
        /* Copy the updated region from ChunkyBM's buffer to the IOSurface */
        IPTR srcbuf = 0, bpr = 0, width = 0, height = 0;
        OOP_GetAttr(o, aHidd_ChunkyBM_Buffer, &srcbuf);
        OOP_GetAttr(o, aHidd_BitMap_BytesPerRow, &bpr);
        OOP_GetAttr(o, aHidd_BitMap_Width, &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);

        if (srcbuf && srcbuf != (IPTR)csd->fb_base) {
            /* Blit the dirty rect */
            UBYTE *src = (UBYTE *)srcbuf + msg->y * bpr + msg->x * 4;
            UBYTE *dst = (UBYTE *)csd->fb_base + msg->y * csd->fb_pitch + msg->x * 4;
            ULONG copy_width = msg->width * 4;
            LONG rows = msg->height;

            while (rows-- > 0) {
                CopyMem(src, dst, copy_width);
                src += bpr;
                dst += csd->fb_pitch;
            }
        }

        /* Trigger Cocoa display refresh */
        if (HostIFace && HostIFace->cocoa_display_refresh)
            HostIFace->cocoa_display_refresh();
    }
}

/* ======== Method tables ======== */

static struct OOP_MethodDescr CocoaBM_Root_descr[] = {
    { (OOP_MethodFunc)CocoaBM__Root__New,     moRoot_New     },
    { (OOP_MethodFunc)CocoaBM__Root__Dispose, moRoot_Dispose },
    { (OOP_MethodFunc)CocoaBM__Root__Get,     moRoot_Get     },
    { NULL, 0 }
};

static struct OOP_MethodDescr CocoaBM_BitMap_descr[] = {
    { (OOP_MethodFunc)CocoaBM__Hidd_BitMap__UpdateRect, moHidd_BitMap_UpdateRect },
    { NULL, 0 }
};

struct OOP_InterfaceDescr CocoaBM_ifdescr[] = {
    { CocoaBM_Root_descr,   IID_Root,       3 },
    { CocoaBM_BitMap_descr, IID_Hidd_BitMap, 1 },
    { NULL, NULL }
};
