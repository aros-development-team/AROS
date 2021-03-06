/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.

    Desc: Class for Headless.
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hardware/custom.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <clib/alib_protos.h>
#include <string.h>

#include "headlessgfx_hidd.h"

#include LC_LIBDEFS_FILE

OOP_Object *HeadlessGfx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] =
    {
        { aHidd_PixFmt_RedShift     , 0                       }, /* 0 */
        { aHidd_PixFmt_GreenShift   , 0                       }, /* 1 */
        { aHidd_PixFmt_BlueShift    , 0                       }, /* 2 */
        { aHidd_PixFmt_AlphaShift   , 0                       }, /* 3 */
        { aHidd_PixFmt_RedMask      , 0x000000FC              }, /* 4 */
        { aHidd_PixFmt_GreenMask    , 0x0000FC00              }, /* 5 */
        { aHidd_PixFmt_BlueMask     , 0x00FC0000              }, /* 6 */
        { aHidd_PixFmt_AlphaMask    , 0x00000000              }, /* 7 */
        { aHidd_PixFmt_ColorModel   , vHidd_ColorModel_Palette}, /* 8 */
        { aHidd_PixFmt_Depth        , 4                       }, /* 9 */
        { aHidd_PixFmt_BytesPerPixel, 1                       }, /* 10 */
        { aHidd_PixFmt_BitsPerPixel , 4                       }, /* 11 */
        { aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_LUT8    }, /* 12 */
        { aHidd_PixFmt_CLUTShift    , 0                       }, /* 13 */
        { aHidd_PixFmt_CLUTMask     , 0x0f                    }, /* 14 */
        { aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE                  , 0UL                     }
    };
    struct TagItem sync_mode[] =
    {
        {aHidd_Sync_PixelClock,         0                       },
        {aHidd_Sync_HTotal,             1024                    },
        {aHidd_Sync_HDisp,              1024                    },
        {aHidd_Sync_VDisp,              768                     },
        {aHidd_Sync_HMax,               16384                   },
        {aHidd_Sync_VMax,               16384                   },
        {aHidd_Sync_Description,        (IPTR)"Headless:%hx%v"  },
        {TAG_DONE,                      0UL                     }
    };
    struct TagItem modetags[] =
    {
        {aHidd_Gfx_PixFmtTags, (IPTR)pftags},
        {aHidd_Gfx_SyncTags,   (IPTR)sync_mode},
        {TAG_DONE, 0UL}
    };
    struct TagItem msgNewTags[] =
    {
        { aHidd_Gfx_ModeTags, (IPTR)modetags},
        { aHidd_Name            , (IPTR)"headlessgfx.hidd"     },
        { aHidd_HardwareName    , (IPTR)"Headless Display Controller"   },
        { aHidd_ProducerName    , (IPTR)"The AROS Dev Team"  },
        { TAG_MORE, 0UL}
    };
    struct pRoot_New msgNew;

    EnterFunc(bug("HeadlessGfx::New()\n"));

    /* Protect against some stupid programmer wishing to
       create one more Headless driver */
    if (XSD(cl)->headlessgfxhidd)
        return NULL;

    if ((msgNewTags[4].ti_Data = (IPTR)msg->attrList) == 0)
        msgNewTags[4].ti_Tag = TAG_DONE;

    msgNew.mID = msg->mID;
    msgNew.attrList = msgNewTags;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&msgNew);
    if (o)
    {
        struct HeadlessGfxHiddData *data = OOP_INST_DATA(cl, o);

        D(bug("Got object from super\n"));
        XSD(cl)->headlessgfxhidd = o;
    }
    ReturnPtr("HeadlessGfx::New", OOP_Object *, o);
}

OOP_Object *HeadlessGfx__Hidd_Gfx__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    D(bug("[Headless] %s()\n", __func__));
    D(bug("[Headless] %s: requested class 0x%p\n", __func__, msg->cl));
    D(bug("[Headless] %s: base bitmap class 0x%p\n", __func__, XSD(cl)->basebm));

    if (msg->cl == XSD(cl)->basebm)
    {
        BOOL displayable;
        struct TagItem tags[2] =
        {
            {TAG_IGNORE, 0                  },
            {TAG_MORE  , (IPTR)msg->attrList}
        };
        struct pHidd_Gfx_CreateObject p;

        displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
        if (displayable)
        {
            /* Only displayable bitmaps are bitmaps of our class */
            tags[0].ti_Tag  = aHidd_BitMap_ClassPtr;
            tags[0].ti_Data = (IPTR)XSD(cl)->bmclass;
            D(bug("[Headless] %s: displayable\n", __func__));
        }
        else
        {
            tags[0].ti_Tag  = aHidd_BitMap_ClassID;
            tags[0].ti_Data = (IPTR)CLID_Hidd_ChunkyBM;
            D(bug("[Headless] %s: using chunkybm\n", __func__));
        }

        p.mID = msg->mID;
        p.cl = msg->cl;
        p.attrList = tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    ReturnPtr("HeadlessGfx::CreateObject", OOP_Object *, object);
}
