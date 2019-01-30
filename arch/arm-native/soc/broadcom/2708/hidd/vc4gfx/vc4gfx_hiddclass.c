/*
    Copyright � 2013-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCM VideoCore4 Gfx Hidd Class.
    Lang: English.
*/

#define DEBUG 1
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

#include "vc4gfx_hidd.h"
#include "vc4gfx_hardware.h"
#include "vc4gfx_bitmap.h"

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
        }
    }
    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *MNAME_GFX(CreateObject)(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    EnterFunc(bug("VideoCoreGfx::CreateObject()\n"));

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
            D(bug("[VideoCoreGfx] VideoCoreGfx::CreateObject: Using OnScreenBM\n"));
            newbm_tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
            newbm_tags[0].ti_Data = (IPTR)XSD(cl)->vcsd_VideoCoreGfxOnBMClass;
        }
        else
        {
            /* Non-displayable friends of our bitmaps are plain chunky bitmaps */
            OOP_Object *friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);

            if (displayable || (friend && (OOP_OCLASS(friend) == XSD(cl)->vcsd_VideoCoreGfxOnBMClass)))
            {
                D(bug("[VideoCoreGfx] VideoCoreGfx::CreateObject: Using OffScreenBM (ChunkyBM)\n"));
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

    ReturnPtr("VideoCoreGfx::CreateObject: Obj", OOP_Object *, object);
}
