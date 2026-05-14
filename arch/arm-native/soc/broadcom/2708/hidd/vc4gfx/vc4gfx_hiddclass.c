/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 Gfx Hidd Class.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <graphics/view.h>
#include <hardware/custom.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <clib/alib_protos.h>
#include <string.h>

#include <proto/mbox.h>

#include "vc4gfx_hidd.h"
#include "vc4gfx_hardware.h"

#ifdef MBoxBase
#undef MBoxBase
#endif
#define MBoxBase      xsd->vcsd_MBoxBase

#include LC_LIBDEFS_FILE

#define MNAME_ROOT(x) VideoCoreGfx__Root__ ## x
#define MNAME_GFX(x) VideoCoreGfx__Hidd_Gfx__ ## x

#define SYNCTAGS_SIZE (11 * sizeof(struct TagItem))

APTR FNAME_SUPPORT(GenModeArray)(OOP_Class *cl, OOP_Object *o, struct List *modelist, struct TagItem *fmts)
{
    APTR                modearray = NULL;
    struct TagItem      *ma_syncs = NULL, *ma_synctags = NULL;
    int                 i, fmtcount = 0, modecount = 0;
    struct DisplayMode  *modecurrent;

    /* quickly count fmts and modes */
    while (fmts[fmtcount].ti_Tag == aHidd_Gfx_PixFmtTags)
        fmtcount++;

    ForeachNode(modelist, modecurrent)
    {
        modecount++;
    }

    D(bug("[VideoCoreGfx] %s: %d PixFmts, %d SyncModes\n", __PRETTY_FUNCTION__, fmtcount, modecount));

    /* build our table .. */
    if (fmtcount && modecount)
    {
        if ((modearray = AllocVec((fmtcount * sizeof(struct TagItem)) + ((modecount + 1) * sizeof(struct TagItem)) + (modecount * SYNCTAGS_SIZE), MEMF_PUBLIC)) != NULL)
        {
            struct TagItem *ma_fmts = (struct TagItem  *)modearray;

            D(bug("[VideoCoreGfx] %s: PixFmt's @ 0x%p\n", __PRETTY_FUNCTION__, modearray));

            for (i = 0; i < fmtcount; i ++)
            {
                D(bug("[VideoCoreGfx] %s: PixFmt #%d @ 0x%p\n", __PRETTY_FUNCTION__, i, fmts[i].ti_Data));
                ma_fmts[i].ti_Tag = aHidd_Gfx_PixFmtTags;
                ma_fmts[i].ti_Data = fmts[i].ti_Data;
            }
            ma_syncs = (struct TagItem *)&ma_fmts[fmtcount];
            D(bug("[VideoCoreGfx] %s: SyncMode's @ 0x%p\n", __PRETTY_FUNCTION__, ma_syncs));
            ma_synctags = (struct TagItem  *)&ma_syncs[modecount + 1];
            i = 0;
            ForeachNode(modelist, modecurrent)
            {
                D(bug("[VideoCoreGfx] %s: SyncMode #%d Tags @ 0x%p\n", __PRETTY_FUNCTION__, i, ma_synctags));

                ma_syncs[i].ti_Tag = aHidd_Gfx_SyncTags;
                ma_syncs[i].ti_Data = (IPTR)ma_synctags;

                ma_synctags[0].ti_Tag = aHidd_Sync_PixelClock;
                ma_synctags[0].ti_Data = modecurrent->dm_clock * 1000;
                ma_synctags[1].ti_Tag = aHidd_Sync_HDisp;
                ma_synctags[1].ti_Data = modecurrent->dm_hdisp;
                ma_synctags[2].ti_Tag = aHidd_Sync_HSyncStart;
                ma_synctags[2].ti_Data = modecurrent->dm_hstart;
                ma_synctags[3].ti_Tag = aHidd_Sync_HSyncEnd;
                ma_synctags[3].ti_Data = modecurrent->dm_hend;
                ma_synctags[4].ti_Tag = aHidd_Sync_HTotal;
                ma_synctags[4].ti_Data = modecurrent->dm_htotal;
                ma_synctags[5].ti_Tag = aHidd_Sync_VDisp;
                ma_synctags[5].ti_Data = modecurrent->dm_vdisp;
                ma_synctags[6].ti_Tag = aHidd_Sync_VSyncStart;
                ma_synctags[6].ti_Data = modecurrent->dm_vstart;
                ma_synctags[7].ti_Tag = aHidd_Sync_VSyncEnd;
                ma_synctags[7].ti_Data = modecurrent->dm_vend;
                ma_synctags[8].ti_Tag = aHidd_Sync_VTotal;
                ma_synctags[8].ti_Data = modecurrent->dm_vtotal;
                ma_synctags[9].ti_Tag = aHidd_Sync_Description;
                ma_synctags[9].ti_Data = (IPTR)modecurrent->dm_descr;
                ma_synctags[10].ti_Tag = TAG_DONE;

                ma_synctags = (struct TagItem  *)&ma_synctags[11];
                i++;
            }
            ma_syncs[i].ti_Tag = TAG_DONE;
        }
    }
#if defined(DEBUGMODEARRAY)
    if (modearray)
    {
        ma_syncs = (struct TagItem *)modearray;
        while (ma_syncs->ti_Tag != TAG_DONE)
        {
            D(bug("[VideoCoreGfx] %s: 0x%p: %08x, %08x\n", __PRETTY_FUNCTION__, ma_syncs, ma_syncs->ti_Tag, ma_syncs->ti_Data));
            ma_syncs++;
        }
    }
#endif
    return (APTR)modearray;
}

void FNAME_SUPPORT(DestroyModeArray)(struct List *modelist, APTR modearray)
{
    D(bug("[VideoCoreGfx] %s()\n", __PRETTY_FUNCTION__));
}

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    //struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    OOP_Object                  *self = NULL;

    struct TagItem gfxmsg_tags[] =
    {
        { aHidd_Gfx_ModeTags    , (IPTR)NULL   },
        { aHidd_Name            , (IPTR)"vc4gfx.hidd"     },
        { aHidd_HardwareName    , (IPTR)"VideoCore4 Display Adaptor"   },
        { aHidd_ProducerName    , (IPTR)"Broadcom Corporation"  },
        { TAG_MORE              , (IPTR)msg->attrList       }
    };
    struct pRoot_New            gfxmsg_New;

    struct List                 vc_modelist;
    APTR                        vc_modearray, vc_pixfmts;

    EnterFunc(bug("VideoCoreGfx::New()\n"));

    NewList(&vc_modelist);

    FNAME_SUPPORT(SDTV_SyncGen)(&vc_modelist, cl);
    FNAME_SUPPORT(HDMI_SyncGen)(&vc_modelist, cl);
    vc_pixfmts = FNAME_SUPPORT(GenPixFmts)(cl);

    if ((vc_modearray = FNAME_SUPPORT(GenModeArray)(cl, o, &vc_modelist, (struct TagItem *)vc_pixfmts)) != NULL)
    {
        D(bug("[VideoCoreGfx] VideoCoreGfx::New: Generated Mode Array @ 0x%p\n", vc_modearray));

        gfxmsg_tags[0].ti_Data = (IPTR)vc_modearray;

        gfxmsg_New.mID = msg->mID;
        gfxmsg_New.attrList = gfxmsg_tags;
        msg = &gfxmsg_New;

        D(bug("[VideoCoreGfx] VideoCoreGfx::New: Creating object [cl:0x%p, o:0x%p, msg:0x%p]\n", cl, o, msg));

        if ((self = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) != NULL)
        {
            D(bug("[VideoCoreGfx] VideoCoreGfx::New: Storing reference to self in staticdata\n"));
            XSD(cl)->vcsd_VideoCoreGfxInstance = self;
        }
        FNAME_SUPPORT(DestroyModeArray)(&vc_modelist, vc_modearray);
    }

    ReturnPtr("VideoCoreGfx::New: Obj", OOP_Object *, self);
}

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[VideoCoreGfx] VideoCoreGfx::Dispose()\n"));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;

//    D(bug("[VideoCoreGfx] VideoCoreGfx::Get()\n"));

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_MemorySize:
            *msg->storage = (IPTR)(XSD(cl)->vcsd_GPUMemManage.mhe_MemHeader.mh_Upper - XSD(cl)->vcsd_GPUMemManage.mhe_MemHeader.mh_Lower);
            found = TRUE;
            break;

        case aoHidd_Gfx_HWSpriteTypes:
            *msg->storage = XSD(cl)->vcsd_CurBuf ? vHidd_SpriteType_DirectColor : 0;
            found = TRUE;
            break;
        }
    }
    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

int FNAME_SUPPORT(InitCursor)(struct VideoCoreGfx_staticdata *xsd)
{
    /* Allocate a 64x64 ARGB scratch buffer in GPU memory for cursor pixels.
     * The firmware reads cursor pixels from this buffer each time we issue
     * SETCURSORINFO, so it must remain locked for the lifetime of the driver.
     * Use VCMEM_DIRECT (uncached, 0xC alias) so the CPU can write pixels
     * via the MMU's uncached mapping - VCMEM_NORMAL is rejected by the
     * firmware when the request comes from ARM (videocore.h:108).
     *
     * ALLOCMEM tag layout: [tag, value_buf_size, code, size, alignment,
     * flags] - the request value buffer is 3 u32 (size/align/flags),
     * the response writes a 1 u32 handle into the first slot.
     */
    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(10 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_ALLOCMEM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(12);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(12);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(VC4_CURSOR_BUF_BYTES);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG(VCMEM_DIRECT);
    xsd->vcsd_MBoxMessage[8] = 0;
    xsd->vcsd_MBoxMessage[9] = 0;

    if (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
        == (volatile unsigned int *)-1)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }
    xsd->vcsd_CurBufHandle = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
    if (!xsd->vcsd_CurBufHandle)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }

    /* LOCKMEM tag layout: [tag, value_buf_size, code, handle] - request
     * value is the handle, response overwrites it with the bus address.
     */
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_LOCKMEM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(xsd->vcsd_CurBufHandle);
    xsd->vcsd_MBoxMessage[6] = 0;
    xsd->vcsd_MBoxMessage[7] = 0;

    if (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
        == (volatile unsigned int *)-1)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }

    xsd->vcsd_CurBufBus = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
    VC4_MBOX_UNLOCK(xsd);
    xsd->vcsd_CurVisible = FALSE;

    /* The firmware should hand back a GPU bus address with the alias
     * bits set (VCMEM_DIRECT == 0xC, VCMEM_NORMAL == 0x4). QEMU's
     * mailbox emulation may return a bare low-RAM address instead,
     * which overlaps with CPU-side allocations and would corrupt
     * unrelated state. Treat anything outside the GPU alias range as
     * "no HW cursor available".
     */
    if ((xsd->vcsd_CurBufBus & 0xC0000000) == 0)
    {
        xsd->vcsd_CurBuf = NULL;
        return FALSE;
    }
    xsd->vcsd_CurBuf = (APTR)(xsd->vcsd_CurBufBus & 0x3fffffff);

    if (xsd->vcsd_CurBuf == NULL)
        return FALSE;

    D(bug("[VideoCoreGfx] HW cursor buffer @ 0x%p (bus 0x%08x)\n",
        xsd->vcsd_CurBuf, xsd->vcsd_CurBufBus));
    return TRUE;
}

VOID MNAME_GFX(NominalDimensions)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NominalDimensions *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);

    /* Hand back the panel's native HDMI resolution if we know it. Falls
     * through to the superclass (which returns AROS_NOMINAL_*) when GETRES
     * couldn't tell us - e.g. headless boot or unsupported firmware.
     */
    if (xsd->vcsd_NativeWidth && xsd->vcsd_NativeHeight)
    {
        if (msg->width)
            *msg->width  = xsd->vcsd_NativeWidth;
        if (msg->height)
            *msg->height = xsd->vcsd_NativeHeight;
        if (msg->depth)
            *msg->depth  = 24;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL MNAME_GFX(SetCursorShape)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    IPTR width = 0, height = 0;

    if (!xsd->vcsd_CurBuf)
        return FALSE;

    if (msg->shape == NULL)
    {
        /* Hide and forget the current shape. */
        xsd->vcsd_CurVisible = FALSE;
        VC4_MBOX_LOCK(xsd);
        xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
        xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
        xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORSTATE);
        xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(16);
        xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(16);
        xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(0);     /* enable=0 */
        xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG(0);
        xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG(0);
        xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(0);
        xsd->vcsd_MBoxMessage[9] = 0;
        MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
        VC4_MBOX_UNLOCK(xsd);
        return TRUE;
    }

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    if (width == 0 || height == 0 ||
        width > VC4_CURSOR_MAX_W || height > VC4_CURSOR_MAX_H)
        return FALSE;

    /* The VideoCore firmware wants cursor pixels as bytes R,G,B,A in
     * memory (mailbox SET_CURSOR_INFO format=0), which corresponds to
     * AROS' vHidd_StdPixFmt_RGBA32. HIDD's GetImage handles conversion.
     */
    HIDD_BM_GetImage(msg->shape, (UBYTE *)xsd->vcsd_CurBuf,
                     width * 4, 0, 0, width, height, vHidd_StdPixFmt_RGBA32);

    xsd->vcsd_CurWidth  = width;
    xsd->vcsd_CurHeight = height;
    xsd->vcsd_CurHotX   = msg->xoffset;
    xsd->vcsd_CurHotY   = msg->yoffset;

    /* SETCURSORINFO: width, height, format(0), buf, hot_x, hot_y */
    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(12 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORINFO);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(24);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(24);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(width);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG(height);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG(0);
    xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(xsd->vcsd_CurBufBus);
    xsd->vcsd_MBoxMessage[9] = AROS_LE2LONG(xsd->vcsd_CurHotX);
    xsd->vcsd_MBoxMessage[10] = AROS_LE2LONG(xsd->vcsd_CurHotY);
    xsd->vcsd_MBoxMessage[11] = 0;

    if (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
        == (volatile unsigned int *)-1)
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }
    {
        BOOL ok = (AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]) == 0);
        VC4_MBOX_UNLOCK(xsd);
        return ok;
    }
}

BOOL MNAME_GFX(SetCursorPos)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);

    if (!xsd->vcsd_CurBuf)
        return FALSE;

    xsd->vcsd_CurX = msg->x;
    xsd->vcsd_CurY = msg->y;

    if (!xsd->vcsd_CurVisible)
        return TRUE;

    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORSTATE);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(1);     /* enable */
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG((ULONG)msg->x);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG((ULONG)msg->y);
    xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(1);     /* 1 = framebuffer coords (firmware scales) */
    xsd->vcsd_MBoxMessage[9] = 0;

    MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    VC4_MBOX_UNLOCK(xsd);
    return TRUE;
}

VOID MNAME_GFX(SetCursorVisible)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);

    if (!xsd->vcsd_CurBuf)
        return;

    xsd->vcsd_CurVisible = msg->visible ? TRUE : FALSE;

    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORSTATE);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(msg->visible ? 1 : 0);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG((ULONG)xsd->vcsd_CurX);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG((ULONG)xsd->vcsd_CurY);
    xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(1);     /* 1 = framebuffer coords */
    xsd->vcsd_MBoxMessage[9] = 0;

    MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    VC4_MBOX_UNLOCK(xsd);
}

OOP_Object *MNAME_GFX(CreateObject)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    if (msg->cl == XSD(cl)->vcsd_basebm)
    {
        BOOL displayable;
        BOOL framebuffer;
        struct TagItem newbm_tags[2] =
        {
            {TAG_IGNORE, 0                  },
            {TAG_MORE  , (IPTR)msg->attrList}
        };
        struct pHidd_Gfx_CreateObject newbm_msg;

        displayable = (BOOL)GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
        framebuffer = (BOOL)GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
        if (framebuffer)
        {
            newbm_tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
            newbm_tags[0].ti_Data = (IPTR)XSD(cl)->vcsd_VideoCoreGfxOnBMClass;
        }
        else
        {
            /* Non-displayable friends of our bitmaps are plain chunky bitmaps */
            OOP_Object *friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);

            if (displayable || (friend && (OOP_OCLASS(friend) == XSD(cl)->vcsd_VideoCoreGfxOnBMClass)))
            {
                newbm_tags[0].ti_Tag  = aHidd_BitMap_ClassID;
                newbm_tags[0].ti_Data = (IPTR)CLID_Hidd_ChunkyBM;
            }
        }

        newbm_msg.mID = msg->mID;
        newbm_msg.cl = msg->cl;
        newbm_msg.attrList = newbm_tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&newbm_msg);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return object;
}
