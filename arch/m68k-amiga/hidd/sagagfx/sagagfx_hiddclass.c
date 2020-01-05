/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Gfx Hidd class for SM502.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#define DEBUG 1
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

#include "sagagfx_hidd.h"
#include "sagagfx_bitmap.h"
#include "sagagfx_hw.h"

#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal,flags,descr)   \
    struct TagItem sync_ ## name[]={            \
        { aHidd_Sync_PixelClock,    clock*1000  },  \
        { aHidd_Sync_HDisp,         hdisp   },  \
        { aHidd_Sync_HSyncStart,    hstart  },  \
        { aHidd_Sync_HSyncEnd,      hend    },  \
        { aHidd_Sync_HTotal,        htotal  },  \
        { aHidd_Sync_VDisp,         vdisp   },  \
        { aHidd_Sync_VSyncStart,    vstart  },  \
        { aHidd_Sync_VSyncEnd,      vend    },  \
        { aHidd_Sync_VTotal,        vtotal  },  \
        { aHidd_Sync_Flags,         flags   },  \
        { aHidd_Sync_Description,       (IPTR)descr},   \
        { TAG_DONE, 0UL }}

OOP_Object *METHOD(SAGAGfx, Root, New)
{
    MAKE_SYNC(320x240, 25180, 320, 656, 752, 800, 240, 490, 492 , 524, 0, "SAGA:320x240");
    MAKE_SYNC(640x360, 28375, 640, 896, 984, 1088, 360, 504, 508, 518, 1, "SAGA:640x360");
    MAKE_SYNC(640x480, 25180, 640, 656, 752, 800, 480, 490, 492, 525, 0, "SAGA:640x480");
    MAKE_SYNC(720x400, 28320, 720, 738, 846, 900, 400, 412, 414, 449, 2, "SAGA:720x400");
    MAKE_SYNC(720x576, 28375, 720, 753, 817, 908, 576, 582, 586, 624, 1, "SAGA:720x576");
    MAKE_SYNC(800x600, 28375, 800, 848, 880, 960, 600, 603, 607, 615, 1, "SAGA:800x600");

    struct TagItem syncs[] = {
        { aHidd_Gfx_SyncTags,       (IPTR)sync_320x240 },
        { aHidd_Gfx_SyncTags,       (IPTR)sync_640x360 },
        { aHidd_Gfx_SyncTags,       (IPTR)sync_640x480 },
        { aHidd_Gfx_SyncTags,       (IPTR)sync_720x400 },
        { aHidd_Gfx_SyncTags,       (IPTR)sync_720x576 },
        { aHidd_Gfx_SyncTags,       (IPTR)sync_800x600 },
        { TAG_DONE, 0UL }
    };

    struct TagItem pftags_32bpp[] = {
        { aHidd_PixFmt_RedShift,    8   }, /* 0 */
        { aHidd_PixFmt_GreenShift,  16  }, /* 1 */
        { aHidd_PixFmt_BlueShift,  	24  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x00ff0000 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x0000ff00 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x000000ff }, /* 6 */
        { aHidd_PixFmt_AlphaMask,   0xff000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,       32	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,4	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,32	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_BGRA32 }, /* 12 Native */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    struct TagItem pftags_24bpp[] = {
        { aHidd_PixFmt_RedShift,    8   }, /* 0 */
        { aHidd_PixFmt_GreenShift,  16  }, /* 1 */
        { aHidd_PixFmt_BlueShift,  	24  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x00ff0000 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x0000ff00 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x000000ff }, /* 6 */
        { aHidd_PixFmt_AlphaMask,   0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,       24	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,3	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,24	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_BGR032 }, /* 12 Native */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    struct TagItem pftags_16bpp[] = {
        { aHidd_PixFmt_RedShift,	16	}, /* 0 */
        { aHidd_PixFmt_GreenShift,	21	}, /* 1 */
        { aHidd_PixFmt_BlueShift,  	27	}, /* 2 */
        { aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
        { aHidd_PixFmt_RedMask,		0x0000f800 }, /* 4 */
        { aHidd_PixFmt_GreenMask,	0x000007e0 }, /* 5 */
        { aHidd_PixFmt_BlueMask,	0x0000001f }, /* 6 */
        { aHidd_PixFmt_AlphaMask,	0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,	vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,		16	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,	2	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,	16	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_RGB16 }, /* 12 */
        { aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    struct TagItem pftags_8bpp[] = {
        { aHidd_PixFmt_RedShift,    8   }, /* 0 */
        { aHidd_PixFmt_GreenShift,  16  }, /* 1 */
        { aHidd_PixFmt_BlueShift,  	24  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x00ff0000 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x0000ff00 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x000000ff }, /* 6 */
        { aHidd_PixFmt_AlphaMask,	0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,	vHidd_ColorModel_Palette }, /* 8 */
        { aHidd_PixFmt_CLUTMask,    0x000000ff },
        { aHidd_PixFmt_CLUTShift,   0x00000000 },
        { aHidd_PixFmt_Depth,		8	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,	1	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,	8	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_LUT8 }, /* 12 */
        { aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    struct TagItem modetags[] = {
        { aHidd_Gfx_PixFmtTags,	(IPTR)pftags_32bpp  },
        { aHidd_Gfx_PixFmtTags,	(IPTR)pftags_24bpp  },
        { aHidd_Gfx_PixFmtTags,	(IPTR)pftags_16bpp  },
        { aHidd_Gfx_PixFmtTags, (IPTR)pftags_8bpp   },
        { TAG_MORE,             (IPTR)syncs },
        { TAG_DONE, 0UL }
    };

    struct TagItem saganewtags[] =
    {
        { aHidd_Gfx_ModeTags    , (IPTR)modetags                },
        { aHidd_Name            , (IPTR)"SAGA"                  },
        { aHidd_HardwareName    , (IPTR)"SAGA Graphics Chip"    },
        { aHidd_ProducerName    , (IPTR)"APOLLO Team"           },
        { TAG_MORE, (IPTR)msg->attrList }
    };

    struct pRoot_New newmsg;

    /* Are we already there? Don't init it for the second time! */
    if (XSD(cl)->sagagfxhidd)
        return NULL;

    newmsg.mID = msg->mID;
    newmsg.attrList = saganewtags;
    msg = &newmsg;

    D(bug("[SAGA] Root::New() called\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        D(bug("[SAGA] DoSuperMethod() returned %p\n", o));
        XSD(cl)->sagagfxhidd = o;
    }

    D(bug("[SAGA] IRoot::New() = %p\n", o));

    return o;
}

VOID METHOD(SAGAGfx, Root, Dispose)
{
    D(bug("[SAGA] Root::Dispose()\n"));
    XSD(cl)->sagagfxhidd = NULL;
    DeletePool(XSD(cl)->mempool);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(SAGAGfx, Root, Get)
{
    ULONG idx;
    int found = FALSE;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Gfx_NoFrameBuffer:
                found = TRUE;
                *msg->storage = TRUE;
                break;
#if 0
            case aoHidd_Gfx_SupportsHWCursor:
                found = TRUE;
                *msg->storage = TRUE;
                break;

            case aoHidd_Gfx_HWSpriteTypes:
                found = TRUE;
                *msg->storage = vHidd_SpriteType_3Plus1;
                return;
#endif
        }
    }

    if (FALSE == found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(SAGAGfx, Hidd_Gfx, SetCursorPos)
{
    D(bug("[SAGA] SetCursorPos(%d, %d)\n", msg->x, msg->y));
    XSD(cl)->cursorX = msg->x;
    XSD(cl)->cursorY = msg->y;
    WRITE16(SAGA_VIDEO_SPRITEX, msg->x);
    WRITE16(SAGA_VIDEO_SPRITEY, msg->y);

    return TRUE;
}

VOID METHOD(SAGAGfx, Hidd_Gfx, SetCursorVisible)
{
    D(bug("[SAGA] SetCursorVisible(%d)\n", msg->visible));

    if (msg->visible)
    {
        WRITE16(SAGA_VIDEO_SPRITEX, XSD(cl)->cursorX);
        WRITE16(SAGA_VIDEO_SPRITEY, XSD(cl)->cursorY);
    }
    else
    {
        WRITE16(SAGA_VIDEO_SPRITEX, SAGA_VIDEO_MAXHV - 1);
        WRITE16(SAGA_VIDEO_SPRITEY, SAGA_VIDEO_MAXVV - 1);
    }
}

BOOL METHOD(SAGAGfx, Hidd_Gfx, SetCursorShape)
{
    D(bug("[SAGA] SetCursorShape()\n"));
    return TRUE;
}

OOP_Object *METHOD(SAGAGfx, Hidd_Gfx, CreateObject)
{
    OOP_Object      *object = NULL;

    D(bug("[SAGA] Hidd_Gfx::CreateObject()\n"));

    if (msg->cl == XSD(cl)->basebm)
    {
        BOOL displayable;
        BOOL framebuffer;
        struct TagItem tags[2] =
        {
            {TAG_IGNORE, 0                  },
            {TAG_MORE  , (IPTR)msg->attrList}
        };
        struct pHidd_Gfx_CreateObject p;

        displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
        framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

        D(bug("[SAGA] displayable=%d, framebuffer=%d\n", displayable, framebuffer));

        if (displayable)
        {
            D(bug("[SAGA] Displayable bitmap.\n"));

            /* Only displayable bitmaps are bitmaps of our class */
            tags[0].ti_Tag  = aHidd_BitMap_ClassPtr;
            tags[0].ti_Data = (IPTR)XSD(cl)->bmclass;
        }
        else
        {
            /* Non-displayable friends of our bitmaps are plain chunky bitmaps */
            OOP_Object *friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);

            D(bug("[SAGA] Not displayable. Friend=%p.\n", friend));

            if (friend && (OOP_OCLASS(friend) == XSD(cl)->bmclass))
            {
                D(bug("[SAGA] ClassID = ChunkyBM\n"));
                tags[0].ti_Tag  = aHidd_BitMap_ClassID;
                tags[0].ti_Data = (IPTR)CLID_Hidd_ChunkyBM;
            }
        }

        p.mID = msg->mID;
        p.cl = msg->cl;
        p.attrList = tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    D(bug("[SAGA] CreateObject returns %p\n", object));

    return object;
}

/*********  GfxHidd::Show()  ***************************/

OOP_Object *METHOD(SAGAGfx, Hidd_Gfx, Show)
{
    struct SAGAGfx_staticdata *data = XSD(cl);
    struct TagItem tags[] = {
        {aHidd_BitMap_Visible, FALSE},
        {TAG_DONE, 0 }
    };

    D(bug("[SAGA] Show(0x%p), old visible 0x%p\n", msg->bitMap, data->visible));

    /* Remove old bitmap from the screen */
    if (data->visible)
    {
        D(bug("[SAGA] Hiding old bitmap\n"));
//        OOP_SetAttrs(data->visible, tags);
    }

    if (msg->bitMap)
    {
        struct SAGAGfxBitmapData *bmdata = OOP_INST_DATA(data->bmclass, msg->bitMap);

        /* If we have a bitmap to show, set it as visible */
        D(bug("[SAGA] Showing new bitmap\n"));
        tags[0].ti_Data = TRUE;
//        OOP_SetAttrs(msg->bitMap, tags);

        WRITE16(SAGA_VIDEO_MODE, SAGA_VIDEO_FORMAT_AMIGA);

        WRITE16(SAGA_VIDEO_HPIXEL, bmdata->hwregs.hpixel);
        WRITE16(SAGA_VIDEO_HSSTRT, bmdata->hwregs.hsstart);
        WRITE16(SAGA_VIDEO_HSSTOP, bmdata->hwregs.hsstop);
        WRITE16(SAGA_VIDEO_HTOTAL, bmdata->hwregs.htotal);

        WRITE16(SAGA_VIDEO_VPIXEL, bmdata->hwregs.vpixel);
        WRITE16(SAGA_VIDEO_VSSTRT, bmdata->hwregs.vsstart);
        WRITE16(SAGA_VIDEO_VSSTOP, bmdata->hwregs.vsstop);
        WRITE16(SAGA_VIDEO_VTOTAL, bmdata->hwregs.vtotal);

        WRITE16(SAGA_VIDEO_HVSYNC, bmdata->hwregs.hvsync);

        SAGA_SetPLL(bmdata->hwregs.pixelclock);

        if (bmdata->CLUT)
            SAGA_LoadCLUT(bmdata->CLUT, 0, 256);

        WRITE16(SAGA_VIDEO_MODE, bmdata->hwregs.video_mode);

        WRITE32(SAGA_VIDEO_BPLPTR, (ULONG)bmdata->VideoData);
        WRITE16(SAGA_VIDEO_BPLHMOD, 0);

        WRITE16(SAGA_VIDEO_MODE, bmdata->hwregs.video_mode);
    }
    else
    {
        D(bug("[SAGA] No bitmap to show? Falling back to AGA...\n"));

        SAGA_SetPLL(SAGA_PIXELCLOCK);
        WRITE16(SAGA_VIDEO_MODE, SAGA_VIDEO_FORMAT_AMIGA);

        return NULL;
    }

    data->visible = msg->bitMap;

    D(bug("[SAGA] Show() done\n"));

    return msg->bitMap;
}
