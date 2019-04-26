/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Class for VMWare.
    Lang: English.
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/kernel.h>
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

#include <asm/io.h>

#include <string.h>
#include <stdio.h>

#include "vmwaresvga_intern.h"

#include LC_LIBDEFS_FILE

#if (DEBUG)
#define DINFO(x)                x
#else
#define DINFO(x)
#endif

static OOP_AttrBase             HiddAttrBase; 
static OOP_AttrBase             HiddBitMapAttrBase;  
static OOP_AttrBase             HiddPixFmtAttrBase;
static OOP_AttrBase             HiddGfxAttrBase;
static OOP_AttrBase             HiddSyncAttrBase;
static OOP_AttrBase             HiddVMWareSVGAAttrBase;
static OOP_AttrBase             HiddVMWareSVGABitMapAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    {IID_Hidd,                  &HiddAttrBase             },
    {IID_Hidd_BitMap,           &HiddBitMapAttrBase             },
    {IID_Hidd_VMWareSVGABitMap, &HiddVMWareSVGABitMapAttrBase   },
    {IID_Hidd_VMWareSVGA,       &HiddVMWareSVGAAttrBase         },
    {IID_Hidd_PixFmt,           &HiddPixFmtAttrBase             },
    {IID_Hidd_Sync,             &HiddSyncAttrBase               },
    {IID_Hidd_Gfx,              &HiddGfxAttrBase                },
    {NULL,                      NULL                            }
};

static ULONG mask_to_shift(ULONG mask)
{
    ULONG i;

    for (i = 32; mask; i --) {
        mask >>= 1;
    }

    if (mask == 32) {
        i = 0;
    }

    return i;
}

static ULONG VMWareSVGA__GetDefSyncSizes(ULONG syncno, ULONG *syncwid, ULONG *synchi)
{
#define VMWARESVGA_DEFSYNCMAX   12
    switch (syncno % VMWARESVGA_DEFSYNCMAX)
    {
        case 1:
            *syncwid = 800;
            *synchi  = 600;
            break;
        case 2:
            *syncwid = 1024;
            *synchi  = 768;
            break;
        case 3:
            *syncwid = 1280;
            *synchi  = 1024;
            break;
        case 4:
            *syncwid = 1366;
            *synchi  = 768;
            break;
        case 5:
            *syncwid = 1440;
            *synchi  = 900;
            break;
        case 6:
            *syncwid = 1600;
            *synchi  = 1200;
            break;
        case 7:
            *syncwid = 1680;
            *synchi  = 1050;
            break;
        case 8:
            *syncwid = 1920;
            *synchi  = 1080;
            break;
        case 9:
            *syncwid = 1920;
            *synchi  = 1200;
            break;
        case 10:
            *syncwid = 2560;
            *synchi  = 1600;
            break;
        case 11:
            *syncwid = 3840;
            *synchi  = 2160;
            break;
        default:
            *syncwid = 640;
            *synchi  = 480;
            break;            
    }
    return (syncno % VMWARESVGA_DEFSYNCMAX);
}

OOP_Object *VMWareSVGA__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] =
    {
        {aHidd_PixFmt_RedShift,         0       }, /*  0 */
        {aHidd_PixFmt_GreenShift,       0       }, /*  1 */
        {aHidd_PixFmt_BlueShift,        0       }, /*  2 */
        {aHidd_PixFmt_AlphaShift,       0       }, /*  3 */
        {aHidd_PixFmt_RedMask,          0       }, /*  4 */
        {aHidd_PixFmt_GreenMask,        0       }, /*  5 */
        {aHidd_PixFmt_BlueMask,         0       }, /*  6 */
        {aHidd_PixFmt_AlphaMask,        0       }, /*  7 */
        {aHidd_PixFmt_ColorModel,       0       }, /*  8 */
        {aHidd_PixFmt_Depth,            0       }, /*  9 */
        {aHidd_PixFmt_BytesPerPixel,    0       }, /* 10 */
        {aHidd_PixFmt_BitsPerPixel,     0       }, /* 11 */
        {aHidd_PixFmt_StdPixFmt,        0       }, /* 12 */
        {aHidd_PixFmt_CLUTShift,        0       }, /* 13 */
        {aHidd_PixFmt_CLUTMask,         0x0f    }, /* 14 */
        {aHidd_PixFmt_BitMapType,       0       }, /* 15 */
        {TAG_DONE,                      0UL     }
    };
    struct TagItem *modetags;
    ULONG max_width, max_height;
    ULONG sync_Width, sync_Height;
    ULONG sync_count, sync_modes, sync_curr, sync_actual, sync_displayid, sync_modeid;

    XSD(cl)->prefWidth = vmwareReadReg(&XSD(cl)->data, SVGA_REG_WIDTH);
    XSD(cl)->prefHeight = vmwareReadReg(&XSD(cl)->data, SVGA_REG_HEIGHT);
    DINFO(bug("[VMWareSVGA] %s: Default %dx%d\n", __func__, XSD(cl)->prefWidth, XSD(cl)->prefHeight));

    max_width = vmwareReadReg(&XSD(cl)->data, SVGA_REG_MAX_WIDTH);
    max_height = vmwareReadReg(&XSD(cl)->data, SVGA_REG_MAX_HEIGHT);
    DINFO(bug("[VMWareSVGA] %s: Max %dx%d\n", __func__, max_width, max_height));

    DINFO(bug("[VMWareSVGA] %s: counting usable modes ...\n", __func__);)
    /* Determine the number of sync modes we will expose (per display if applicable) */
    sync_count = VMWARESVGA_DEFSYNCMAX;
    sync_modes = 0;
    while (sync_count > 0)
    {
        sync_count = VMWareSVGA__GetDefSyncSizes(sync_count - 1, &sync_Width, &sync_Height) + 1;
        DINFO(bug("[VMWareSVGA] %s: #%d ... %dx%d ", __func__, (VMWARESVGA_DEFSYNCMAX - sync_count) + 1, sync_Width, sync_Height);)
        if ((sync_Width <= max_width) && (sync_Height <= max_height))
        {
            DINFO(bug("is suitable");)
            sync_modes += 1;
        }
        sync_count -= 1;
        DINFO(bug("\n");)
    }
    DINFO(bug("[VMWareSVGA] %s: %d usable modes found\n", __func__, sync_modes);)
    sync_count = sync_modes;
#if defined(VMWARESVGA_USEMULTIMON)
    sync_count *= XSD(cl)->data.displaycount;
#endif
    sync_curr = 1;
#if defined(VMWARESVGA_USE8BIT)
    if ((data->capabilities & SVGA_CAP_8BIT_EMULATION) && (XSD(cl)->data.depth > 8))
        sync_curr += 1;
#endif
    modetags = AllocVec((sync_count + sync_curr + 1) * sizeof(struct TagItem), MEMF_CLEAR);
    modetags[0].ti_Tag = aHidd_Gfx_PixFmtTags;
    modetags[0].ti_Data = (IPTR)pftags;
#if defined(VMWARESVGA_USE8BIT)
    if ((data->capabilities & SVGA_CAP_8BIT_EMULATION) && (XSD(cl)->data.depth > 8))
    {
    }
#endif

    sync_curr = 0;
    while (sync_curr < sync_count)
    {
        char *sync_Description = AllocVec(SYNC_DESCNAME_LEN, MEMF_CLEAR);
        struct TagItem *sync_mode = AllocVec(11 * sizeof(struct TagItem), MEMF_CLEAR);

        sync_modeid = VMWareSVGA__GetDefSyncSizes(sync_curr, &sync_Width, &sync_Height);
        sync_displayid = sync_curr/sync_modes;

        DINFO(bug("[VMWareSVGA] %s: Setting Sync Mode %d for Display %d\n", __func__, sync_modeid, sync_displayid));

        if (sync_displayid == 0)
        {
            sprintf(sync_Description, "VMWareSVGA:%dx%d", sync_Width, sync_Height);
        }
        else
        {
            sprintf(sync_Description, "VMWareSVGA.%d:%dx%d", sync_displayid, sync_Width, sync_Height);
        }
        DINFO(bug("[VMWareSVGA] %s: Description '%s'\n", __func__, sync_Description));

        sync_mode[0].ti_Tag = aHidd_Sync_Description;
        sync_mode[0].ti_Data = (IPTR)sync_Description;

        sync_mode[1].ti_Tag = aHidd_Sync_PixelClock;

        sync_mode[2].ti_Tag = aHidd_Sync_HDisp;
        sync_mode[2].ti_Data = sync_Width;

        sync_mode[3].ti_Tag = aHidd_Sync_VDisp;
        sync_mode[3].ti_Data = sync_Height;

        sync_mode[4].ti_Tag = aHidd_Sync_HSyncStart;

        sync_mode[5].ti_Tag = aHidd_Sync_HSyncEnd;

        sync_mode[6].ti_Tag = aHidd_Sync_HTotal;

        sync_mode[7].ti_Tag = aHidd_Sync_VSyncStart;

        sync_mode[8].ti_Tag = aHidd_Sync_VSyncEnd;

        sync_mode[9].ti_Tag = aHidd_Sync_VTotal;

        sync_mode[10].ti_Tag = TAG_DONE;

        modetags[1 + sync_curr].ti_Tag = aHidd_Gfx_SyncTags;
        modetags[1 + sync_curr].ti_Data = (IPTR)sync_mode;
        sync_curr++;
    }
    modetags[1 + sync_curr].ti_Tag = TAG_DONE;

    struct TagItem svganewtags[] =
    {
        {aHidd_Gfx_ModeTags,    (IPTR)modetags  },
        { aHidd_Name            , (IPTR)"VMWareSVGA"     },
        { aHidd_HardwareName    , (IPTR)"VMWare SVGA Gfx Adaptor"   },
        { aHidd_ProducerName    , (IPTR)"VMWare Inc"  },
        {TAG_MORE, (IPTR)msg->attrList }
    };
    struct pRoot_New svganewmsg;

    /* set pftags = 0 */
    if (!XSD(cl)->data.pseudocolor)
    {
        pftags[0].ti_Data = mask_to_shift(XSD(cl)->data.redmask);
        pftags[1].ti_Data = mask_to_shift(XSD(cl)->data.greenmask);
        pftags[2].ti_Data = mask_to_shift(XSD(cl)->data.bluemask);
    }
    else
    {
        pftags[0].ti_Data = 0;
        pftags[1].ti_Data = 0;
        pftags[2].ti_Data = 0;
    }
    pftags[3].ti_Data = 0;
    pftags[4].ti_Data = XSD(cl)->data.redmask;
    pftags[5].ti_Data = XSD(cl)->data.greenmask;
    pftags[6].ti_Data = XSD(cl)->data.bluemask;
    pftags[7].ti_Data = 0;
    DINFO(bug("[VMWareSVGA] New: Masks red=%08x<<%d,green=%08x<<%d,blue%08x<<%d\n",
            pftags[4].ti_Data, pftags[0].ti_Data,
            pftags[5].ti_Data, pftags[1].ti_Data,
            pftags[6].ti_Data, pftags[2].ti_Data));

    if (XSD(cl)->data.pseudocolor)
        pftags[8].ti_Data = vHidd_ColorModel_Palette;
    else
        pftags[8].ti_Data = vHidd_ColorModel_TrueColor;

    pftags[9].ti_Data = XSD(cl)->data.depth;
    pftags[10].ti_Data = XSD(cl)->data.bytesperpixel;
    pftags[11].ti_Data = XSD(cl)->data.bitsperpixel;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;
#if defined(VMWARESVGA_USE8BIT)
    if ((data->capabilities & SVGA_CAP_8BIT_EMULATION) && (XSD(cl)->data.depth > 8))
    {
    }
#endif

    svganewmsg.mID = msg->mID;
    svganewmsg.attrList = svganewtags;
    msg = &svganewmsg;
    EnterFunc(bug("[VMWareSVGA] New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        DINFO(bug("[VMWareSVGA] %s: object @ 0x%p\n", __func__, o);)

        XSD(cl)->vmwaresvgahidd = o;
        XSD(cl)->mouse.shape = NULL;
        DINFO(
            bug("[VMWareSVGA] %s: Device capabilities: %08x\n", __func__, XSD(cl)->data.capabilities);
         )
            if (XSD(cl)->data.capabilities & SVGA_CAP_IRQMASK)
            {
                UWORD port = (UWORD)((IPTR)XSD(cl)->data.iobase + SVGA_IRQSTATUS_PORT);
                DINFO(bug("[VMWareSVGA] %s:   IRQ Mask\n", __func__);)
                vmwareWriteReg(&XSD(cl)->data, SVGA_REG_IRQMASK, 0);
                outl(0xFF, port);

                DINFO(bug("[VMWareSVGA] %s:   - Registering handler for IRQ #%d\n", __func__, XSD(cl)->data.hwint);)
                XSD(cl)->data.irq = KrnAddIRQHandler(XSD(cl)->data.hwint, vmwareHandlerIRQ, &XSD(cl)->data, NULL);
                vmwareWriteReg(&XSD(cl)->data, SVGA_REG_IRQMASK, SVGA_IRQFLAG_ERROR);
            }
        DINFO(
            if (XSD(cl)->data.capabilities & SVGA_CAP_EXTENDED_FIFO)
                bug("[VMWareSVGA] %s:   Extended FIFO\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_ALPHA_CURSOR)
                bug("[VMWareSVGA] %s:   Alpha Cursor\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_CURSOR_BYPASS)
                bug("[VMWareSVGA] %s:   Cursor-Bypass\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_CURSOR_BYPASS_2)
                bug("[VMWareSVGA] %s:   Cursor-Bypass2\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_8BIT_EMULATION)
                bug("[VMWareSVGA] %s:   8bit-emu\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_3D)
                bug("[VMWareSVGA] %s:   3D.\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_MULTIMON)
                bug("[VMWareSVGA] %s:   Multimon\n", __func__);
         )
        if (XSD(cl)->data.capabilities & SVGA_CAP_DISPLAY_TOPOLOGY)
        {
            DINFO(bug("[VMWareSVGA] %s:   Display Topology\n", __func__);)
            vmwareWriteReg(&XSD(cl)->data, SVGA_REG_NUM_GUEST_DISPLAYS, 1);
        }
        DINFO(
            if (XSD(cl)->data.capabilities & SVGA_CAP_PITCHLOCK)
                bug("[VMWareSVGA] %s:   Pitchlock\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_GMR)
                bug("[VMWareSVGA] %s:   GMR\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_GMR2)
                bug("[VMWareSVGA] %s:   GMR2\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_TRACES)
                bug("[VMWareSVGA] %s:   Traces\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_SCREEN_OBJECT_2)
                bug("[VMWareSVGA] %s:   Screen-Object2\n", __func__);
         )
        DINFO(
            if (XSD(cl)->data.capabilities & SVGA_CAP_RECT_COPY)
                bug("[VMWareSVGA] %s:   Copy Rect\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_RECT_FILL)
                bug("[VMWareSVGA] %s:   Fill Rect\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_OFFSCREEN_1)
                bug("[VMWareSVGA] %s:   BitMap/Pixmap\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_RECT_PAT_FILL)
                bug("[VMWareSVGA] %s:   Pattern Fill\n", __func__);
            if ((XSD(cl)->data.capabilities & (SVGA_CAP_RECT_FILL|SVGA_CAP_RASTER_OP)) == (SVGA_CAP_RECT_FILL|SVGA_CAP_RASTER_OP))
                bug("[VMWareSVGA] %s:   ROp Fill\n", __func__);
            if ((XSD(cl)->data.capabilities & (SVGA_CAP_RECT_COPY|SVGA_CAP_RASTER_OP)) == (SVGA_CAP_RECT_COPY|SVGA_CAP_RASTER_OP))
                bug("[VMWareSVGA] %s:   ROp Copy\n", __func__);
            if ((XSD(cl)->data.capabilities & (SVGA_CAP_RECT_PAT_FILL|SVGA_CAP_RASTER_OP)) == (SVGA_CAP_RECT_PAT_FILL|SVGA_CAP_RASTER_OP))
                bug("[VMWareSVGA] %s:   ROp Pattern Fill\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_GLYPH)
                bug("[VMWareSVGA] %s:   Glyph\n", __func__);
            if (XSD(cl)->data.capabilities & SVGA_CAP_GLYPH_CLIPPING)
                bug("[VMWareSVGA] %s:   Glyph Clipping\n", __func__);
         )
#if (0)
        /* Set the ID so vmware knows we are here */
        vmwareWriteReg(&XSD(cl)->data, SVGA_REG_GUEST_ID, 0x09);
#endif
    }

    D(bug("[VMWareSVGA] %s: returning 0x%p\n", __func__, o);)

    return o;
}

VOID VMWareSVGA__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    FreeVec(XSD(cl)->mouse.shape);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID VMWareSVGA__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_SupportsHWCursor:
            {
                found = TRUE;
                if (XSD(cl)->data.capabilities & SVGA_CAP_CURSOR)
                {
                    DINFO(bug("[VMWareSVGA] %s:   HW Cursor\n", __func__);)
                    *msg->storage = (IPTR)TRUE;
                }
                else
                    *msg->storage = (IPTR)FALSE;
            }
            break;

        case aoHidd_Gfx_SupportsGamma:
            *msg->storage = (IPTR)TRUE;
            found = TRUE;
            break;

        case aoHidd_Gfx_MemoryAttribs:
            {
                struct TagItem *matstate = (struct TagItem *)msg->storage;
                found = TRUE;
                 if (matstate)
                {
                    struct TagItem *matag;
                    while ((matag = NextTagItem(&matstate)))
                    {
                        switch(matag->ti_Tag)
                        {
                            case tHidd_Gfx_MemTotal:
                            case tHidd_Gfx_MemAddressableTotal:
                                matag->ti_Data = (IPTR)vmwareReadReg(&XSD(cl)->data, SVGA_REG_VRAM_SIZE);
                                DINFO(bug("[VMWareSVGA] %s: Mem Size = %ld\n", __func__, matag->ti_Data);)
                                break;
                            case tHidd_Gfx_MemFree:
                            case tHidd_Gfx_MemAddressableFree:
                                matag->ti_Data = 0;
                                break;
                        }
                    }
                }
            }
            break;
        }
    }
    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *VMWareSVGA__Hidd_Gfx__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    D(bug("[VMWareSVGA] %s()\n", __func__);)

    if (msg->cl == XSD(cl)->basebm)
    {
        BOOL displayable;
        BOOL framebuffer;
        OOP_Class *classptr = NULL;
        struct TagItem tags[] =
        {
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_MORE, (IPTR)msg->attrList }
        };

        struct pHidd_Gfx_CreateObject comsg;

        displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
        framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
        if (framebuffer)
            classptr = XSD(cl)->vmwaresvgaonbmclass;
        else if (displayable)
            classptr = XSD(cl)->vmwaresvgaoffbmclass;
        else
        {
            HIDDT_ModeID modeid;
            modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
            if (modeid != vHidd_ModeID_Invalid)
                classptr = XSD(cl)->vmwaresvgaoffbmclass;
            else
            {
                HIDDT_StdPixFmt stdpf;
                stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
                if (stdpf == vHidd_StdPixFmt_Unknown)
                {
                    OOP_Object *friend;
                    friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, (IPTR)NULL, msg->attrList);
                    if (friend != NULL)
                    {
                        OOP_Class *friend_class = NULL;
                        OOP_GetAttr(friend, aHidd_BitMap_ClassPtr, (IPTR *)&friend_class);
                        if (friend_class == XSD(cl)->vmwaresvgaonbmclass)
                        {
                            classptr = XSD(cl)->vmwaresvgaoffbmclass;
                        }
                    }
                }
            }
        }
        if (classptr != NULL)
        {
            tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
            tags[0].ti_Data = (IPTR)classptr;
        }
        comsg.mID = msg->mID;
        comsg.cl = msg->cl;
        comsg.attrList = tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&comsg);
    }
    else if ((XSD(cl)->basegallium && (msg->cl == XSD(cl)->basegallium)) &&
                (XSD(cl)->data.capabilities & SVGA_CAP_3D))
    {
        /* Create the gallium 3d driver object .. */
        object = OOP_NewObject(NULL, CLID_Hidd_Gallium_VMWareSVGA, msg->attrList);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    D(bug("[VMWareSVGA] %s: returning 0x%p\n", __func__, object);)
    return object;
}

ULONG VMWareSVGA__Hidd_Gfx__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    IPTR width = 0, bm_width, height = 0, bm_height;
    struct HIDD_ViewPortData *currVPD;

    D(bug("[VMWareSVGA] %s()\n", __func__);)

#if defined(VMWAREGFX_UPDATEFBONSHOWVP)
    if ((currVPD = msg->Data) != 0)
    {
        if ((XSD(cl)->data.shown) && (XSD(cl)->mouse.visible))
        {
            D(bug("[VMWareSVGA] %s: removing cursor...\n", __func__);)
            displayCursorVMWareSVGA(&XSD(cl)->data, SVGA_CURSOR_ON_HIDE);
        }

        XSD(cl)->data.shown = currVPD;
        D(bug("[VMWareSVGA] %s: shown = 0x%p\n", __func__, XSD(cl)->data.shown));

        while ((currVPD) && (currVPD->Bitmap))
        {
            OOP_GetAttr(currVPD->Bitmap, aHidd_BitMap_Width, (IPTR *)&bm_width);
            if (bm_width > width)
                width = bm_width;
            OOP_GetAttr(currVPD->Bitmap, aHidd_BitMap_Height, (IPTR *)&bm_height);
            if (bm_height > height)
                height = bm_height;
            currVPD = currVPD->Next;
        }
        D(bug("[VMWareSVGA] %s: %dx%d\n", __func__, width, height));

        if (width == 0)
            width = XSD(cl)->prefWidth;
        if (height == 0)
            height = XSD(cl)->prefHeight;

        setModeVMWareSVGA(&XSD(cl)->data, width, height);
        syncfenceVMWareSVGAFIFO(&XSD(cl)->data, fenceVMWareSVGAFIFO(&XSD(cl)->data));
        if (XSD(cl)->mouse.visible)
        {
            D(bug("[VMWareSVGA] %s: displaying cursor...\n", __func__);)
            defineCursorVMWareSVGA(&XSD(cl)->data, &XSD(cl)->mouse);
            moveCursorVMWareSVGA(&XSD(cl)->data, XSD(cl)->mouse.x, XSD(cl)->mouse.y);
            displayCursorVMWareSVGA(&XSD(cl)->data, SVGA_CURSOR_ON_SHOW);
        }
    }
    else
    {
        D(bug("[VMWareSVGA] %s: nothing to show ...\n", __func__);)
    }
#endif
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL VMWareSVGA__Hidd_Gfx__SetGamma(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Gamma *msg)
{
    D(bug("[VMWareSVGA] %s()\n", __func__);)
#if (0)
    int i;
    for (i = 0; i < 256; i++) {
        D(bug("[VMWareSVGA] %s: #%d    0x%04x:0x%04x:0x%04x\n", __func__, i, msg->Red[i], msg->Green[i], msg->Blue[i]);)
        vmwareWriteReg(&XSD(cl)->data, SVGA_PALETTE_BASE + i * 3 + 0, msg->Red[i] >> 8);
        vmwareWriteReg(&XSD(cl)->data, SVGA_PALETTE_BASE + i * 3 + 1, msg->Green[i] >> 8);
        vmwareWriteReg(&XSD(cl)->data, SVGA_PALETTE_BASE + i * 3 + 2, msg->Blue[i] >> 8);
    }
#endif
    return TRUE;
}

VOID VMWareSVGA__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    UBYTE *src = NULL;
    UBYTE *dst = NULL;
    HIDDT_DrawMode mode;
    struct HWData *hwdata = &XSD(cl)->data;
    struct Box box = { msg->srcX, msg->srcY, msg->srcX + msg->width + 1, msg->srcY + msg->height + 1};

    D(bug("[VMWareSVGA] %s()\n", __func__);)

    ObtainSemaphore(&hwdata->damage_control);

    mode = GC_DRMD(msg->gc);
    OOP_GetAttr(msg->src, aHidd_VMWareSVGABitMap_Drawable, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_VMWareSVGABitMap_Drawable, (IPTR *)&dst);
    if (((dst == NULL) || (src == NULL))) /* no vmwaregfx bitmap */
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    // TODO: This is nice and fast. but unfortunately has to go. We'll soon switch to a more refined accelerated blitting
    else if ((VPVISFLAG) && (dst == src) && (OOP_OCLASS(msg->dest) == XSD(cl)->vmwaresvgaonbmclass))
    {
        D(bug("[VMWareSVGA] %s: suitable bitmaps used ...\n", __func__);)

        struct BitmapData *data;
        data = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        switch (mode)
        {
            case vHidd_GC_DrawMode_Clear:
                    clearCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_And:
                    andCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_AndReverse:
                    andReverseCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_Copy:
                    copyCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_AndInverted:
                    andInvertedCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_NoOp:
                    noOpCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_Xor:
                    xorCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_Or:
                    orCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_Nor:
                    norCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_Equiv:
                    equivCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_Invert:
                    invertCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_OrReverse:
                    orReverseCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_CopyInverted:
                    copyInvertedCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_OrInverted:
                    orInvertedCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_Nand:
                    nandCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            case vHidd_GC_DrawMode_Set:
                    setCopyVMWareSVGA(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
                    break;
            default:
                {
                    D(bug("[VMWareSVGA] %s: mode %d is not handled\n", __func__, mode);)
                    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
                }
        }
    }
    else
    {
        struct BitmapData *srcbd = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct BitmapData *dstbd = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
        UBYTE *sbuffer;
        ULONG srestadd;
        UBYTE *dbuffer;
        ULONG drestadd;
        ULONG ycnt = msg->height;
        ULONG xcnt;
        LONG offset;

        /* get src/dest video data start addresses and skip sizes */
        if (srcbd->VideoData == srcbd->data->vrambase)
        {
            offset = (msg->srcX*srcbd->bytesperpix)+(msg->srcY*srcbd->data->bytesperline);
            srestadd = (srcbd->data->bytesperline - (msg->width*srcbd->bytesperpix));
            if ((VPVISFLAG) && (XSD(cl)->mouse.visible))
            {
                displayCursorVMWareSVGA(&XSD(cl)->data, SVGA_CURSOR_ON_REMOVE_FROM_FB);
            }
        }
        else
        {
            offset = (msg->srcX+(msg->srcY*srcbd->width))*srcbd->bytesperpix;
            srestadd = (srcbd->width - msg->width)*srcbd->bytesperpix;
        }
        sbuffer = srcbd->VideoData+offset;
        if (dstbd->VideoData == dstbd->data->vrambase)
        {
            offset = (msg->destX*dstbd->bytesperpix)+(msg->destY*dstbd->data->bytesperline);
            drestadd = (dstbd->data->bytesperline - (msg->width*dstbd->bytesperpix));
            if ((VPVISFLAG) && (XSD(cl)->mouse.visible))
            {
                displayCursorVMWareSVGA(&XSD(cl)->data, SVGA_CURSOR_ON_REMOVE_FROM_FB);
            }
        }
        else
        {
            offset = (msg->destX+(msg->destY*dstbd->width))*dstbd->bytesperpix;
            drestadd = (dstbd->width - msg->width)*dstbd->bytesperpix;
        }
        dbuffer = dstbd->VideoData+offset;

        switch (mode)
        {
            case vHidd_GC_DrawMode_Copy:
                {
                    D(bug("[VMWareSVGA] %s: using CopyMem\n", __func__);)
                    while (ycnt--)
                    {
                        xcnt = msg->width;

                        // NOTE: this is only valid if the two bitmaps share the same bytes per pixel.
                        //		 we may want to pre-process it (see below in the mouse definition code)
                        CopyMem(sbuffer, dbuffer, xcnt * dstbd->bytesperpix);

                        sbuffer += xcnt * dstbd->bytesperpix;
                        sbuffer += srestadd;
                        dbuffer += xcnt * dstbd->bytesperpix;
                        dbuffer += drestadd;
                    }
                }
                break;
            default:
                D(bug("[VMWareSVGA] mode = %ld, src @ 0x%p dst @ 0x%p\n", mode, src, dst);)
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }

        if ((VPVISFLAG) && (XSD(cl)->mouse.visible))
        {
            displayCursorVMWareSVGA(&XSD(cl)->data, SVGA_CURSOR_ON_RESTORE_TO_FB);
        }
    }

    box.x1 = msg->srcX;
    box.y1 = msg->srcY;
    box.x2 = box.x1+msg->width+1;
    box.y2 = box.y1+msg->height+1;

    VMWareSVGA_Damage_DeltaAdd(hwdata, &box);
    ReleaseSemaphore(&hwdata->damage_control);

    D(bug("[VMWareSVGA] %s: done\n", __func__);)
}

BOOL VMWareSVGA__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct VMWareSVGA_staticdata *data = XSD(cl);

    D(bug("[VMWareSVGA] %s()\n", __func__);)

    if (msg->shape == NULL)
    {
        D(bug("[VMWareSVGA] %s: blanking cursor\n", __func__);)
        if ((VPVISFLAG) && (XSD(cl)->visible))
            displayCursorVMWareSVGA(&XSD(cl)->data, SVGA_CURSOR_ON_HIDE);
        data->mouse.oopshape = NULL;
        FreeVec(data->mouse.shape);
        data->mouse.shape = NULL;

        return TRUE;
    }
    else
    {
#if (0)
        OOP_Object *colmap;
        OOP_Object *pfmt;
        HIDDT_StdPixFmt pixfmt;
#endif
        IPTR tmp;

        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &tmp);
        data->mouse.width = tmp;
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &tmp);
        data->mouse.height = tmp;
#if (0)
        OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (IPTR *)&pfmt);
        OOP_GetAttr(pfmt, aHidd_PixFmt_StdPixFmt, (IPTR *)&pixfmt);
        OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (IPTR *)&colmap);
#endif

        /* convert shape to vmware needs */
        FreeVec(data->mouse.shape);
        tmp = data->mouse.width * data->mouse.height;
        data->mouse.shape = AllocVec(tmp << 2, MEMF_CLEAR|MEMF_PUBLIC);
        if (data->mouse.shape != NULL)
        {
            UBYTE *shape;
            shape = data->mouse.shape;

            data->mouse.oopshape = msg->shape;
            // Get data from the bitmap. Using the ALPHA CURSOR we can now directly pre-process the bitmap to a suitable format
            HIDD_BM_GetImage(msg->shape, (UBYTE *)shape, data->mouse.width * 4, 0, 0, data->mouse.width, data->mouse.height, vHidd_StdPixFmt_BGRA32);
            if (XSD(cl)->visible)
            {
                struct BitmapData *bmdata = OOP_INST_DATA(XSD(cl)->vmwaresvgaonbmclass, XSD(cl)->visible);

                syncfenceVMWareSVGAFIFO(bmdata->data, (bmdata->data->fence - 1));
                defineCursorVMWareSVGA(bmdata->data, &data->mouse);
                syncfenceVMWareSVGAFIFO(bmdata->data, fenceVMWareSVGAFIFO(bmdata->data));
            }
            return TRUE;
        }
    }

    return FALSE;
}

BOOL VMWareSVGA__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    D(bug("[VMWareSVGA] %s()\n", __func__);)

    XSD(cl)->mouse.x = msg->x;
    XSD(cl)->mouse.y = msg->y;

    if ((VPVISFLAG) && (XSD(cl)->visible))
    {
        struct BitmapData *bmdata = OOP_INST_DATA(XSD(cl)->vmwaresvgaonbmclass, XSD(cl)->visible);

        syncfenceVMWareSVGAFIFO(bmdata->data, (bmdata->data->fence - 1));
        moveCursorVMWareSVGA(bmdata->data, XSD(cl)->mouse.x, XSD(cl)->mouse.y);
        syncfenceVMWareSVGAFIFO(bmdata->data, fenceVMWareSVGAFIFO(bmdata->data));
    }

    return TRUE;
}

VOID VMWareSVGA__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    D(bug("[VMWareSVGA] %s()\n", __func__);)

    XSD(cl)->mouse.visible = msg->visible;
    if ((VPVISFLAG) && (XSD(cl)->visible))
    {
        struct BitmapData *bmdata = OOP_INST_DATA(XSD(cl)->vmwaresvgaonbmclass, XSD(cl)->visible);

        syncfenceVMWareSVGAFIFO(bmdata->data, (bmdata->data->fence - 1));
        displayCursorVMWareSVGA(bmdata->data, msg->visible ? SVGA_CURSOR_ON_SHOW : SVGA_CURSOR_ON_HIDE);
        syncfenceVMWareSVGAFIFO(bmdata->data, fenceVMWareSVGAFIFO(bmdata->data));
    }
}

static int VMWareSVGA_InitStatic(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[VMWareSVGA] %s()\n", __func__);)

    LIBBASE->vsd.mouse.x=0;
    LIBBASE->vsd.mouse.y=0;
    LIBBASE->vsd.mouse.shape = NULL;

    if (!OOP_ObtainAttrBases(attrbases))
    {
        D(bug("[VMWareSVGA] %s: attrbases init failed\n", __func__);)
        return FALSE;
    }
    
    D(bug("[VMWareSVGA] %s: initialised\n", __func__);)

    return TRUE;
}

static int VMWareSVGA_ExpungeStatic(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[VMWareSVGA] %s()\n", __func__);)

    OOP_ReleaseAttrBases(attrbases);

    D(bug("[VMWareSVGA] %s: done\n", __func__);)

    return TRUE;
}

ADD2INITLIB(VMWareSVGA_InitStatic, 0)
ADD2EXPUNGELIB(VMWareSVGA_ExpungeStatic, 0)
