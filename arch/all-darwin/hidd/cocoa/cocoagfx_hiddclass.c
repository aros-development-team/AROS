/*
 * cocoagfx_hiddclass.c - Cocoa GFX HIDD for AROS on macOS Apple Silicon
 */

#include <aros/debug.h>
#include <hidd/gfx.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "cocoa_intern.h"
#include "hostinterface.h"

extern struct HostInterface *HostIFace;

/* ======== GFX class: Root::New ======== */

OOP_Object *CocoaGfx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] = {
        { aHidd_PixFmt_ColorModel,     vHidd_ColorModel_TrueColor },
        { aHidd_PixFmt_RedShift,       8  },
        { aHidd_PixFmt_GreenShift,     16 },
        { aHidd_PixFmt_BlueShift,      24 },
        { aHidd_PixFmt_AlphaShift,     0  },
        { aHidd_PixFmt_RedMask,        0x00FF0000 },
        { aHidd_PixFmt_GreenMask,      0x0000FF00 },
        { aHidd_PixFmt_BlueMask,       0x000000FF },
        { aHidd_PixFmt_AlphaMask,      0xFF000000 },
        { aHidd_PixFmt_Depth,          32 },
        { aHidd_PixFmt_BitsPerPixel,   32 },
        { aHidd_PixFmt_BytesPerPixel,  4  },
        { aHidd_PixFmt_StdPixFmt,      vHidd_StdPixFmt_BGRA32 },
        { aHidd_PixFmt_BitMapType,     vHidd_BitMapType_Chunky },
        { TAG_DONE, 0 }
    };

    struct TagItem synctags[] = {
        { aHidd_Sync_HDisp,       640 },
        { aHidd_Sync_VDisp,       480 },
        { aHidd_Sync_PixelClock,  25175000 },
        { aHidd_Sync_HTotal,      800 },
        { aHidd_Sync_VTotal,      525 },
        { aHidd_Sync_HSyncStart,  656 },
        { aHidd_Sync_HSyncEnd,    752 },
        { aHidd_Sync_VSyncStart,  490 },
        { aHidd_Sync_VSyncEnd,    492 },
        { aHidd_Sync_Description, (IPTR)"Cocoa:640x480" },
        { TAG_DONE, 0 }
    };

    struct TagItem modetags[] = {
        { aHidd_Gfx_PixFmtTags, (IPTR)pftags    },
        { aHidd_Gfx_SyncTags,   (IPTR)synctags  },
        { TAG_DONE, 0 }
    };

    struct TagItem msgtags[] = {
        { aHidd_Gfx_ModeTags,    (IPTR)modetags },
        { aHidd_Name,            (IPTR)"Cocoa"  },
        { aHidd_HardwareName,    (IPTR)"macOS Cocoa Display" },
        { aHidd_ProducerName,    (IPTR)"AROS Development Team" },
        { TAG_MORE,              (IPTR)msg->attrList },
        { TAG_DONE, 0 }
    };

    struct pRoot_New supermsg;
    supermsg.mID = msg->mID;
    supermsg.attrList = msgtags;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&supermsg);

    bug("[CocoaGfx] New: %p\n", o);
    return o;
}

/* ======== GFX class: Root::Get ======== */

VOID CocoaGfx__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_GFX_ATTR(msg->attrID, idx)) {
        switch (idx) {
        case aoHidd_Gfx_IsWindowed:
            *msg->storage = TRUE;
            return;
        case aoHidd_Gfx_DriverName:
            *msg->storage = (IPTR)"Cocoa";
            return;
        case aoHidd_Gfx_NoFrameBuffer:
            *msg->storage = FALSE;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* ======== GFX class: Hidd_Gfx::CreateObject ======== */

OOP_Object *CocoaGfx__Hidd_Gfx__CreateObject(OOP_Class *cl, OOP_Object *o,
                                               struct pHidd_Gfx_CreateObject *msg)
{
    struct CocoaGfx_staticdata *csd = CSD(cl);

    if (msg->cl == OOP_FindClass(CLID_Hidd_BitMap)) {
        BOOL is_fb = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
        HIDDT_ModeID modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID,
                                                        vHidd_ModeID_Invalid, msg->attrList);

        if (is_fb || (modeid != vHidd_ModeID_Invalid)) {
            struct TagItem bmtags[] = {
                { aHidd_BitMap_ClassPtr,    (IPTR)csd->bmclass },
                { TAG_MORE,                (IPTR)msg->attrList },
                { TAG_DONE, 0 }
            };

            struct pHidd_Gfx_CreateObject supermsg;
            supermsg.mID = msg->mID;
            supermsg.cl = msg->cl;
            supermsg.attrList = bmtags;

            bug("[CocoaGfx] CreateObject: fb=%d modeid=0x%x\n", is_fb, modeid);
            return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&supermsg);
        }
    }

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* ======== GFX class: Hidd_Gfx::Show ======== */

OOP_Object *CocoaGfx__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o,
                                       struct pHidd_Gfx_Show *msg)
{
    /* Refresh the Cocoa display */
    if (HostIFace && HostIFace->cocoa_display_refresh)
        HostIFace->cocoa_display_refresh();

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* ======== Method tables ======== */

static struct OOP_MethodDescr CocoaGfx_Root_descr[] = {
    { (OOP_MethodFunc)CocoaGfx__Root__New,     moRoot_New     },
    { (OOP_MethodFunc)CocoaGfx__Root__Get,     moRoot_Get     },
    { NULL, 0 }
};

static struct OOP_MethodDescr CocoaGfx_Gfx_descr[] = {
    { (OOP_MethodFunc)CocoaGfx__Hidd_Gfx__CreateObject, moHidd_Gfx_CreateObject },
    { (OOP_MethodFunc)CocoaGfx__Hidd_Gfx__Show,        moHidd_Gfx_Show         },
    { NULL, 0 }
};

struct OOP_InterfaceDescr CocoaGfx_ifdescr[] = {
    { CocoaGfx_Root_descr, IID_Root,     2 },
    { CocoaGfx_Gfx_descr,  IID_Hidd_Gfx, 2 },
    { NULL, NULL }
};
