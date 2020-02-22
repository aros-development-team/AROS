/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Gfx Hidd class for SM502.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#define DEBUG 0
#include <aros/debug.h>

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/icon.h>
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
#include <stdio.h>

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

enum {
    ML_PIXCLK,
    ML_HPIXEL,
    ML_HSSTRT,
    ML_HSSTOP,
    ML_HTOTAL,
    ML_VPIXEL,
    ML_VSSTRT,
    ML_VSSTOP,
    ML_VTOTAL,
    ML_HVSYNC,
    ML_COUNT
};

struct TagItem * LoadExternalSyncs(OOP_Class *cl)
{
    struct Process *pr = NULL;
    struct TagItem *retVal = NULL;
    struct TagItem *lastTag = NULL;

    pr = (struct Process *)FindTask(NULL);

    if (pr->pr_Task.tc_Node.ln_Type == NT_PROCESS)
    {
        struct DOSBase *DOSBase;

        APTR winptr = pr->pr_WindowPtr;
        pr->pr_WindowPtr = (APTR)-1;

        DOSBase = (struct DOSBase *)OpenLibrary("dos.library", 0UL);

        if (DOSBase)
        {
            BPTR fp = Open("DEVS:modelines.txt", MODE_OLDFILE);

            if (fp)
            {
                TEXT buf[512];

                while (FGets(fp, buf, 512))
                {
                    char *p = buf;
                    ULONG i, Modeline[ML_COUNT];

                    if(p[0] != '#')
                    {
                        for(i = 0; i < ML_COUNT; i++)
                        {
                            LONG n = StrToLong(p, (LONG *)&Modeline[i]);
                            if(n == -1 || Modeline[i] == 0 || (i != ML_PIXCLK && Modeline[i] > 4000))
                                break;
                            p += n;
                        }

                        if(i == ML_COUNT)
                        {
                            struct TagItem *tags = AllocVecPooled(XSD(cl)->mempool, sizeof(struct TagItem)*14);
                            char *desc = AllocVecPooled(XSD(cl)->mempool, 32);

                            if (lastTag != NULL) {
                                lastTag[1].ti_Tag = TAG_MORE;
                                lastTag[1].ti_Data = (IPTR)tags;
                            }

                            snprintf(desc, 32, "SAGA(User):%dx%d", Modeline[ML_HPIXEL], Modeline[ML_VPIXEL]);
                            tags[0].ti_Tag = aHidd_Gfx_SyncTags;
                            tags[0].ti_Data = (IPTR)&tags[2];
                            tags[1].ti_Tag = TAG_DONE;
                            tags[1].ti_Data = 0;

                            tags[2].ti_Tag = aHidd_Sync_PixelClock;
                            tags[2].ti_Data = Modeline[ML_PIXCLK] *1000;
                            tags[3].ti_Tag = aHidd_Sync_HDisp;
                            tags[3].ti_Data = Modeline[ML_HPIXEL];
                            tags[4].ti_Tag = aHidd_Sync_HSyncStart;
                            tags[4].ti_Data = Modeline[ML_HSSTRT];
                            tags[5].ti_Tag = aHidd_Sync_HSyncEnd;
                            tags[5].ti_Data = Modeline[ML_HSSTOP];
                            tags[6].ti_Tag = aHidd_Sync_HTotal;
                            tags[6].ti_Data = Modeline[ML_HTOTAL];

                            tags[7].ti_Tag = aHidd_Sync_VDisp;
                            tags[7].ti_Data = Modeline[ML_VPIXEL];
                            tags[8].ti_Tag = aHidd_Sync_VSyncStart;
                            tags[8].ti_Data = Modeline[ML_VSSTRT];
                            tags[9].ti_Tag = aHidd_Sync_VSyncEnd;
                            tags[9].ti_Data = Modeline[ML_VSSTOP];
                            tags[10].ti_Tag = aHidd_Sync_VTotal;
                            tags[10].ti_Data = Modeline[ML_VTOTAL];

                            tags[11].ti_Tag = aHidd_Sync_Flags;
                            tags[11].ti_Data = Modeline[ML_HVSYNC];

                            tags[12].ti_Tag = aHidd_Sync_Description;
                            tags[12].ti_Data = (IPTR)desc;

                            tags[13].ti_Tag = TAG_DONE;
                            tags[13].ti_Data = 0UL;

                            lastTag = tags;
                            if (retVal == 0)
                                retVal = tags;
                        }
                    }
                }
                Close(fp);
            }

            CloseLibrary((struct Library *)DOSBase);
        }

        pr->pr_WindowPtr = winptr;
    }

    return retVal;
}

OOP_Object *METHOD(SAGAGfx, Root, New)
{
    struct TagItem *userSyncs = NULL;

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

    /*
        The instance of driver object is created by the wrapper from
        DEVS:Monitory through a call to AddDisplayDriver(). The wrapper
        has set the current directory properly and we can extract its name.

        We use this knowledge to eventually open the corresponding Icon and
        read the driver specific tooltypes. Eventually we parse those needed.
    */

    struct Library *IconBase = OpenLibrary("icon.library", 0);
    XSD(cl)->useHWSprite = FALSE;

    if (IconBase)
    {
        struct DiskObject *icon;
        STRPTR myName = FindTask(NULL)->tc_Node.ln_Name;

        /* We have icon.library and our (wrapper) name. Open icon now */
        icon = GetDiskObject(myName);

        if (icon)
        {
            /* Check our driver specific parameter */
            STRPTR hwSprite = FindToolType(icon->do_ToolTypes, "HWSPRITE");

            /* Found? Is it set to Yes? */
            if (hwSprite)
            {
                if (MatchToolValue(hwSprite, "Yes"))
                {
                    /* Use hardware sprite */
                    XSD(cl)->useHWSprite = TRUE;
                }
            }
            FreeDiskObject(icon);
        }

        CloseLibrary(IconBase);
    }

    /*
        Hide HW Sprite now - it will be either shown later or not used at all,
        depending on the tooltype.
    */
    WRITE16(SAGA_VIDEO_SPRITEX, SAGA_VIDEO_MAXHV - 1);
    WRITE16(SAGA_VIDEO_SPRITEY, SAGA_VIDEO_MAXVV - 1);

    newmsg.mID = msg->mID;
    newmsg.attrList = saganewtags;
    msg = &newmsg;

    D(bug("[SAGA] Root::New() called\n"));

    userSyncs = LoadExternalSyncs(cl);

    if (userSyncs) {
        syncs[6].ti_Tag = TAG_MORE;
        syncs[6].ti_Data = (IPTR)userSyncs;
    }

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

            case aoHidd_Gfx_HWSpriteTypes:
                found = TRUE;
                *msg->storage = XSD(cl)->useHWSprite ? vHidd_SpriteType_3Plus1 : 0;
                break;

#if 0 /* Not implemented yet */
            case aoHidd_Gfx_MaxHWSpriteWidth:
                found = TRUE;
                *msg->storage = 16;
                break;

            case aoHidd_Gfx_MaxHWSpriteHeight:
                found = TRUE;
                *msg->storage = 16;
                break;
#endif
        }
    }

    if (FALSE == found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(SAGAGfx, Hidd_Gfx, SetCursorPos)
{
    if (XSD(cl)->visible)
    {
        struct SAGAGfxBitmapData *bmdata = OOP_INST_DATA(XSD(cl)->bmclass, XSD(cl)->visible);

        D(bug("[SAGA] SetCursorPos(%d, %d)\n", msg->x, msg->y));
        WORD x = msg->x;
        WORD y = msg->y;

        if (bmdata->hwregs.video_mode & SAGA_VIDEO_MODE_DBLSCN(SAGA_VIDEO_DBLSCAN_X))
        {
            x <<= 1;
        }

        if (bmdata->hwregs.video_mode & SAGA_VIDEO_MODE_DBLSCN(SAGA_VIDEO_DBLSCAN_Y))
        {
            y <<= 1;
        }

        x += SAGA_MOUSE_DELTAX;
        y += SAGA_MOUSE_DELTAY;

        XSD(cl)->cursorX = x;
        XSD(cl)->cursorY = y;

        if (XSD(cl)->cursor_visible)
        {
            WRITE16(SAGA_VIDEO_SPRITEX, x);
            WRITE16(SAGA_VIDEO_SPRITEY, y);
        }
    }
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

    XSD(cl)->cursor_visible = msg->visible;
}

BOOL METHOD(SAGAGfx, Hidd_Gfx, SetCursorShape)
{
    IPTR width, height, depth;
    OOP_Object *cmap = NULL;
    IPTR num_colors = 0;
    IPTR ptr;

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Depth, &depth);
    OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, &cmap);

    if (cmap) {
        OOP_GetAttr(cmap, aHidd_ColorMap_NumEntries, &num_colors);
        if (num_colors > 4)
            num_colors = 4;

        D(bug("[SAGA] number of colors: %d\n", num_colors));

        for (int i=0; i < num_colors; i++) {
            HIDDT_Color c;
            HIDD_CM_GetColor(cmap, i, &c);

            XSD(cl)->cursor_pal[i] =
                ((c.red >> 12) & 15) << 8 |
                ((c.green >> 12) & 15) << 4 |
                ((c.blue >> 12) & 15) << 0;

            D(bug("[SAGA] c%02x: %x %x %x %x %08x\n", i, c.red, c.green, c.blue, c.alpha, c.pixval));
        }
    }

    D(bug("[SAGA] SetCursorShape(%p, %d, %d, %d)\n", msg->shape, width, height, depth));

    if (width > 16)
        width = 16;

    if (height > 16)
        height = 16;

    if (width != 16 || height != 16)
    {
        for (UWORD i=0; i < 16*16; i++)
            XSD(cl)->cursor_clut[i] = 0;
    }

    HIDD_BM_GetImageLUT(msg->shape, XSD(cl)->cursor_clut, 16, 0, 0, width, height, NULL);

    bug("Shape:\n");
    ptr = 0xdff800;

    for (int y = 0; y < 16; y++)
    {
        ULONG pix = 0x80008000;
        ULONG val = 0;

        for (int x = 0; x < 16; x++)
        {
            bug("%d ", XSD(cl)->cursor_clut[y *16 + x]);
            switch (XSD(cl)->cursor_clut[y*16 + x])
            {
                case 1:
                    val |= pix & 0xffff;
                    break;
                case 2:
                    val |= pix & 0xffff0000;
                    break;
                case 3:
                    val |= pix;
                    break;
                default:
                    break;
            }
            pix >>= 1;
        }
        WRITE32(ptr, val);
        ptr += 4;
        bug("\n");
    }

    for (int i=1; i < 4; i++) {
        WRITE16(0xdff3a0 + (i << 1), XSD(cl)->cursor_pal[i]);
    }

    XSD(cl)->hotX = msg->xoffset;
    XSD(cl)->hotY = msg->yoffset;

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
            /* Non-displayable friends of our bitmaps are our bitmaps too */
            OOP_Object *friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);

            D(bug("[SAGA] Not displayable. Friend=%p.\n", friend));

            if (friend && (OOP_OCLASS(friend) == XSD(cl)->bmclass))
            {
                D(bug("[SAGA] ClassID = ChunkyBM, friend is OK, returning correct class\n"));
                tags[0].ti_Tag  = aHidd_BitMap_ClassPtr;
                tags[0].ti_Data = (IPTR)XSD(cl)->bmclass;
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

/*********  GfxHidd::CopyBox()  ************************/

void METHOD(SAGAGfx, Hidd_Gfx, CopyBox)
{
    ULONG mode = GC_DRMD(msg->gc);
    IPTR src=0, dst=0;

bug("[SAGA] CopyBox(%p, %p, dx:%d, dy:%d, sx:%d, sy:%d, w:%d, h:%d)\n", msg->src, msg->dest, msg->destX, msg->destY,
msg->srcX, msg->srcY, msg->width, msg->height);

    if (OOP_OCLASS(msg->src) != XSD(cl)->bmclass ||
        OOP_OCLASS(msg->dest) != XSD(cl)->bmclass)
    {
        bug("[SAGA] CopyBox - either source or dest is not SAGA bitmap\n");
        bug("[SAGA] oclass src: %p, oclass dst: %p, bmclass: %p\n", OOP_OCLASS(msg->src), OOP_OCLASS(msg->dest), XSD(cl)->bmclass);
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {
        struct SAGAGfxBitmapData *bm_src = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct SAGAGfxBitmapData *bm_dst = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);

        if (bm_src->bitsperpix <= 8 || bm_dst->bitsperpix <= 8 || (bm_src->bitsperpix != bm_dst->bitsperpix))
        {
            bug("[SAGA] bpp_src=%d, bpp_dst=%d\n", bm_src->bitsperpix, bm_dst->bitsperpix);
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
        else
        {
            bug("[SAGA] both bitmaps compatible. drmd=%d\n", mode);
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
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

        WRITE16(SAGA_VIDEO_MODE, SAGA_VIDEO_FORMAT_OFF);

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

        if (XSD(cl)->cursor_visible)
        {
            IPTR ptr = 0xdff800;

            for (int y = 0; y < 16; y++)
            {
                ULONG pix = 0x80008000;
                ULONG val = 0;

                for (int x = 0; x < 16; x++)
                {
                    switch (XSD(cl)->cursor_clut[y*16 + x])
                    {
                        case 1:
                            val |= pix & 0xffff;
                            break;
                        case 2:
                            val |= pix & 0xffff0000;
                            break;
                        case 3:
                            val |= pix;
                            break;
                        default:
                            break;
                    }
                    pix >>= 1;
                }
                WRITE32(ptr, val);
                ptr += 4;
            }

            for (int i=1; i < 4; i++) {
                WRITE16(0xdff3a0 + (i << 1), XSD(cl)->cursor_pal[i]);
            }

            WRITE16(SAGA_VIDEO_SPRITEX, XSD(cl)->cursorX);
            WRITE16(SAGA_VIDEO_SPRITEY, XSD(cl)->cursorY);
        }
    }
    else
    {
        D(bug("[SAGA] No bitmap to show? Falling back to AGA...\n"));

        SAGA_SetPLL(SAGA_PIXELCLOCK);
        WRITE16(SAGA_VIDEO_MODE, SAGA_VIDEO_FORMAT_AMIGA);
    }

    data->visible = msg->bitMap;

    D(bug("[SAGA] Show() done\n"));

    return msg->bitMap;
}
