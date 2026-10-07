/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Bitmap class for Amdgpu hidd: the framebuffer on screen.
*/

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/gfx.h>
#include <oop/oop.h>

#include "amdgpu_intern.h"

#include LC_LIBDEFS_FILE

/* Don't initialize static variables with "=0", otherwise they go into DATA segment */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddAmdgpuAttrBase;
static OOP_AttrBase HiddAmdgpuBitMapAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap,              &HiddBitMapAttrBase             },
    { IID_Hidd_PixFmt,              &HiddPixFmtAttrBase             },
    { IID_Hidd_Gfx,                 &HiddGfxAttrBase                },
    { IID_Hidd_Sync,                &HiddSyncAttrBase               },
    /* Private bases */
    { IID_Hidd_Amdgpu,          &HiddAmdgpuAttrBase         },
    { IID_Hidd_AmdgpuBitMap,    &HiddAmdgpuBitMapAttrBase   },
    { NULL,                         NULL                            }
};

#define DEBUGNAME "[Amdgpu:OnBitMap]"
#define MNAME_ROOT(x) AmdgpuOnBM__Root__ ## x
#define MNAME_BM(x) AmdgpuOnBM__Hidd_BitMap__ ## x

#define OnBitmap 1
#include "amdgpu_bitmap_common.c"

/*
  include our debug overides after bitmap_common incase it sets its own values...
 */
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

/*
 * A scanout buffer of the bitmap's size, shown in the mode of that size.
 *
 * The old buffer goes first: on a host without guest backed objects the
 * scanout buffer has to sit in the (small) video RAM, and two of them
 * rarely fit. Should the new one fail, the old size is put back.
 */
static BOOL AmdgpuOnBM_ShowSize(OOP_Class *cl, struct BitmapData *data, ULONG width, ULONG height)
{
    struct Amdgpu_KMS *kms = &XSD(cl)->kms;
    drmModeModeInfoPtr mode;

    mode = Amdgpu_KMS_FindMode(kms, width, height);
    if (!mode)
    {
        bug(DEBUGNAME " %s: no %ux%u mode\n", __func__, width, height);
        return FALSE;
    }

    /*
     * The buffer lives in the bitmap's own data: the KMS layer remembers
     * the framebuffer it is scanning out by address.
     */
    if (!Amdgpu_KMS_CreateFB(kms, &data->fb, width, height, AMDGPU_FB_BPP, AMDGPU_FB_DEPTH))
        return FALSE;

    if (!Amdgpu_KMS_SetMode(kms, &data->fb, mode))
    {
        Amdgpu_KMS_DestroyFB(kms, &data->fb);
        return FALSE;
    }

    data->mode = mode;
    data->VideoData = data->fb.map;
    data->pitch = data->fb.pitch;
    data->width = width;
    data->height = height;

    /* the crtc is now on a different buffer; the whole of it is stale */
    Amdgpu_KMS_DirtyFB(kms, &data->fb, NULL);

    return TRUE;
}

static BOOL AmdgpuOnBM_Show(OOP_Class *cl, struct BitmapData *data, ULONG width, ULONG height)
{
    struct Amdgpu_KMS *kms = &XSD(cl)->kms;
    ULONG oldwidth = data->width, oldheight = data->height;
    BOOL hadfb = (data->fb.handle != 0);

    if (hadfb)
    {
        Amdgpu_KMS_DestroyFB(kms, &data->fb);
        data->VideoData = NULL;
    }

    if (AmdgpuOnBM_ShowSize(cl, data, width, height))
        return TRUE;

    if (hadfb && AmdgpuOnBM_ShowSize(cl, data, oldwidth, oldheight))
        bug(DEBUGNAME " %s: %ux%u could not be shown, back at %ux%u\n", __func__, width, height, oldwidth, oldheight);
    else
        bug(DEBUGNAME " %s: no framebuffer can be shown\n", __func__);

    return FALSE;
}

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug(DEBUGNAME " %s()\n", __func__);)

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct BitmapData *data = OOP_INST_DATA(cl, o);
        OOP_Object *pf;
        IPTR width, height, depth;
        HIDDT_ModeID modeid;

        /* clear all data  */
        memset(data, 0, sizeof(struct BitmapData));

        /* Get attr values */
        OOP_GetAttr(o, aHidd_BitMap_Width, &width);
        OOP_GetAttr(o, aHidd_BitMap_Height, &height);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
        OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
        ASSERT (width != 0 && height != 0 && depth != 0);

        data->bpp = depth;
        data->disp = -1;
        data->bytesperpix = AMDGPU_FB_BPP / 8;

        /* We should be able to get modeID from the bitmap */
        OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
        if (modeid == vHidd_ModeID_Invalid)
        {
            OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
            o = NULL;
        }
        else
        {
            OOP_Object *sync, *pixfmt;
            IPTR mode_width = width, mode_height = height;

            InitSemaphore(&data->bmsem);

            /* the framebuffer is the size of the mode it is shown in */
            HIDD_DMEnum_GetMode(XSD(cl)->dmenum, modeid, &sync, &pixfmt);
            if (sync)
            {
                OOP_GetAttr(sync, aHidd_Sync_HDisp, &mode_width);
                OOP_GetAttr(sync, aHidd_Sync_VDisp, &mode_height);
            }
            if (mode_width < width)
                mode_width = width;
            if (mode_height < height)
                mode_height = height;

            if (!AmdgpuOnBM_Show(cl, data, mode_width, mode_height))
            {
                OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
                OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
                o = NULL;
            }
            else
            {
                XSD(cl)->visible = o;
                if (XSD(cl)->hwCursor && XSD(cl)->mouse.visible)
                {
                    Amdgpu_KMS_ShowCursor(&XSD(cl)->kms, TRUE);
                    Amdgpu_KMS_MoveCursor(&XSD(cl)->kms, XSD(cl)->mouse.x, XSD(cl)->mouse.y);
                }
            }
        }
    } /* if created object */

    D(bug(DEBUGNAME " %s: returning 0x%p\n", __func__, o));
    return o;
}

IPTR MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct BitmapData *data =OOP_INST_DATA(cl, o);

    if (FindTagItem(aHidd_BitMap_ModeID, msg->attrList))
    {
        HIDDT_ModeID modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        OOP_Object *sync, *pixfmt;
        IPTR width, height;

        if (modeid == vHidd_ModeID_Invalid)
            return FALSE;

        HIDD_DMEnum_GetMode(XSD(cl)->dmenum, modeid, &sync, &pixfmt);
        OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);

        if (data->width != width || data->height != height)
        {
            LOCK_BITMAP
            if (!AmdgpuOnBM_Show(cl, data, width, height))
            {
                UNLOCK_BITMAP
                return FALSE;
            }
            UNLOCK_BITMAP

            if (XSD(cl)->hwCursor && XSD(cl)->mouse.visible)
            {
                Amdgpu_KMS_ShowCursor(&XSD(cl)->kms, TRUE);
                Amdgpu_KMS_MoveCursor(&XSD(cl)->kms, XSD(cl)->mouse.x, XSD(cl)->mouse.y);
            }
        }
    }

    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/**********  Bitmap::Dispose()  ***********************************/

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    D(bug(DEBUGNAME " %s()\n", __func__);)

    if (XSD(cl)->visible == o)
        XSD(cl)->visible = NULL;
    if (data->fb.handle)
        Amdgpu_KMS_DestroyFB(&XSD(cl)->kms, &data->fb);

    OOP_DoSuperMethod(cl, o, msg);
}

/*** init_onbmclass *********************************************************/

static int AmdgpuOnBM_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug(DEBUGNAME " %s()\n", __func__);)

    ReturnInt("AmdgpuOnBM_Init", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*** free_bitmapclass *********************************************************/

static int AmdgpuOnBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug(DEBUGNAME " %s()\n", __func__);)

    OOP_ReleaseAttrBases(attrbases);

    ReturnInt("AmdgpuOnBM_Expunge", int, TRUE);
}

ADD2INITLIB(AmdgpuOnBM_Init, 0)
ADD2EXPUNGELIB(AmdgpuOnBM_Expunge, 0)
