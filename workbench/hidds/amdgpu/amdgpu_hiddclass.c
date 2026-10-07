/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Class for AMD GPUs.
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

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
#include <stdio.h>

#include "amdgpu_intern.h"

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
static OOP_AttrBase             HiddDisplayAttrBase;
static OOP_AttrBase             HiddDMEnumAttrBase;
static OOP_AttrBase             HiddSyncAttrBase;
static OOP_AttrBase             HiddAmdgpuAttrBase;
static OOP_AttrBase             HiddAmdgpuBitMapAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    {IID_Hidd,                  &HiddAttrBase             },
    {IID_Hidd_BitMap,           &HiddBitMapAttrBase             },
    {IID_Hidd_AmdgpuBitMap, &HiddAmdgpuBitMapAttrBase   },
    {IID_Hidd_Amdgpu,       &HiddAmdgpuAttrBase         },
    {IID_Hidd_PixFmt,           &HiddPixFmtAttrBase             },
    {IID_Hidd_Sync,             &HiddSyncAttrBase               },
    {IID_Hidd_Gfx,              &HiddGfxAttrBase                },
    {IID_Hidd_Display,          &HiddDisplayAttrBase            },
    {IID_Hidd_DMEnum,           &HiddDMEnumAttrBase             },
    {NULL,                      NULL                            }
};

/* drm-compat/linux_time.c: waits spin rather than yield once this is set */
extern int amdgpu_compat_atomic;

/*
 * The system is going down: hand the device back the way the firmware
 * left it, so that whatever boots next finds the firmware display.
 */
AROS_INTH1(ResetHandler, struct Amdgpu_KMS *, kms)
{
    AROS_INTFUNC_INIT

    amdgpu_compat_atomic = 1;
    Amdgpu_KMS_ShowCursor(kms, FALSE);
    Amdgpu_KMS_Shutdown(kms);

    return FALSE;

    AROS_INTFUNC_EXIT
}

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

/*
 * One sync per distinct size the display offers. The driver lists them
 * largest first; here the size the display prefers (its initial one)
 * comes first, since the first sync is what a plain screen opens on, and
 * the rest follow smallest first.
 */
static ULONG Amdgpu_ModeRank(drmModeModeInfoPtr mode)
{
    if (mode->type & DRM_MODE_TYPE_PREFERRED)
        return 0;
    return 1 + (ULONG)mode->hdisplay * mode->vdisplay;
}

static struct TagItem *Amdgpu_CreateSyncTags(struct Amdgpu_KMS *kms, ULONG *count)
{
    drmModeConnectorPtr connector = kms->connector;
    struct TagItem *modetags;
    ULONG *order;
    ULONG n = 0, i, j;

    modetags = AllocVec((connector->count_modes + 2) * sizeof(struct TagItem), MEMF_CLEAR);
    order = AllocVec(connector->count_modes * sizeof(ULONG), MEMF_CLEAR);
    if (!modetags || !order)
    {
        FreeVec(modetags);
        FreeVec(order);
        return NULL;
    }

    for (i = 0; i < connector->count_modes; i++)
    {
        /* insertion sort by rank */
        for (j = i; j > 0; j--)
        {
            if (Amdgpu_ModeRank(&connector->modes[order[j - 1]]) <= Amdgpu_ModeRank(&connector->modes[i]))
                break;
            order[j] = order[j - 1];
        }
        order[j] = i;
    }

    for (i = 0; i < connector->count_modes; i++)
    {
        drmModeModeInfoPtr mode = &connector->modes[order[i]];
        struct TagItem *sync_mode;
        char *sync_Description;
        int smtagno = 0;
        BOOL dup = FALSE;

        for (j = 0; j < i; j++)
        {
            if (connector->modes[order[j]].hdisplay == mode->hdisplay &&
                connector->modes[order[j]].vdisplay == mode->vdisplay)
            {
                dup = TRUE;
                break;
            }
        }
        if (dup)
            continue;

        sync_Description = AllocVec(SYNC_DESCNAME_LEN, MEMF_CLEAR);
        sync_mode = AllocVec(12 * sizeof(struct TagItem), MEMF_CLEAR);
        if (!sync_Description || !sync_mode)
        {
            FreeVec(sync_Description);
            FreeVec(sync_mode);
            break;
        }

        sprintf(sync_Description, "Amdgpu:%dx%d", mode->hdisplay, mode->vdisplay);
        DINFO(bug("[Amdgpu] %s: Description '%s'\n", __func__, sync_Description));

        sync_mode[smtagno].ti_Tag = aHidd_Sync_Description;  sync_mode[smtagno++].ti_Data = (IPTR)sync_Description;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_PixelClock;   sync_mode[smtagno++].ti_Data = mode->clock * 1000;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_HDisp;        sync_mode[smtagno++].ti_Data = mode->hdisplay;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_HSyncStart;   sync_mode[smtagno++].ti_Data = mode->hsync_start;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_HSyncEnd;     sync_mode[smtagno++].ti_Data = mode->hsync_end;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_HTotal;       sync_mode[smtagno++].ti_Data = mode->htotal;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_VDisp;        sync_mode[smtagno++].ti_Data = mode->vdisplay;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_VSyncStart;   sync_mode[smtagno++].ti_Data = mode->vsync_start;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_VSyncEnd;     sync_mode[smtagno++].ti_Data = mode->vsync_end;
        sync_mode[smtagno].ti_Tag = aHidd_Sync_VTotal;       sync_mode[smtagno++].ti_Data = mode->vtotal;
        sync_mode[smtagno].ti_Tag = TAG_DONE;

        modetags[1 + n].ti_Tag = aHidd_DMEnum_SyncTags;
        modetags[1 + n].ti_Data = (IPTR)sync_mode;
        n++;
    }

    modetags[1 + n].ti_Tag = TAG_DONE;
    FreeVec(order);
    *count = n;
    return modetags;
}

OOP_Object *Amdgpu__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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
    ULONG sync_count = 0;

    if (!XSD(cl)->kms.ready)
        return NULL;

    modetags = Amdgpu_CreateSyncTags(&XSD(cl)->kms, &sync_count);
    if (!modetags || sync_count == 0)
    {
        bug("[Amdgpu] %s: the display offers no modes\n", __func__);
        FreeVec(modetags);
        return NULL;
    }
    modetags[0].ti_Tag = aHidd_DMEnum_PixFmtTags;
    modetags[0].ti_Data = (IPTR)pftags;
    DINFO(bug("[Amdgpu] %s: %d usable modes found\n", __func__, sync_count);)

    struct TagItem amdgpunewtags[] =
    {
        { aHidd_Name            , (IPTR)"Amdgpu"     },
        { aHidd_HardwareName    , (IPTR)"AMD Radeon"   },
        { aHidd_ProducerName    , (IPTR)"Advanced Micro Devices, Inc."  },
        {TAG_MORE, (IPTR)msg->attrList }
    };
    struct pRoot_New amdgpunewmsg;

    /* The framebuffer is XRGB8888 whatever the host runs */
    pftags[4].ti_Data = 0x00ff0000;
    pftags[5].ti_Data = 0x0000ff00;
    pftags[6].ti_Data = 0x000000ff;
    pftags[0].ti_Data = mask_to_shift(pftags[4].ti_Data);
    pftags[1].ti_Data = mask_to_shift(pftags[5].ti_Data);
    pftags[2].ti_Data = mask_to_shift(pftags[6].ti_Data);
    pftags[3].ti_Data = 0;
    pftags[7].ti_Data = 0;
    pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
    pftags[9].ti_Data = AMDGPU_FB_DEPTH;
    pftags[10].ti_Data = AMDGPU_FB_BPP / 8;
    pftags[11].ti_Data = AMDGPU_FB_BPP;
    pftags[12].ti_Data = vHidd_StdPixFmt_Native;
    pftags[15].ti_Data = vHidd_BitMapType_Chunky;

    amdgpunewmsg.mID = msg->mID;
    amdgpunewmsg.attrList = amdgpunewtags;
    msg = &amdgpunewmsg;
    EnterFunc(bug("[Amdgpu] New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct AmdgpuHiddData *data = OOP_INST_DATA(cl, o);

        DINFO(bug("[Amdgpu] %s: object @ 0x%p\n", __func__, o);)

        XSD(cl)->amdgpuhidd = o;
        XSD(cl)->mouse.shape = NULL;
        XSD(cl)->hwCursor = XSD(cl)->kms.cursor_ok;

        data->ResetInterrupt.is_Node.ln_Name = (char *)amdgpunewtags[0].ti_Data;
        data->ResetInterrupt.is_Code = (VOID_FUNC)ResetHandler;
        data->ResetInterrupt.is_Data = &XSD(cl)->kms;
        AddResetCallback(&data->ResetInterrupt);

        struct TagItem displaytags[] =
        {
            { aHidd_Display_GfxHidd,  (IPTR)o        },
            { aHidd_Display_ModeTags, (IPTR)modetags },
            { TAG_DONE,               0              }
        };

        XSD(cl)->amdgpudisplay = OOP_NewObject(XSD(cl)->amdgpudisplayclass, NULL, displaytags);
        if (XSD(cl)->amdgpudisplay)
        {
            OOP_GetAttr(XSD(cl)->amdgpudisplay, aHidd_Display_DMEnumerator, (IPTR *)&XSD(cl)->dmenum);
        }
        else
        {
            OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
            XSD(cl)->amdgpuhidd = NULL;
            o = NULL;
        }
    }

    D(bug("[Amdgpu] %s: returning 0x%p\n", __func__, o);)

    return o;
}

VOID Amdgpu__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    FreeVec(XSD(cl)->mouse.shape);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID Amdgpu__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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
                *msg->storage = (IPTR)XSD(cl)->hwCursor;
            }
            break;

        case aoHidd_Gfx_DisplayDefault:
            *msg->storage = (IPTR)XSD(cl)->amdgpudisplay;
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
                                matag->ti_Data = (IPTR)XSD(cl)->kms.max_width * XSD(cl)->kms.max_height * (AMDGPU_FB_BPP / 8);
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

static inline BOOL Amdgpu_IsOwnBitMap(struct Amdgpu_staticdata *xsd, OOP_Object *bm)
{
    OOP_Class *bmcl = OOP_OCLASS(bm);

    return (bmcl == xsd->amdgpuonbmclass) || (bmcl == xsd->amdgpuoffbmclass);
}

VOID Amdgpu__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    UBYTE *src = NULL;
    UBYTE *dst = NULL;
    HIDDT_DrawMode mode;

    D(bug("[Amdgpu] %s()\n", __func__);)

    mode = GC_DRMD(msg->gc);
    /*
     * Only bitmaps of our own classes carry our instance data. A wrapper
     * such as the software-pointer framebuffer forwards the Drawable
     * attribute of the bitmap it wraps, so the attribute alone must not
     * be taken as proof of ownership.
     */
    if (Amdgpu_IsOwnBitMap(XSD(cl), msg->src))
        OOP_GetAttr(msg->src, aHidd_AmdgpuBitMap_Drawable, (IPTR *)&src);
    if (Amdgpu_IsOwnBitMap(XSD(cl), msg->dest))
        OOP_GetAttr(msg->dest, aHidd_AmdgpuBitMap_Drawable, (IPTR *)&dst);
    if (((dst == NULL) || (src == NULL))) /* no amdgpu bitmap */
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }
    else
    {
        struct BitmapData *srcbd = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct BitmapData *dstbd = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
        BOOL gpu = FALSE;

        if (mode == vHidd_GC_DrawMode_Copy && srcbd->bytesperpix == 4 && dstbd->bytesperpix == 4 &&
            srcbd->fb.handle)
        {
            if (srcbd == dstbd)
                gpu = Amdgpu_2D_CopyBox(&XSD(cl)->kms, srcbd, msg->srcX, msg->srcY,
                                        msg->destX, msg->destY, msg->width, msg->height);
            else if (!dstbd->fb.handle && msg->width * msg->height >= AMDGPU_2D_MIN_READ)
                gpu = Amdgpu_2D_Read(&XSD(cl)->kms, srcbd, msg->srcX, msg->srcY, msg->width, msg->height,
                                     dstbd->VideoData + msg->destY * dstbd->pitch + msg->destX * 4,
                                     dstbd->pitch);
        }

        if (!gpu)
        switch (mode)
        {
            case vHidd_GC_DrawMode_Copy:
            {
                if (srcbd->bytesperpix == dstbd->bytesperpix)
                {
                    switch (srcbd->bytesperpix)
                    {
                        case 1:
                            /* Not supported */
                            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
                        break;

                        case 2:
                            HIDD_BM_CopyMemBox16(msg->dest, srcbd->VideoData, msg->srcX, msg->srcY,
                                                dstbd->VideoData, msg->destX, msg->destY, msg->width, msg->height,
                                                srcbd->pitch, dstbd->pitch);
                            break;

                        case 3:
                            HIDD_BM_CopyMemBox24(msg->dest, srcbd->VideoData, msg->srcX, msg->srcY,
                                                dstbd->VideoData, msg->destX, msg->destY, msg->width, msg->height,
                                                srcbd->pitch, dstbd->pitch);
                            break;

                        case 4:
                            HIDD_BM_CopyMemBox32(msg->dest, srcbd->VideoData, msg->srcX, msg->srcY,
                                                dstbd->VideoData, msg->destX, msg->destY, msg->width, msg->height,
                                                srcbd->pitch, dstbd->pitch);
                            break;
                    }
                }
                else
                    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

                break;
            }
            default:
                D(bug("[Amdgpu] mode = %ld, src @ 0x%p dst @ 0x%p\n", mode, src, dst);)
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }

        if (dstbd->disp && dstbd->fb.fbid)
        {
            struct Box box = { msg->destX, msg->destY, msg->destX + msg->width - 1, msg->destY + msg->height - 1 };

            Amdgpu_KMS_DamageAdd(&XSD(cl)->kms, &box);
        }
    }

    D(bug("[Amdgpu] %s: done\n", __func__);)
}

static int Amdgpu_InitStatic(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Amdgpu] %s()\n", __func__);)

    Amdgpu_2D_Init();

    if (!OOP_ObtainAttrBases(attrbases))
    {
        D(bug("[Amdgpu] %s: attrbases init failed\n", __func__);)
        return FALSE;
    }

    D(bug("[Amdgpu] %s: initialised\n", __func__);)

    return TRUE;
}

static int Amdgpu_ExpungeStatic(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Amdgpu] %s()\n", __func__);)

    OOP_ReleaseAttrBases(attrbases);

    D(bug("[Amdgpu] %s: done\n", __func__);)

    return TRUE;
}

ADD2INITLIB(Amdgpu_InitStatic, 0)
ADD2EXPUNGELIB(Amdgpu_ExpungeStatic, 0)
