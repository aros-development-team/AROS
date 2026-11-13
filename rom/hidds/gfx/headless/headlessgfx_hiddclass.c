/*
    Copyright (C) 2021-2026, The AROS Development Team. All rights reserved.

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

/* Geometry of the single sync this driver registers. The defaults can be
   overridden from the monitor icon's tooltypes; the bounds keep a typo
   there from producing a display nothing can allocate a bitmap for. */
#define HEADLESS_DEF_WIDTH   1024
#define HEADLESS_DEF_HEIGHT  768
#define HEADLESS_MIN_WIDTH   320
#define HEADLESS_MIN_HEIGHT  200
#define HEADLESS_MAX_WIDTH   16384      /* HMax/VMax advertised in the sync */
#define HEADLESS_MAX_HEIGHT  16384

OOP_Object *HeadlessGfx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    /*
     * Pixel formats for every depth this display can expose. This
     * display has no hardware of its own - it exists to be read back
     * and served over VNC - so the deepest format is a 32-bit
     * true-colour surface giving the remote viewer full colour, with
     * shallower formats available for clients that want to trade
     * colour for bandwidth. All shift/mask values are the canonical
     * little-endian layouts from rom/hidds/gfx/stdpixfmts_le.h, so the
     * colour conversion tables agree with what is registered here.
     */
    struct TagItem pftags_lut8[] =
    {
        { aHidd_PixFmt_RedShift     , 0                       },
        { aHidd_PixFmt_GreenShift   , 0                       },
        { aHidd_PixFmt_BlueShift    , 0                       },
        { aHidd_PixFmt_AlphaShift   , 0                       },
        { aHidd_PixFmt_RedMask      , 0x000000FF              },
        { aHidd_PixFmt_GreenMask    , 0x0000FF00              },
        { aHidd_PixFmt_BlueMask     , 0x00FF0000              },
        { aHidd_PixFmt_AlphaMask    , 0x00000000              },
        { aHidd_PixFmt_ColorModel   , vHidd_ColorModel_Palette},
        { aHidd_PixFmt_Depth        , 8                       },
        { aHidd_PixFmt_BytesPerPixel, 1                       },
        { aHidd_PixFmt_BitsPerPixel , 8                       },
        { aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_LUT8    },
        { aHidd_PixFmt_CLUTShift    , 0                       },
        { aHidd_PixFmt_CLUTMask     , 0x000000FF              },
        { aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky },
        { TAG_DONE                  , 0UL                     }
    };
    struct TagItem pftags_rgb15[] =
    {
        { aHidd_PixFmt_RedShift     , 17                      },
        { aHidd_PixFmt_GreenShift   , 22                      },
        { aHidd_PixFmt_BlueShift    , 27                      },
        { aHidd_PixFmt_AlphaShift   , 0                       },
        { aHidd_PixFmt_RedMask      , 0x00007C00              },
        { aHidd_PixFmt_GreenMask    , 0x000003E0              },
        { aHidd_PixFmt_BlueMask     , 0x0000001F              },
        { aHidd_PixFmt_AlphaMask    , 0x00000000              },
        { aHidd_PixFmt_ColorModel   , vHidd_ColorModel_TrueColor},
        { aHidd_PixFmt_Depth        , 15                      },
        { aHidd_PixFmt_BytesPerPixel, 2                       },
        { aHidd_PixFmt_BitsPerPixel , 15                      },
        { aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_RGB15_LE},
        { aHidd_PixFmt_CLUTShift    , 0                       },
        { aHidd_PixFmt_CLUTMask     , 0                       },
        { aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky },
        { TAG_DONE                  , 0UL                     }
    };
    struct TagItem pftags_rgb16[] =
    {
        { aHidd_PixFmt_RedShift     , 16                      },
        { aHidd_PixFmt_GreenShift   , 21                      },
        { aHidd_PixFmt_BlueShift    , 27                      },
        { aHidd_PixFmt_AlphaShift   , 0                       },
        { aHidd_PixFmt_RedMask      , 0x0000F800              },
        { aHidd_PixFmt_GreenMask    , 0x000007E0              },
        { aHidd_PixFmt_BlueMask     , 0x0000001F              },
        { aHidd_PixFmt_AlphaMask    , 0x00000000              },
        { aHidd_PixFmt_ColorModel   , vHidd_ColorModel_TrueColor},
        { aHidd_PixFmt_Depth        , 16                      },
        { aHidd_PixFmt_BytesPerPixel, 2                       },
        { aHidd_PixFmt_BitsPerPixel , 16                      },
        { aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_RGB16_LE},
        { aHidd_PixFmt_CLUTShift    , 0                       },
        { aHidd_PixFmt_CLUTMask     , 0                       },
        { aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky },
        { TAG_DONE                  , 0UL                     }
    };
    struct TagItem pftags_bgra32[] =
    {
        { aHidd_PixFmt_RedShift     , 8                       },
        { aHidd_PixFmt_GreenShift   , 16                      },
        { aHidd_PixFmt_BlueShift    , 24                      },
        { aHidd_PixFmt_AlphaShift   , 0                       },
        { aHidd_PixFmt_RedMask      , 0x00FF0000              },
        { aHidd_PixFmt_GreenMask    , 0x0000FF00              },
        { aHidd_PixFmt_BlueMask     , 0x000000FF              },
        { aHidd_PixFmt_AlphaMask    , 0xFF000000              },
        { aHidd_PixFmt_ColorModel   , vHidd_ColorModel_TrueColor},
        { aHidd_PixFmt_Depth        , 24                      },
        { aHidd_PixFmt_BytesPerPixel, 4                       },
        { aHidd_PixFmt_BitsPerPixel , 32                      },
        { aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_BGRA32  },
        { aHidd_PixFmt_CLUTShift    , 0                       },
        { aHidd_PixFmt_CLUTMask     , 0                       },
        { aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky },
        { TAG_DONE                  , 0UL                     }
    };
    const struct
    {
        ULONG           depth;
        struct TagItem  *pftags;
    } depths[] =
    {
        {  8, pftags_lut8   },
        { 15, pftags_rgb15  },
        { 16, pftags_rgb16  },
        { 24, pftags_bgra32 }
    };
    struct TagItem sync_mode[] =
    {
        /* HTotal/HDisp/VDisp are patched below from the tooltypes -
           keep them at indices 1..3 */
        {aHidd_Sync_PixelClock,         0                       },
        {aHidd_Sync_HTotal,             HEADLESS_DEF_WIDTH      },
        {aHidd_Sync_HDisp,              HEADLESS_DEF_WIDTH      },
        {aHidd_Sync_VDisp,              HEADLESS_DEF_HEIGHT     },
        {aHidd_Sync_HMax,               HEADLESS_MAX_WIDTH      },
        {aHidd_Sync_VMax,               HEADLESS_MAX_HEIGHT     },
        {aHidd_Sync_Description,        (IPTR)"Headless:%hx%v"  },
        {TAG_DONE,                      0UL                     }
    };
    /* Up to one PixFmtTags entry per depth, one SyncTags entry, TAG_DONE */
    struct TagItem modetags[sizeof(depths)/sizeof(depths[0]) + 2];
    ULONG maxdepth, fixeddepth, nominaldepth;
    ULONG width, height;
    /* The depths actually registered, ascending */
    ULONG regdepth[sizeof(depths)/sizeof(depths[0])];
    ULONG nregdepth = 0;
    ULONG i, nmodetags = 0;
    struct TagItem msgNewTags[] =
    {
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

    /*
     * Depth configuration, normally supplied by the monitor loader
     * from its icon's tooltypes. FixedDepth restricts the driver to a
     * single depth; otherwise every supported depth up to MaxDepth is
     * registered - except 15, which is only exposed when asked for
     * directly, so the default set is 8, 16 and 24. 32 is accepted as
     * a synonym for the 24-bit (32bpp) format.
     */
    maxdepth   = GetTagData(aHidd_Gfx_Headless_MaxDepth, 24, msg->attrList);
    fixeddepth = GetTagData(aHidd_Gfx_Headless_FixedDepth, 0, msg->attrList);
    if (maxdepth >= 24)
        maxdepth = 24;
    if (fixeddepth >= 24)
        fixeddepth = 24;

    for (i = 0; i < sizeof(depths)/sizeof(depths[0]); i++)
    {
        if (fixeddepth ? (depths[i].depth == fixeddepth)
                       : ((depths[i].depth <= maxdepth) &&
                          ((depths[i].depth != 15) || (maxdepth == 15))))
        {
            modetags[nmodetags].ti_Tag  = aHidd_DMEnum_PixFmtTags;
            modetags[nmodetags].ti_Data = (IPTR)depths[i].pftags;
            nmodetags++;
            regdepth[nregdepth++] = depths[i].depth;
        }
    }
    if (nmodetags == 0)
    {
        /* Bad configuration must not leave the system displayless */
        D(bug("HeadlessGfx::New: no depth matches MaxDepth %u/FixedDepth %u, using defaults\n", maxdepth, fixeddepth));
        for (i = 0; i < sizeof(depths)/sizeof(depths[0]); i++)
        {
            if (depths[i].depth == 15)
                continue;
            modetags[nmodetags].ti_Tag  = aHidd_DMEnum_PixFmtTags;
            modetags[nmodetags].ti_Data = (IPTR)depths[i].pftags;
            nmodetags++;
            regdepth[nregdepth++] = depths[i].depth;
        }
        maxdepth = 24;
        fixeddepth = 0;
    }

    /*
     * Nominal geometry, also from the loader's tooltypes. This is what
     * NominalDimensions() reports, and it decides which mode the system
     * opens its first display in: BestModeIDA() takes the depth as a
     * minimum and then settles on the smallest mode meeting it, so
     * without this the build-time AROS_NOMINAL_* values would be used
     * and the shallowest registered depth would win.
     */
    width  = GetTagData(aHidd_Gfx_Headless_Width, HEADLESS_DEF_WIDTH, msg->attrList);
    height = GetTagData(aHidd_Gfx_Headless_Height, HEADLESS_DEF_HEIGHT, msg->attrList);
    if (width < HEADLESS_MIN_WIDTH)
        width = HEADLESS_MIN_WIDTH;
    if (width > HEADLESS_MAX_WIDTH)
        width = HEADLESS_MAX_WIDTH;
    if (height < HEADLESS_MIN_HEIGHT)
        height = HEADLESS_MIN_HEIGHT;
    if (height > HEADLESS_MAX_HEIGHT)
        height = HEADLESS_MAX_HEIGHT;
    /* The sync is the display - keep the two in step */
    sync_mode[1].ti_Data = width;    /* HTotal */
    sync_mode[2].ti_Data = width;    /* HDisp  */
    sync_mode[3].ti_Data = height;   /* VDisp  */

    /*
     * A requested nominal depth must be one this driver actually
     * exposes: with FixedDepth there is only one to have, otherwise
     * take the shallowest registered depth that still satisfies the
     * request (BestModeIDA() treats it as a minimum) - which also
     * caps it at MaxDepth. Unset means "the deepest we registered".
     */
    nominaldepth = GetTagData(aHidd_Gfx_Headless_NominalDepth, 0, msg->attrList);
    if (fixeddepth || !nominaldepth || nominaldepth > regdepth[nregdepth - 1])
        nominaldepth = regdepth[nregdepth - 1];
    else
    {
        for (i = 0; i < nregdepth; i++)
        {
            if (regdepth[i] >= nominaldepth)
            {
                nominaldepth = regdepth[i];
                break;
            }
        }
    }

    XSD(cl)->nominalwidth  = width;
    XSD(cl)->nominalheight = height;
    XSD(cl)->nominaldepth  = nominaldepth;
    D(bug("HeadlessGfx::New: nominal %ux%ux%u\n", width, height, nominaldepth));
    modetags[nmodetags].ti_Tag  = aHidd_DMEnum_SyncTags;
    modetags[nmodetags].ti_Data = (IPTR)sync_mode;
    nmodetags++;
    modetags[nmodetags].ti_Tag  = TAG_DONE;
    modetags[nmodetags].ti_Data = 0;

    if ((msgNewTags[3].ti_Data = (IPTR)msg->attrList) == 0)
        msgNewTags[3].ti_Tag = TAG_DONE;

    msgNew.mID = msg->mID;
    msgNew.attrList = msgNewTags;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&msgNew);
    if (o)
    {
        struct TagItem displaytags[] =
        {
            { aHidd_Display_GfxHidd,  (IPTR)o        },
            { aHidd_Display_ModeTags, (IPTR)modetags },
            { TAG_DONE,               0              }
        };

        struct HeadlessGfxHiddData *data = OOP_INST_DATA(cl, o);

        data->maxDepth   = fixeddepth ? fixeddepth : maxdepth;
        data->fixedDepth = fixeddepth;

        D(bug("Got object from super\n"));
        XSD(cl)->headlessgfxhidd = o;

        XSD(cl)->headlessgfxdisplay = OOP_NewObject(XSD(cl)->displayclass, NULL, displaytags);
        if (XSD(cl)->headlessgfxdisplay)
            OOP_GetAttr(XSD(cl)->headlessgfxdisplay, aHidd_Display_DMEnumerator, (IPTR *)&XSD(cl)->dmenum);
    }
    ReturnPtr("HeadlessGfx::New", OOP_Object *, o);
}

VOID HeadlessGfx__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HeadlessGfxHiddData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_HEADLESSGFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_Headless_MaxDepth:
            *msg->storage = data->maxDepth;
            return;

        case aoHidd_Gfx_Headless_FixedDepth:
            *msg->storage = data->fixedDepth;
            return;
        }
    }
    else if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_DriverName:
            *msg->storage = (IPTR)"Headless";
            return;

        case aoHidd_Gfx_DisplayDefault:
            *msg->storage = (IPTR)XSD(cl)->headlessgfxdisplay;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *HeadlessGfxDisplay__Hidd_Display__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CreateObject *msg)
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
        struct pHidd_Display_CreateObject p;

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

VOID HeadlessGfxDisplay__Hidd_Display__NominalDimensions(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Display_NominalDimensions *msg)
{
    /*
     * Report what this driver actually offers. The base class answers
     * with the build-time AROS_NOMINAL_* values, which say nothing about
     * the modes registered here - and since BestModeIDA() takes the
     * depth as a minimum and then settles on the smallest mode meeting
     * it, a nominal depth below our shallowest format would open the
     * display in that format rather than the deepest one available.
     */
    if (msg->width)
        *(msg->width) = XSD(cl)->nominalwidth;
    if (msg->height)
        *(msg->height) = XSD(cl)->nominalheight;
    if (msg->depth)
        *(msg->depth) = XSD(cl)->nominaldepth;
}
