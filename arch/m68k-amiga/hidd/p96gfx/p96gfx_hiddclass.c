/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: P96 rtg card Gfx Hidd wrapper.
*/

#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/rawfmt.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <graphics/driver.h>
#include <graphics/displayinfo.h>
#include <intuition/intuitionbase.h>
#include <aros/libcall.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/gfx.h>

#include <aros/symbolsets.h>

#include <stdio.h>
#include <string.h>

#include LC_LIBDEFS_FILE

#include "p96gfx_intern.h"
#include "p96gfx_bitmap.h"
#include "p96gfx_rtg.h"

#define SDEBUG                  0
#define DEBUG                   0
#define DRTG(x)
#define DEXTRA(x)
#include <aros/debug.h>

#define SIZE_RESLIST            11
#define SIZE_PFLIST             19
#define SIZE_MODELIST           (5 + RGBFB_MaxFormats)

#define SPRITE_PEN_COUNT        16
/* REMOVE-ME: 4MB Hack (used for easier debug of vram <-> ram swapping) !!!!*/
/*#define USE_VRAM_HACK*/

static struct P96GfxData *P96GFX__GetGfxDataFromDisplay(OOP_Class *cl, OOP_Object *o, OOP_Object **gfx)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    OOP_GetAttr(o, aHidd_Display_GfxHidd, (IPTR *)gfx);
    return OOP_INST_DATA(CSD(cl)->gfxclass, *gfx);
}

static struct P96GfxData *P96GFX__GetGfxDataFromDMEnum(OOP_Class *cl, OOP_Object *o, OOP_Object **gfx)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    OOP_Object *display = NULL;

    OOP_GetAttr(o, aHidd_DMEnum_Display, (IPTR *)&display);
    OOP_GetAttr(display, aHidd_Display_GfxHidd, (IPTR *)gfx);
    return OOP_INST_DATA(CSD(cl)->gfxclass, *gfx);
}

static struct p96gfx_carddata *P96GFX__GetCardFromDisplay(OOP_Class *cl, OOP_Object *o)
{
    OOP_Object *gfx = NULL;

    return P96GFX__GetGfxDataFromDisplay(cl, o, &gfx)->cardData;
}

static struct p96gfx_carddata *P96GFX__GetCardFromDMEnum(OOP_Class *cl, OOP_Object *o)
{
    OOP_Object *gfx = NULL;

    return P96GFX__GetGfxDataFromDMEnum(cl, o, &gfx)->cardData;
}

HIDDT_ModeID *P96GFXDMEnum__Hidd_DMEnum__QueryModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_QueryModeIDs *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDMEnum(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);
    struct RTGMode *node;
    struct TagItem *tag, *tstate;
    ULONG minwidth = 0, maxwidth = 0xFFFFFFFF;
    ULONG minheight = 0, maxheight = 0xFFFFFFFF;
    OOP_Object **pf = NULL;
    HIDDT_ModeID *modeids;
    WORD cnt;

   if (cid->superforward)
        return (HIDDT_ModeID*)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    for (tstate = msg->queryTags; (tag = NextTagItem(&tstate)); )
    {
        switch (tag->ti_Tag)
        {
            case tHidd_GfxMode_MinWidth:
                minwidth = (ULONG)tag->ti_Data;
                break;

            case tHidd_GfxMode_MaxWidth:
                maxwidth = (ULONG)tag->ti_Data;
                break;

            case tHidd_GfxMode_MinHeight:
                minheight = (ULONG)tag->ti_Data;
                break;

            case tHidd_GfxMode_MaxHeight:
                maxheight = (ULONG)tag->ti_Data;
                break;

            case tHidd_GfxMode_PixFmts:
                pf = (OOP_Object**)tag->ti_Data;
                break;
        }
    }
    DB2(bug("[P96Gfx] %s: (%dx%d)-(%dx%d) %p\n", __func__, minwidth, minheight, maxwidth, maxheight, pf);)
    /* Two passes over the mode list: count the matches, then store them */
    modeids = NULL;
    for (;;) {
        cnt = 0;
        ForeachNode(&cid->rtglist, node) {
            if (node->width >= minwidth && node->width <= maxwidth && node->height >= minheight && node->height <= maxheight) {
                OOP_Object **pfp = NULL;
                if (pf) {
                    pfp = pf;
                    while (*pfp) {
                        if (*pfp == node->pf) {
                            pfp = NULL;
                            break;
                        }
                        pfp++;
                    }
                }
                if (!pfp) {
                    if (modeids)
                        modeids[cnt] = node->modeid;
                    cnt++;
                }
            }
        }
        if (modeids)
            break;
        modeids = AllocVec((cnt + 1) * sizeof(HIDDT_ModeID), MEMF_PUBLIC);
        if (!modeids)
            return NULL;
    }
    modeids[cnt] = vHidd_ModeID_Invalid;
    return modeids;
}

VOID P96GFXDMEnum__Hidd_DMEnum__ReleaseModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_ReleaseModeIDs *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDMEnum(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);

    if (cid->superforward)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    else
        FreeVec(msg->modeIDs);
}

HIDDT_ModeID P96GFXDMEnum__Hidd_DMEnum__NextModeID(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_NextModeID *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDMEnum(cl, o);
    struct RTGMode *node = NULL;
    HIDDT_ModeID mid = vHidd_ModeID_Invalid;

    DB2(bug("[P96Gfx] %s(%08x)\n", __func__, msg->modeID);)

    if (msg->modeID != vHidd_ModeID_Invalid) {
        ForeachNode(&cid->rtglist, node) {
            if (node->modeid == msg->modeID) {
                node = (struct RTGMode*)node->node.ln_Succ;
                break;
            }
        }
    }
    if (!node)
        node = (struct RTGMode*)cid->rtglist.lh_Head;
    if (node->node.ln_Succ) {
        mid = node->modeid;
        *msg->syncPtr = node->sync;
        *msg->pixFmtPtr = node->pf;
    }
    DB2(bug("[P96Gfx] %s:  %08x %p %p\n", __func__, mid, *msg->syncPtr, *msg->pixFmtPtr);)
    return mid;
}

BOOL P96GFXDMEnum__Hidd_DMEnum__GetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_GetMode *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDMEnum(cl, o);
    struct RTGMode *node;

    DB2(bug("[P96Gfx] %s(%08x)\n", __func__, msg->modeID);)

    if (cid->superforward)
        return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    ForeachNode(&cid->rtglist, node) {
        if (node->modeid == msg->modeID) {
            *msg->syncPtr = node->sync;
            *msg->pixFmtPtr = node->pf;
            DB2(bug("[P96Gfx] %s: %p %p\n", __func__, node->sync, node->pf);)
            return TRUE;
        }
    }
    DB2(bug("[P96Gfx] %s: FAIL\n", __func__);)
    /* Base class contract: outputs must be NULL when the mode is not found */
    *msg->syncPtr = NULL;
    *msg->pixFmtPtr = NULL;
    return FALSE;
}

struct RTGFormat
{
    UBYTE rgbformat;
    UBYTE pixfmt;
    ULONG rm, gm, bm, am;
    UBYTE rs, gs, bs, as;
    UBYTE endianswap;
};

static const struct RTGFormat formats[] =
{
    { RGBFB_CLUT,           vHidd_StdPixFmt_LUT8,       0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,  8, 16, 24,  0, FALSE },

    { RGBFB_B8G8R8A8,   vHidd_StdPixFmt_BGRA32,     0x0000ff00, 0x00ff0000, 0xff000000, 0x000000ff, 16,  8,  0, 24, FALSE },
    { RGBFB_R8G8B8A8,   vHidd_StdPixFmt_RGBA32,     0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff,  0,  8, 16, 24, FALSE },
    { RGBFB_A8B8G8R8,   vHidd_StdPixFmt_ABGR32,     0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, 24, 16,  8,  0, FALSE },
    { RGBFB_A8R8G8B8,   vHidd_StdPixFmt_ARGB32,     0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000,  8, 16, 24,  0, FALSE },

    { RGBFB_B8G8R8,         vHidd_StdPixFmt_BGR24,      0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000, 24, 16,  8,  0, FALSE },
    { RGBFB_R8G8B8,         vHidd_StdPixFmt_RGB24,      0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,  8, 16, 24,  0, FALSE },

    { RGBFB_R5G5B5,         vHidd_StdPixFmt_RGB15,      0x00007c00, 0x000003e0, 0x0000001f, 0x00000000, 17, 22, 27,  0, FALSE },
    { RGBFB_R5G6B5,         vHidd_StdPixFmt_RGB16,      0x0000f800, 0x000007e0, 0x0000001f, 0x00000000, 16, 21, 27,  0, FALSE },

    { RGBFB_R5G5B5PC,   vHidd_StdPixFmt_RGB15_LE,   0x00007c00, 0x000003e0, 0x0000001f, 0x00000000, 17, 22, 27,  0, TRUE },
    { RGBFB_R5G6B5PC,   vHidd_StdPixFmt_RGB16_LE,   0x0000f800, 0x000007e0, 0x0000001f, 0x00000000, 16, 21, 27,  0, TRUE },

    { RGBFB_B5G5R5PC,   vHidd_StdPixFmt_BGR15_LE,   0x0000003e, 0x000007c0, 0x0000f800, 0x00000000, 26, 21, 16,  0, TRUE },
    { RGBFB_B5G6R5PC,   vHidd_StdPixFmt_BGR16_LE,   0x0000001f, 0x000007e0, 0x0000f800, 0x00000000, 27, 21, 16,  0, TRUE },
};

#define NUM_RTGFORMATS (sizeof(formats) / sizeof(formats[0]))
_Static_assert(RGBFB_MaxFormats <= 256 && num_Hidd_AllPf <= 256,
               "RGB and pixel formats must fit a byte");

static const UBYTE rgbtypelist[] = {
    RGBFB_CLUT,

    RGBFB_R5G6B5PC,
    RGBFB_R5G5B5PC,
    RGBFB_B5G6R5PC,
    RGBFB_B5G5R5PC,
    RGBFB_R5G6B5,
    RGBFB_R5G5B5,

    RGBFB_B8G8R8,
    RGBFB_R8G8B8,

    RGBFB_B8G8R8A8,
    RGBFB_A8B8G8R8,
    RGBFB_A8R8G8B8,
    RGBFB_R8G8B8A8,
    0
};

char *P96GFX__MakeBoardName(struct p96gfx_carddata *cid)
{
    char *BoardName = gp(cid->boardinfo + PSSO_BoardInfo_BoardName);
    if (BoardName)
        return BoardName;
    ULONG boardType = gl(cid->boardinfo + PSSO_BoardInfo_BoardType);
    switch (boardType)
    {
        case P96BoardType_UAEGfx:
            return "UAE";
        case P96BoardType_Vampire:
            return "SAGA";
        case P96BoardType_Pixel64:
            return "Pixel64";
        case P96BoardType_PicassoIV:
            return "PicassoIV";
        case P96BoardType_PiccoloSD64:
            return "PiccoloSD64";
        case P96BoardType_PicassoII:
            return "PicassoII";
        case P96BoardType_Piccolo:
            return "Piccolo";
        case P96BoardType_RetinaBLT:
            return "RetinaBLT";
        case P96BoardType_A2410:
            return "A2410";
    }
    return NULL;
}

/*
 * The attributes of the sync, pixel format and mode limit tag lists, in list
 * order. The attribute bases are only known at run time, so the lists are
 * made from these offsets and a matching array of values.
 */
static const UBYTE synctagoffs[] =
{
    aoHidd_Sync_HDisp, aoHidd_Sync_VDisp, aoHidd_Sync_Description, aoHidd_Sync_PixelClock,
    aoHidd_Sync_HTotal, aoHidd_Sync_VTotal, aoHidd_Sync_HSyncStart, aoHidd_Sync_HSyncEnd,
    aoHidd_Sync_VSyncStart, aoHidd_Sync_VSyncEnd
};

static const UBYTE pftagoffs[] =
{
    aoHidd_PixFmt_RedShift, aoHidd_PixFmt_GreenShift, aoHidd_PixFmt_BlueShift, aoHidd_PixFmt_AlphaShift,
    aoHidd_PixFmt_RedMask, aoHidd_PixFmt_GreenMask, aoHidd_PixFmt_BlueMask, aoHidd_PixFmt_AlphaMask,
    aoHidd_PixFmt_CLUTMask, aoHidd_PixFmt_CLUTShift, aoHidd_PixFmt_ColorModel, aoHidd_PixFmt_Depth,
    aoHidd_PixFmt_BytesPerPixel, aoHidd_PixFmt_BitsPerPixel, aoHidd_PixFmt_StdPixFmt,
    aoHidd_PixFmt_BitMapType, aoHidd_PixFmt_SwapPixelBytes
};

static const UBYTE limittagoffs[] =
{
    aoHidd_Sync_HMin, aoHidd_Sync_VMin, aoHidd_Sync_HMax, aoHidd_Sync_VMax
};

static const IPTR limitvals[] =
{
    112, 112, 16384, 16384
};

static void P96GFX__PutTags(struct TagItem *tags, OOP_AttrBase base, const UBYTE *offs, const IPTR *vals, WORD count)
{
    do
    {
        tags->ti_Tag = base + *offs++;
        tags->ti_Data = *vals++;
        tags++;
    } while (--count);
}

OOP_Object *P96GFXCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct LibResolution *r;
    struct TagItem *reslist, *restags, *pflist, *modetags;
    struct TagItem mytags[] = {
        { aHidd_Name,                   (IPTR)"p96gfx.hidd" },
        { aHidd_HardwareName,           0                   },
        { aHidd_ProducerName,           (IPTR)"P96"         },
        { TAG_MORE,                     0UL                 }
    };
    struct pRoot_New mymsg;
    char *BoardName;
    UWORD supportedformats, gotmodes;
    WORD rescnt, i, j, k, l;
    struct p96gfx_carddata *cid = (struct p96gfx_carddata *)GetTagData(aHidd_P96Gfx_CardData, NULL, msg->attrList);

    if (!cid || cid->initialized)
        return NULL;

    D(bug("[P96CRASH] 0 new-enter cid=%p\n", cid));

    NEWLIST(&cid->rtglist);
    NEWLIST(&cid->bitmaplist);
    InitSemaphore(&cid->HWLock);
    InitSemaphore(&cid->MultiBMLock);

    BoardName  = P96GFX__MakeBoardName(cid);
    if (BoardName)
    {
        D(bug("[P96Gfx] %s: BoardName '%s'\n", __func__, BoardName);)
        cid->p96gfx_HWName = AllocVec(strlen(BoardName) + 18, MEMF_PUBLIC);
        cid->p96gfx_HWResTmplt = AllocVec(strlen(BoardName) + 7, MEMF_PUBLIC);
        sprintf(cid->p96gfx_HWName, "%s P96 Card Wrapper", BoardName);
        sprintf(cid->p96gfx_HWResTmplt, "%s:%%hx%%v", BoardName);
    }
    else
    {
        cid->p96gfx_HWResTmplt = (cid->CardBase) ? "RTGFX:%hx%v" : "P96:%hx%v";
        cid->p96gfx_HWName = "P96 Card Wrapper";
    }
    mytags[1].ti_Data = (IPTR)cid->p96gfx_HWName;

    supportedformats = gw(cid->boardinfo + PSSO_BoardInfo_RGBFormats);

    rescnt = 0;
    ForeachNode(cid->boardinfo + PSSO_BoardInfo_ResolutionsList, r) {
        rescnt++;
    }
    D(bug("[P96Gfx] %s: resolutions: %d, supportmask: %x\n", __func__, rescnt, supportedformats);)

    reslist = AllocVec(rescnt * SIZE_RESLIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    restags = AllocVec((rescnt + 1) * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    pflist = AllocVec(RGBFB_MaxFormats * SIZE_PFLIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);
    modetags = AllocVec(SIZE_MODELIST * sizeof(struct TagItem), MEMF_PUBLIC | MEMF_CLEAR);

    /* All four lists were allocated cleared, which is what terminates them */
    i = 0;
    ForeachNode(cid->boardinfo + PSSO_BoardInfo_ResolutionsList, r) {
        struct ModeInfo *mi = r->Modes[CHUNKY];
        IPTR vals[sizeof(synctagoffs)];

        vals[0] = r->Width;
        vals[1] = r->Height;
        vals[2] = (IPTR)cid->p96gfx_HWResTmplt;
        vals[3] = mi->PixelClock;
        vals[4] = mi->HorTotal;
        vals[5] = mi->VerTotal;
        vals[6] = mi->HorSyncStart;
        vals[7] = (mi->HorSyncStart + mi->HorSyncSize);
        vals[8] = mi->VerSyncStart;
        vals[9] = (mi->VerSyncStart + mi->VerSyncSize);
        P96GFX__PutTags(&reslist[i * SIZE_RESLIST], __IHidd_Sync, synctagoffs, vals, sizeof(synctagoffs));

        D(bug("[P96Gfx] %s:     %08x %d*%d\n", __func__, r, r->Width, r->Height);)
        restags[i].ti_Tag = aHidd_DMEnum_SyncTags;
        restags[i].ti_Data = (IPTR)&reslist[i * SIZE_RESLIST];
        i++;
    }

    D(bug("[P96CRASH] Q reslist-done cnt=%d\n", (int)rescnt));

    gotmodes = 0;
    k = 0;
    j = 0;
    for (i = 0; rgbtypelist[i]; i++) {
        UBYTE rgbtype = rgbtypelist[i];
        WORD depth = P96GFXRTG__GetDepth(1 << rgbtype);
        IPTR vals[sizeof(pftagoffs)];

        if (!((1 << rgbtype) & RGBFB_SUPPORTMASK) || depth == 0 || !((1 << rgbtype) & supportedformats)) {
            j++;
            continue;
        }
        for (l = 0; l < NUM_RTGFORMATS; l++) {
            if (formats[l].rgbformat == rgbtype)
                break;
        }
        if (l == NUM_RTGFORMATS) {
            j++;
            continue;
        }
        D(bug("[P96Gfx] %s: RTGFORMAT=%d found. Depth=%d\n", __func__, rgbtype, depth);)
        
        if (gotmodes & (1 << (depth / 8))) {
            D(bug("[P96Gfx] %s:   -> skipped\n", __func__);)
            j++;
            continue;
        }

        gotmodes |= 1 << (depth / 8);

        modetags[k].ti_Tag = aHidd_DMEnum_PixFmtTags;
        modetags[k].ti_Data = (IPTR)&pflist[j];
        k++;

        vals[0] = formats[l].rs;
        vals[1] = formats[l].gs;
        vals[2] = formats[l].bs;
        vals[3] = formats[l].as;
        vals[4] = formats[l].rm;
        vals[5] = formats[l].gm;
        vals[6] = formats[l].bm;
        vals[7] = formats[l].am;
        vals[8] = 0x000000FF;
        vals[9] = 0;
        vals[10] = depth <= 8 ? vHidd_ColorModel_Palette : vHidd_ColorModel_TrueColor;
        vals[11] = depth;
        vals[12] = (depth + 7) / 8;
        vals[13] = depth;
        vals[14] = formats[l].pixfmt;
        vals[15] = vHidd_BitMapType_Chunky;
        vals[16] = formats[l].endianswap;
        P96GFX__PutTags(&pflist[j], __IHidd_PixFmt, pftagoffs, vals, sizeof(pftagoffs));
        j += sizeof(pftagoffs) + 1;
    }

    P96GFX__PutTags(&modetags[k], __IHidd_Sync, limittagoffs, limitvals, sizeof(limittagoffs));
    k += sizeof(limittagoffs);
    modetags[k].ti_Tag = TAG_MORE;
    modetags[k].ti_Data = (IPTR)restags;
    mytags[3].ti_Data = (IPTR)msg->attrList;

    EnterFunc(bug("[P96Gfx] %s: tags @ %x\n", __func__, mytags);)

    mymsg.mID   = msg->mID;
    mymsg.attrList = mytags;
    msg = &mymsg;

    /* Register gfxmodes */
    D(bug("[P96CRASH] R pre-super o=%p\n", o));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    D(bug("[P96CRASH] B post-super o=%p\n", o));
    if (NULL != o)
    {
        struct TagItem displaytags[] = {
            { aHidd_Display_GfxHidd,     (IPTR)o               },
            { aHidd_Display_ModeTags,    (IPTR)modetags        },
            { aHidd_Display_DMEnumClass, (IPTR)csd->dmenumclass },
            { TAG_DONE,                  0                     }
        };
        struct P96GfxData *data = OOP_INST_DATA(cl, o);
        struct TagItem cmtags[] = {
            { aHidd_ColorMap_NumEntries,    SPRITE_PEN_COUNT    },
            { TAG_DONE,                     0UL                 }
        };
        HIDDT_ModeID *midp;

        D(bug("[P96Gfx] %s: Hidd Object @ 0x%p\n", __func__, o);)
        data->cardData = cid;
        NewList((struct List *)&data->bitmaps);

        data->spriteColors = OOP_NewObject(NULL, CLID_Hidd_ColorMap, cmtags);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR*)&data->pfo);
        cid->spritepencnt = SPRITE_PEN_COUNT;

        D(bug("[P96CRASH] P5 pre-disp cm=%p\n", data->spriteColors));
        csd->display = OOP_NewObject(csd->displayclass, NULL, displaytags);
        if (!csd->display)
        {
            OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
            o = NULL;
        }
        else
        {
            OOP_GetAttr(csd->display, aHidd_Display_DMEnumerator, (IPTR *)&csd->dmenum);
            if (!csd->dmenum)
            {
                OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
                OOP_DisposeObject(csd->display);
                csd->display = NULL;
                OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
                o = NULL;
            }
        }

        D(bug("[P96CRASH] C disp=%p dmenum=%p o=%p\n", csd->display, csd->dmenum, o));
        if (!o)
            goto new_cleanup;

        cid->initialized = 1;

        cid->superforward = TRUE;
        midp = HIDD_DMEnum_QueryModeIDs(csd->dmenum, NULL);
        for (i = 0; midp && midp[i] != vHidd_ModeID_Invalid; i++) {
            OOP_Object *sync, *pf;
            HIDDT_ModeID mid = midp[i];
            IPTR dwidth, dheight;
            struct RTGMode *node1, *node2;
            ULONG modeid, rtgmodeid, p96mode;

            if (!HIDD_DMEnum_GetMode(csd->dmenum, mid, &sync, &pf))
                continue;
            OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
            OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);

            DB2(bug("[P96Gfx] %s: w=%d h=%d mode=%08x sync=%x pf=%x\n", __func__, dwidth, dheight, mid, sync, pf);)

            modeid = vHidd_ModeID_Invalid;
            ForeachNode(cid->boardinfo + PSSO_BoardInfo_ResolutionsList, r) {
                if (r->Width == dwidth && r->Height == dheight) {
                    modeid = r->DisplayID;
                    break;
                }
            }
            if (modeid == vHidd_ModeID_Invalid) {
                D(bug("[P96Gfx] %s: w=%d h=%d not found!\n", __func__, dwidth, dheight);)
                continue;
            }

            p96mode = P96GFXRTG__GetFormat(csd, pf);
            rtgmodeid = (modeid & 0x00ff0000) | 0x1000 | (p96mode << 8);

            ForeachNode(&cid->rtglist, node2) {
                if (node2->width == dwidth && node2->height == dheight && node2->modeid == rtgmodeid)
                    break;
            }
            if (node2->node.ln_Succ != NULL) {
                D(bug("[P96Gfx] %s: w=%d h=%d mode=%08x already found!\n", __func__, dwidth, dheight, rtgmodeid);)
                continue;
            }

            node1 = AllocMem(sizeof(struct RTGMode), MEMF_CLEAR);
            node1->width = dwidth;
            node1->height = dheight;
            node1->pf = pf;
            node1->sync = sync;
            node1->modeid = rtgmodeid;
            AddTail(&cid->rtglist, &node1->node);

            DB2(bug("[P96Gfx] %s: Added %dx%d %08x %d\n", __func__, node1->width, node1->height, node1->modeid, p96mode);)
        }
        if (midp)
            HIDD_DMEnum_ReleaseModeIDs(csd->dmenum, midp);
        cid->superforward = FALSE;
        D(bug("[P96CRASH] D modereg-done o=%p\n", o));
    }

new_cleanup:
    FreeVec(restags);
    FreeVec(reslist);
    FreeVec(pflist);
    FreeVec(modetags);

    D(bug("[P96CRASH] 2 new-done o=%p\n", o));
    ReturnPtr("[P96Gfx]:New", OOP_Object *, o);
}

VOID P96GFXDisplay__Hidd_Display__NominalDimensions(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_NominalDimensions *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx] %s()\n", __func__));

    /* The architecture nominals describe the chipset display; report
       the RTG display's own, at the deepest depth the card offers */
    if (msg->width)
        *(msg->width) = 640;
    if (msg->height)
        *(msg->height) = 480;
    if (msg->depth)
    {
        HIDDT_ModeID *midp;
        UBYTE maxdepth = 8;

        midp = HIDD_DMEnum_QueryModeIDs(csd->dmenum, NULL);
        if (midp)
        {
            ULONG i;

            for (i = 0; midp[i] != vHidd_ModeID_Invalid; i++)
            {
                OOP_Object *sync, *pf;
                IPTR depth = 0;

                if (HIDD_DMEnum_GetMode(csd->dmenum, midp[i], &sync, &pf) && pf)
                {
                    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
                    if (depth > maxdepth)
                        maxdepth = depth;
                }
            }
            HIDD_DMEnum_ReleaseModeIDs(csd->dmenum, midp);
        }
        *(msg->depth) = maxdepth;
    }
}

/********** GfxHidd::Dispose()  ******************************/
OOP_Object *P96GFXDisplay__Hidd_Display__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    D(bug("[P96Gfx] %s()\n", __func__));

    if (msg->cl == CSD(cl)->basebm)
    {
        struct p96gfx_staticdata        *csd = CSD(cl);
        HIDDT_ModeID                    modeid;
        struct pHidd_Display_CreateObject   p;
        struct TagItem tags[] =
        {
            { TAG_IGNORE,       TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_MORE,         (IPTR)msg->attrList }
        };

        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        if (modeid != vHidd_ModeID_Invalid) {
            tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
            tags[0].ti_Data = (IPTR)CSD(cl)->bmclass;
        }
        p.mID = msg->mID;
        p.cl = msg->cl;
        p.attrList = tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    ReturnPtr("[P96Gfx]:CreateObject", OOP_Object *, object);
}

VOID P96GFXCl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    ULONG idx;

    //bug("P96GFXCl__Root__Get %x\n", msg->attrID);

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        //bug("=%x\n", idx);
        switch (idx)
        {
            case aoHidd_Gfx_SupportsHWCursor:
                *msg->storage = cid->hardwaresprite;
                return;
            case aoHidd_Gfx_NoFrameBuffer:
                *msg->storage = TRUE;
                return;
            case aoHidd_Gfx_IsWindowed:
                *msg->storage = FALSE;
                return;
            case aoHidd_Gfx_DriverName:
                *msg->storage = (IPTR)"P96GFX";
                return;
            case aoHidd_Gfx_DisplayDefault:
                *msg->storage = (IPTR)csd->display;
                return;
        }
    }
    Hidd_P96Gfx_Switch(msg->attrID, idx)
    {
        case aoHidd_P96Gfx_CardData:
            *msg->storage = (IPTR)data->cardData;
            return;
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID P96GFXDisplay__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    ULONG idx;

    /*
     * NB: do NOT fetch the gfx data (which queries aHidd_Display_GfxHidd on this
     * very object) unconditionally here - that re-enters this Get method and
     * recurses infinitely. Only resolve it for the attribute that needs it.
     */
    Hidd_Display_Switch(msg->attrID, idx)
    {
        case aoHidd_Display_SpriteTypes:
        {
            struct p96gfx_carddata *cid = P96GFX__GetCardFromDisplay(cl, o);

            *msg->storage = cid->hardwaresprite ? vHidd_SpriteType_3Plus1 : 0;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID P96GFXCl__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    struct TagItem  *tag, *tstate;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        ULONG idx;
        D(bug("[P96Gfx] %s: %x\n", __func__, tag->ti_Tag);)
        if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
            D(bug("[P96Gfx] %s:   ->%d\n", __func__, idx);)
            switch(idx)
            {
            case aoHidd_Gfx_ActiveCallBack:
                cid->acb = (void *)tag->ti_Data;
                break;

            case aoHidd_Gfx_ActiveCallBackData:
                cid->acbdata = (APTR)tag->ti_Data;
                break;
            }
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

ULONG P96GFXDisplay__Hidd_Display__MakeViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_MakeViewPort *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDisplay(cl, o);
    struct HIDD_ViewPortData *vpd = msg->Data;

    D(bug("[P96Gfx] %s()\n", __func__));

    cid->viewport = NULL;
    if (vpd && vpd->vpe)
        cid->viewport = vpd->vpe->ViewPort;

    return MVP_OK;
}

void P96GFXDisplay__Hidd_Display__DeinitViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_DeinitViewPort *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDisplay(cl, o);

    D(bug("[P96Gfx] %s()\n", __func__));

    cid->viewport = NULL;
}

static void P96GFXDisplay__DoShow(OOP_Class *cl, OOP_Object *o, OOP_Object *bm, struct ViewPort *vp, BOOL offonly)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDisplay(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);
    struct IntuitionBase *ib = (struct IntuitionBase*)csd->cs_IntuitionBase;
    struct ViewPort *vpi = NULL;

    D(bug("[P96Gfx] %s()\n", __func__));

    D(bug("[P96CRASH] 3 doshow-enter bm=%p\n", bm));

    if (ib->FirstScreen)
        vpi = &ib->FirstScreen->ViewPort;

    D(bug("[P96Gfx] %s: b=%p vp=%p vpi=%p acb=%p acbd=%p\n", __func__, bm, vp, vpi, cid->acb, cid->acbdata);)

    if (bm && vpi == vp) {
        /* we are topmost screen -> show our display */
        IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};

        if (offonly)
            return;

        D(bug("[P96CRASH] 4 doshow-visible bm=%p\n", bm));
        OOP_SetAttrs(bm, (struct TagItem *)tags);

        if (cid->acb)
            cid->acb(cid->acbdata, bm);

    } else if (bm) {
        /* we are not topmost -> turn off our display */
        IPTR tags[] = {aHidd_BitMap_Visible, FALSE, TAG_DONE};
        OOP_SetAttrs(bm, (struct TagItem *)tags);
    } else {
    
        LOCK_HW

        /* no display */
        /* When changing, check P96GFXBitmap__HideScreen */
        SetDisplay(cid, FALSE);
        SetSwitch(cid, FALSE);
        if (cid->disp)
        {
            cid->disp->locked--;
            cid->disp = NULL;
        }

        UNLOCK_HW
    }
}

OOP_Object *P96GFXDisplay__Hidd_Display__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_Show *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDisplay(cl, o);

    D(bug("[P96Gfx] %s()\n", __func__));

    P96GFXDisplay__DoShow(cl, o, msg->bitMap, cid->viewport, FALSE);
    return msg->bitMap;
}

ULONG P96GFXDisplay__Hidd_Display__InitViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_InitViewPorts *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDisplay(cl, o);
    struct HIDD_ViewPortData *vpd = msg->Data;
    OOP_Object *bm = NULL;
    struct ViewPort *vp = NULL;

    D(bug("[P96Gfx] %s()\n", __func__));

    if (vpd) {
        bm = vpd->Bitmap;
        if (vpd->vpe)
            vp = vpd->vpe->ViewPort;
    }
    cid->viewport = vp;
    P96GFXDisplay__DoShow(cl, o, bm, vp, FALSE);

    D(bug("[P96Gfx] %s: viewport=%p\n", __func__, cid->viewport);)
    return MCOP_OK;
}

ULONG P96GFXDisplay__Hidd_Display__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_ShowViewPorts *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct HIDD_ViewPortData *vpd = msg->Data;
    OOP_Object *bm = NULL;
    struct ViewPort *vp = NULL;

    D(bug("[P96Gfx] %s()\n", __func__));

    if (vpd) {
        bm = vpd->Bitmap;
        if (vpd->vpe)
            vp = vpd->vpe->ViewPort;
    }
    P96GFXDisplay__DoShow(cl, o, bm, vp, FALSE);
    return TRUE;
}

VOID P96GFXCl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    struct P96GfxBitMapData *sdata = NULL;
    struct P96GfxBitMapData *ddata = NULL;
    struct RenderInfo risrc, ridst;

    D(bug("[P96Gfx] %s()\n", __func__));

    if (OOP_OCLASS(msg->src) == csd->bmclass) sdata = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
    if (OOP_OCLASS(msg->dest) == csd->bmclass) ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
    
    if (!sdata || !ddata)
    {
        DEXTRA(
          bug("[P96Gfx] %s: ==== copybox: unknown bitmap %p %p drawmode %d\n", __func__, sdata, ddata, mode);
          if (!sdata) bug("[P96Gfx] %s:   src: %s\n", __func__, OOP_OCLASS(msg->src)->ClassNode.ln_Name ? OOP_OCLASS(msg->src)->ClassNode.ln_Name : "unknown");
          if (!ddata) bug("[P96Gfx] %s:   dst: %s\n", __func__, OOP_OCLASS(msg->dest)->ClassNode.ln_Name ? OOP_OCLASS(msg->dest)->ClassNode.ln_Name : "unknown");
         )

        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }

    if (sdata->rgbformat != ddata->rgbformat) {
        DEXTRA(bug("[P96Gfx] %s: ==== copybox: format mismatch %d %d drawmode %d\n", __func__, sdata->rgbformat, ddata->rgbformat, mode);)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }

    LOCK_MULTI_BITMAP
    LOCK_BITMAP(sdata)
    LOCK_BITMAP(ddata)
    UNLOCK_MULTI_BITMAP

    P96GFXBitmap__WaitBlit(csd, cid);
    
    if (!sdata->invram || !ddata->invram)
    {
        /* Blit from VRAM to RAM or from RAM to VRAM */
        DEXTRA(
          bug("[P96Gfx] %s: == VRAM <-> RAM blit bpp %d\n", __func__, sdata->bytesperpixel);
          bug("[P96Gfx] %s: %p to %p %d,%d -> %d,%d  %d x %d  modulo %d %d\n",
                                                                        __func__, sdata->VideoData, ddata->VideoData,
                                                                        msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height,
                                                                        sdata->bytesperline,
                                                                        ddata->bytesperline);
         )
        
        if (mode == vHidd_GC_DrawMode_Copy)
        {
            /* CopyMemBox8..32 are numbered in pixel size order */
            if (P96_BPP_VALID(sdata->bytesperpixel))
                HIDD_BM_CopyMemBox8_(P96_BPP_BASE(sdata->bytesperpixel), msg->dest,
                                    sdata->VideoData,
                                    msg->srcX,
                                    msg->srcY,
                                    ddata->VideoData,
                                    msg->destX,
                                    msg->destY,
                                    msg->width,
                                    msg->height,
                                    sdata->bytesperline,
                                    ddata->bytesperline);
            
        } /* if (mode == vHidd_GC_DrawMode_Copy) */
        else
        {
            DEXTRA(if (mode == vHidd_GC_DrawMode_Clear) bug("[P96Gfx] %s:  clear blit %d x %d done by superclass\n", __func__, msg->width, msg->height);)
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    } /* if (!sdata->invram || !ddata->invram) */
    else
    {
        DEXTRA(if (mode == vHidd_GC_DrawMode_Clear) bug("[P96Gfx] %s:  clear blit %d x %d done by me\n", __func__, msg->width, msg->height);)

        P96GFXRTG__MakeRenderInfo(&risrc, sdata);
        P96GFXRTG__MakeRenderInfo(&ridst, ddata);
        
        LOCK_HW
        
        if (!BlitRectNoMaskComplete(cid, &risrc, &ridst,
            msg->srcX, msg->srcY, msg->destX, msg->destY,
            msg->width, msg->height, modetable[mode], sdata->rgbformat))
        {
            DEXTRA(bug("[P96Gfx] %s: == unhandled blit %d x %d drawmode %d. super must help\n", __func__, msg->width, msg->height, mode);)
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
        
        UNLOCK_HW
    }
    
    UNLOCK_BITMAP(sdata)
    UNLOCK_BITMAP(ddata)

}

BOOL P96GFXCl__Hidd_Gfx__CopyBoxMasked(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBoxMasked *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    DEXTRA(bug("[P96Gfx] %s()\n", __func__));

    P96GFXBitmap__WaitBlit(csd, cid);

    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

static UBYTE *P96GFXCl__PrepareSprite(struct p96gfx_staticdata *csd, struct p96gfx_carddata *cid, ULONG store, ULONG size, ULONG width, ULONG height, struct pHidd_Display_SetCursorShape *msg)
{
    UBYTE *p;

    D(bug("[P96Gfx] %s()\n", __func__));

    pb(cid->boardinfo + PSSO_BoardInfo_MouseXOffset, msg->xoffset);
    pb(cid->boardinfo + PSSO_BoardInfo_MouseYOffset, msg->yoffset);
    p = (UBYTE*)gp(cid->boardinfo + store);
    if (p == NULL || width != cid->sprite_width || height != cid->sprite_height) {
        FreeVec(p);
        p = AllocVec(size, MEMF_CLEAR | MEMF_PUBLIC);
        pp(cid->boardinfo + store, p);
        if (!p) {

            return NULL;
        }
        cid->sprite_width = width;
        cid->sprite_height = height;
    }
    return p;
}

#ifndef MAX
#define MAX(x,y) (x)>(y)?(x):(y)
#define MIN(x,y) (x)<(y)?(x):(y)
#endif

static UBYTE P96GFXCl__PickPen(struct p96gfx_staticdata *csd, ULONG pixel, OOP_Object *cm)
{
    UBYTE retval = 0;
    ULONG diff=(ULONG)-1, tmp;
    int i;
    
    DEXTRA(bug("[P96Gfx] %s()\n", __func__));

    for (i = 0; i < 3; i++) {
        HIDDT_Color c;
        HIDD_CM_GetColor(cm, i + 1, &c);
        tmp = (MAX(c.red, ((pixel & 0xFF0000) >> 16)) - MIN(c.red, ((pixel & 0xFF0000) >> 16))) +
                    (MAX(c.green, ((pixel & 0xFF00) >> 8)) - MIN(c.green, ((pixel & 0xFF00) >> 8))) +
                    (MAX(c.blue, (pixel & 0xFF)) - MIN(c.blue, (pixel & 0xFF)));
        if (tmp < diff)
        {
            diff = tmp;
            retval = i + 1;
        }
    }
    return retval;
}

BOOL P96GFXDisplay__Hidd_Display__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorShape *msg)
{
    OOP_Object *gfx = NULL;
    struct P96GfxData *data = P96GFX__GetGfxDataFromDisplay(cl, o, &gfx);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    OOP_Object *cm = NULL;
    OOP_Object *bmPFObj = NULL;
    HIDDT_PixelFormat *bmPF = NULL;
    IPTR pf, bmcmod, width, height;
    WORD x, y, hiressprite, i;
    ULONG flags;

    D(bug("[P96Gfx] %s()\n", __func__);)


    /* No hardware sprite: let the base Display class run its software cursor.
       Must agree with aoHidd_Gfx_SupportsHWCursor or two pointers are drawn */
    if (!cid->hardwaresprite)
        return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (IPTR*)&cm);
    OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (IPTR*)&bmPFObj);
    OOP_GetAttr(bmPFObj, aHidd_PixFmt_ColorModel, &bmcmod);
    if (bmcmod == vHidd_ColorModel_TrueColor)
    {
        OOP_GetAttr(bmPFObj, aHidd_PixFmt_StdPixFmt, (IPTR*)&pf);
        bmPF = (HIDDT_PixelFormat *)HIDD_DMEnum_GetPixFmt(csd->dmenum, pf);
    }

    LOCK_HW
    if (cm) {
        for (i = 0; i < 3; i++) {
            HIDDT_Color c;
            HIDD_CM_GetColor(cm, i + 1, &c);
            HIDD_P96GFX_SetCursorPen(gfx, i, c);
        }
    }
    else
    {
        // TODO: Calculate histogram of image and choose best colors
    }

    /*
     * A sprite row carries one plane 0 entry followed by one plane 1
     * entry of 16 pixels per word each. Plain sprite data is one word
     * per plane; BIF_HIRESSPRITE doubles that to 32 pixels. A shape too
     * wide for the sprite is clipped to what the row can carry.
     */
    flags = gl(cid->boardinfo + PSSO_BoardInfo_Flags);
    if (cid->hiressprite && (width > 16))
    {
        hiressprite = 2;
        flags |= BIF_HIRESSPRITE;
    }
    else
    {
        hiressprite = 1;
        flags &= ~BIF_HIRESSPRITE;
    }
    pl(cid->boardinfo + PSSO_BoardInfo_Flags, flags);

    if (width > 16 * hiressprite)
        width = 16 * hiressprite;

    pb(cid->boardinfo + PSSO_BoardInfo_MouseWidth, width);
    pb(cid->boardinfo + PSSO_BoardInfo_MouseHeight, height);

    Forbid();
    DB2(bug("[P96Gfx] %s: filling planar buffer ...\n", __func__);)
    {
        UWORD *pw;

        pw = (UWORD *)P96GFXCl__PrepareSprite(csd, cid, PSSO_BoardInfo_MouseImage, 4 + 4 + ((width + 15) & ~15) / 8 * height * 2, width, height, msg);
        if (!pw)
        {
            Permit();
            UNLOCK_HW
            return FALSE;
        }
        /*
         * Built a whole word at a time and assigned rather than or'ed
         * in: a sprite of unchanged size keeps its buffer, and the
         * pixels the new shape leaves clear have to clear the old
         * one's.
         */
        pw += 2 * hiressprite;
        for(y = 0; y < height; y++) {
            for(WORD w = 0; w < hiressprite; w++) {
                UWORD pix1 = 0, pix2 = 0;

                for(WORD b = 0; b < 16; b++) {
                    UBYTE c = 0;

                    x = w * 16 + b;
                    if (x < width)
                    {
                        HIDDT_Pixel pix = HIDD_BM_GetPixel(msg->shape, x, y);

                        if (bmcmod != vHidd_ColorModel_TrueColor)
                            c = pix;
                        else if ((ALPHA_COMP(pix, bmPF) & 0xFF00) == 0xFF00)
                            c = P96GFXCl__PickPen(csd, ((RED_COMP(pix, bmPF) & 0xFF00) << 8) | (GREEN_COMP(pix, bmPF) & 0xFF00) | ((BLUE_COMP(pix, bmPF) >> 8) & 0xFF), data->spriteColors);
                    }
                    pix1 = (pix1 << 1) | ((c & 1) ? 1 : 0);
                    pix2 = (pix2 << 1) | ((c & 2) ? 1 : 0);
                }
                pw[w] = pix1;
                pw[hiressprite + w] = pix2;
            }
            pw += hiressprite * 2;
        }
    }

#if (0)
    DB2(bug("[P96Gfx] %s: filling clut buffer ...\n", __func__);)
    {
        UBYTE *pb;

        pb = P96GFXCl__PrepareSprite(csd, cid, PSSO_BoardInfo_MouseChunky, 4 + 4 + ((width + 15) & ~15) * height, width, height, msg);
        if (!pb)
        {
            Permit();
            UNLOCK_HW
            return FALSE;
        }
        pb += 2 * hiressprite;
        for(y = 0; y < height; y++) {
            for(x = 0; x < width; x++) {
                UBYTE c;
                if (bmcmod != vHidd_ColorModel_TrueColor)
                    c = HIDD_BM_GetPixel(msg->shape, x, y);
                else
                {
                    HIDDT_Pixel pix = HIDD_BM_GetPixel(msg->shape, x, y);
                    if ((ALPHA_COMP(pix, bmPF) & 0xFF00) == 0xFF00)
                        c = P96GFXCl__PickPen(csd, ((RED_COMP(pix, bmPF) & 0xFF00) << 8) | (GREEN_COMP(pix, bmPF) & 0xFF00) | ((BLUE_COMP(pix, bmPF) >> 8) & 0xFF), data->spriteColors);
                    else c = 0;
                }
                pb[x] = c;
            }
            pb += width;
        }
    }
#endif
    Permit();

    DB2(bug("[P96Gfx] %s: loading hw sprite ...\n", __func__));
    D(bug("[P96CRASH] 8 cursor-setimage w=%d h=%d\n", (int)width, (int)height));
    SetSpriteImage(cid);

    UNLOCK_HW
    DB2(bug("[P96Gfx] %s: hw sprite loaded\n", __func__));

    return TRUE;
}
                             
BOOL P96GFXDisplay__Hidd_Display__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorPos *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDisplay(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx] %s()\n", __func__));

    if (!cid->hardwaresprite)
        return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    LOCK_HW
    pw(cid->boardinfo + PSSO_BoardInfo_MouseX, msg->x + (BYTE)cid->boardinfo[PSSO_BoardInfo_MouseXOffset]);
    pw(cid->boardinfo + PSSO_BoardInfo_MouseY, msg->y + (BYTE)cid->boardinfo[PSSO_BoardInfo_MouseYOffset]);
    SetSpritePosition(cid);
    UNLOCK_HW

    return TRUE;
}

VOID P96GFXDisplay__Hidd_Display__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorVisible *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDisplay(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx] %s()\n", __func__));

    if (!cid->hardwaresprite)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }
    LOCK_HW
    SetSprite(cid, msg->visible);
    UNLOCK_HW
}

VOID P96GFXCl__Hidd_P96Gfx__SetCursorPen(OOP_Class *cl, OOP_Object *o, struct pHidd_P96Gfx_SetCursorPen *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx] %s()\n", __func__));

    HIDD_CM_SetColors(data->spriteColors, &msg->color, msg->pen, 1, data->pfo);
    SetSpriteColor(cid, msg->pen, msg->color.red, msg->color.green, msg->color.blue);
}

BOOL P96GFXDMEnum__Hidd_DMEnum__CheckMode(OOP_Class *cl, OOP_Object *o, struct pHidd_DMEnum_CheckMode *msg)
{
    struct p96gfx_carddata *cid = P96GFX__GetCardFromDMEnum(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);
    IPTR width, height, bpp;

    D(bug("[P96Gfx] %s()\n", __func__));

    OOP_GetAttr(msg->sync, aHidd_Sync_HDisp, &width);
    OOP_GetAttr(msg->sync, aHidd_Sync_VDisp, &height);
    OOP_GetAttr(msg->pixFmt, aHidd_PixFmt_BytesPerPixel, &bpp);
    if (width > cid->maxwidth[bpp])
        return FALSE;
    if (height > cid->maxheight[bpp])
        return FALSE;
    return width * height * bpp < cid->vram_size;
}

/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/***************************************** Driver Probing and Initialization Functions *********************************************************/

/*
 * The attribute and method bases of struct p96gfx_staticdata, in the order
 * of its fields: hiddAttrBase onwards and hiddBitMapBase onwards.
 */
static const CONST_STRPTR attrbase_ids[] =
{
    IID_Hidd,
    IID_Hidd_BitMap,
    IID_Hidd_BitMap_P96,
    IID_Hidd_GC,
    IID_Hidd_Sync,
    IID_Hidd_PixFmt,
    IID_Hidd_Gfx,
    IID_Hidd_Display,
    IID_Hidd_DMEnum,
    IID_Hidd_P96Gfx,
    IID_Hidd_ColorMap,
    NULL
};

static const CONST_STRPTR methodbase_ids[] =
{
    IID_Hidd_BitMap,
    IID_Hidd_ColorMap,
    IID_Hidd_Gfx,
    IID_Hidd_Display,
    IID_Hidd_DMEnum,
    IID_Hidd_P96Gfx,
    NULL
};

#define P96_BASE_SLOT(first, type, field, n)                                    \
    _Static_assert(offsetof(struct p96gfx_staticdata, field) ==                 \
                   offsetof(struct p96gfx_staticdata, first) + (n) * sizeof(type), \
                   #field " must be slot " #n " of the table")
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddBitMapAttrBase, 1);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddP96GFXBitMapAttrBase, 2);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddGCAttrBase, 3);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddSyncAttrBase, 4);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddPixFmtAttrBase, 5);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddGfxAttrBase, 6);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddDisplayAttrBase, 7);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddDMEnumAttrBase, 8);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddP96GfxAttrBase, 9);
P96_BASE_SLOT(hiddAttrBase, OOP_AttrBase, hiddColorMapAttrBase, 10);
_Static_assert(sizeof(attrbase_ids) / sizeof(attrbase_ids[0]) == 10 + 2,
               "one id for each attribute base");
P96_BASE_SLOT(hiddBitMapBase, OOP_MethodID, hiddColorMapBase, 1);
P96_BASE_SLOT(hiddBitMapBase, OOP_MethodID, hiddGfxBase, 2);
P96_BASE_SLOT(hiddBitMapBase, OOP_MethodID, hiddDisplayBase, 3);
P96_BASE_SLOT(hiddBitMapBase, OOP_MethodID, hiddDMEnumBase, 4);
P96_BASE_SLOT(hiddBitMapBase, OOP_MethodID, hiddP96GfxBase, 5);
_Static_assert(sizeof(methodbase_ids) / sizeof(methodbase_ids[0]) == 5 + 2,
               "one id for each method base");

/*
 * Each interface is released by name, whether or not its base was obtained
 * (the expunge after an init that failed early comes here too).
 */
static void P96GFX__FreeAttrBases(LIBBASETYPEPTR LIBBASE, struct p96gfx_staticdata *csd)
{
    const CONST_STRPTR *id;

    for (id = attrbase_ids; *id; id++)
        OOP_ReleaseAttrBase(*id);
}

AROS_INTP(rtg_vblank);

struct P96RTGmode
{
    ULONG clock;
    UWORD w, h;
    UWORD htotal, vtotal;
    UBYTE id;
    UBYTE hpos, vpos;
    UBYTE hsync, vsync;
    UBYTE flags;
};

static const struct P96RTGmode rtgmodes[] =
{
    {  13020000,  320, 240,  408, 262, 1, 32,  5,  24,  1, GMF_HPOLARITY | GMF_VPOLARITY | GMF_DOUBLESCAN },
    {  25060000,  640, 480,  800, 525, 3, 40, 11,  88,  2, GMF_HPOLARITY | GMF_VPOLARITY },
    {  32010000,  800, 600, 1024, 625, 4, 56,  1,  40,  2, 0 },
    {  64430000, 1024, 768, 1336, 800, 5, 48,  4, 112,  5, GMF_HPOLARITY | GMF_VPOLARITY },
    {  85010000, 1152, 900, 1496, 973, 6, 48,  5,  40,  4, GMF_HPOLARITY | GMF_VPOLARITY },
    { 106360000, 1440, 900, 1904, 932, 7, 80,  0,   0, 10, GMF_HPOLARITY | GMF_VPOLARITY },
};

/* real RTG only */
static BOOL P96GFX__PopulateModeInfo(struct p96gfx_staticdata *csd, struct p96gfx_carddata *cid, struct LibResolution *res, const struct P96RTGmode *mode)
{
    UWORD rgbformat;
    BOOL ok = FALSE;

    D(bug("[HiddP96Gfx] %s()\n", __func__);)

    for (rgbformat = 0; rgbformat < RGBFB_MaxFormats; rgbformat++) {
        ULONG clockindex;
        UBYTE depth, index;
        struct ModeInfo *mi;
        UWORD maxhval, maxvval, maxhres, maxvres;

        if (!((1 << rgbformat) & RGBFB_SUPPORTMASK))
            continue;
        if (!((1 << rgbformat) & gw(cid->boardinfo + PSSO_BoardInfo_RGBFormats)))
            continue;
        depth = P96GFXRTG__GetDepth(1 << rgbformat);
        index = (depth + 7) / 8;
        if (res->Modes[index])
            continue;

        maxhval = gw(cid->boardinfo + PSSO_BoardInfo_MaxHorValue + index * 2);
        maxvval = gw(cid->boardinfo + PSSO_BoardInfo_MaxVerValue + index * 2);
        maxhres = gw(cid->boardinfo + PSSO_BoardInfo_MaxHorResolution + index * 2);
        maxvres = gw(cid->boardinfo + PSSO_BoardInfo_MaxVerResolution + index * 2);

        if (mode->htotal > maxhval || mode->vtotal > maxvval ||
            mode->w > maxhres || mode->h > maxvres)
            continue;

        mi = AllocMem(sizeof(struct ModeInfo), MEMF_CLEAR | MEMF_PUBLIC);
        if (!mi)
            continue;
        mi->OpenCount = 1;
        mi->Active = TRUE;
        mi->Width = mode->w;
        mi->Height = mode->h;
        mi->Depth = depth;
        mi->HorTotal = mode->htotal;
        mi->VerTotal = mode->vtotal;
        /* HorBlankSize and VerBlankSize stay 0: no mode has a border */
        mi->HorSyncStart = mode->hpos;
        mi->VerSyncStart = mode->vpos;
        mi->HorSyncSize = mode->hsync;
        mi->VerSyncSize = mode->vsync;
        mi->Flags = mode->flags;
        clockindex = ResolvePixelClock(cid, mi, mode->clock, rgbformat);
        mi->PixelClock = GetPixelClock(cid, mi, clockindex, rgbformat);
        DRTG(bug("[HiddP96Gfx] %s: %d,%p: %dx%dx%d ci=%d clk=%d (%d/%d)\n",
            __func__, index, mi, mi->Width, mi->Height, mi->Depth,
            clockindex, mi->PixelClock, mi->Numerator, mi->Denominator);)
        res->Modes[index] = mi;
        ok = TRUE;
    }
    return ok;
}

static void P96GFX__PopulateResolutionsList(struct p96gfx_staticdata *csd, struct p96gfx_carddata *cid)
{
    struct LibResolution *node;
    UWORD cnt;

    D(bug("[HiddP96Gfx] %s()\n", __func__);)

    NEWLIST((cid->boardinfo + PSSO_BoardInfo_ResolutionsList));
    for (cnt = 0; cnt < sizeof(rtgmodes) / sizeof(rtgmodes[0]); cnt++) {
        const struct P96RTGmode *mode = &rtgmodes[cnt];
        node = AllocMem(sizeof(struct LibResolution), MEMF_CLEAR | MEMF_PUBLIC);
        if (!node)
            break;
        node->Width = mode->w;
        node->Height = mode->h;
        node->DisplayID = 0x50001000 | (mode->id << 16);
        node->BoardInfo = cid->boardinfo;
        if (P96GFX__PopulateModeInfo(csd, cid, node, mode))
            AddTail((struct List*)(cid->boardinfo + PSSO_BoardInfo_ResolutionsList), (struct Node*)node);
        else
            FreeMem(node, sizeof(struct LibResolution));
    }
}

static int P96GFX__OpenPrivateLibs(struct p96gfx_staticdata *csd)
{
    D(bug("[HiddP96Gfx] %s()\n", __func__);)
    if ((csd->cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY))) {
        if ((csd->cs_IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION))) {
            return TRUE;
        }
    }
    return FALSE;
}

static void P96GFX__ClosePrivateLibs(struct p96gfx_staticdata *csd)
{
    D(bug("[HiddP96Gfx] %s()\n", __func__);)
    CloseLibrary(csd->cs_IntuitionBase);
    CloseLibrary(csd->cs_UtilityBase);
}

static void P96DebugInfo(struct p96gfx_carddata *cid)
{
    UBYTE i;
    DRTG(bug("[HiddP96Gfx] %s: Name:'%s'\n", __func__,
        gl(cid->boardinfo + PSSO_BoardInfo_BoardName));)
    DRTG(bug("[HiddP96Gfx] %s: Reg:%08x IO:%08x\n", __func__,
        gl(cid->boardinfo + PSSO_BoardInfo_RegisterBase),
        gl(cid->boardinfo + PSSO_BoardInfo_MemoryIOBase));)
    DRTG(bug("[HiddP96Gfx] %s: BoardType:%d GCType:%d PCType:%d BPC:%d Flags:%08x\n", __func__,
        gl(cid->boardinfo + PSSO_BoardInfo_BoardType),
        gl(cid->boardinfo + PSSO_BoardInfo_GraphicsControllerType),
        gl(cid->boardinfo + PSSO_BoardInfo_PaletteChipType),
        gw(cid->boardinfo + PSSO_BoardInfo_BitsPerCannon),
        gl(cid->boardinfo + PSSO_BoardInfo_Flags));)
    for (i = 0; i < MAXMODES; i++) {
        DRTG(bug("[HiddP96Gfx] %s: %d: HV:%4d VV: %4d HR:%4d VR:%4d C:%d\n", __func__, i,
            gw(cid->boardinfo + PSSO_BoardInfo_MaxHorValue + i * 2),
            gw(cid->boardinfo + PSSO_BoardInfo_MaxVerValue + i * 2),
            gw(cid->boardinfo + PSSO_BoardInfo_MaxHorResolution + i * 2),
            gw(cid->boardinfo + PSSO_BoardInfo_MaxVerResolution + i * 2),
            gl(cid->boardinfo + PSSO_BoardInfo_PixelClockCount + i * 4));)
    }
}

struct p96gfx_carddata *P96GFX__AllocCID(struct p96gfx_staticdata *csd)
{
    struct p96gfx_carddata *cid;
    struct Interrupt *intr;

    D(bug("[HiddP96Gfx] %s()\n", __func__);)

    /* Allocate the Card Instance Data */
    cid = AllocMem(sizeof(struct p96gfx_carddata), MEMF_CLEAR | MEMF_PUBLIC);
    if (!cid) {
        return NULL;
    }

    cid->boardinfo = AllocVec(PSSO_BoardInfo_SizeOf + PSSO_BitMapExtra_Last + sizeof(struct ModeInfo), MEMF_CLEAR | MEMF_PUBLIC);
    if (!cid->boardinfo) {
        FreeMem(cid, sizeof(struct p96gfx_carddata));
        return NULL;
    }
    NEWLIST((struct List*)(cid->boardinfo + PSSO_BoardInfo_ResolutionsList));
    NEWLIST((struct List*)(cid->boardinfo + PSSO_BoardInfo_BitMapList));
    NEWLIST((struct List*)(cid->boardinfo + PSSO_BoardInfo_MemList));
    NEWLIST((struct List*)(cid->boardinfo + PSSO_BoardInfo_WaitQ));
    cid->rgbformat = (ULONG *)((IPTR)cid->boardinfo + PSSO_BoardInfo_RGBFormat);
    cid->bitmapextra = cid->boardinfo + PSSO_BoardInfo_SizeOf;
    cid->fakemodeinfo = (struct ModeInfo*)(cid->boardinfo + PSSO_BoardInfo_SizeOf + PSSO_BitMapExtra_Last);
    pl(cid->boardinfo + PSSO_BoardInfo_BitMapExtra, (ULONG)cid->bitmapextra);
    pl(cid->boardinfo + PSSO_BoardInfo_ExecBase, (ULONG)SysBase);
    pl(cid->boardinfo + PSSO_BoardInfo_UtilBase, (ULONG)csd->cs_UtilityBase);
    InitSemaphore((struct SignalSemaphore*)(cid->boardinfo + PSSO_BoardInfo_BoardLock));
    intr = (struct Interrupt*)(cid->boardinfo + PSSO_BoardInfo_HardInterrupt);
    intr->is_Code = (APTR)rtg_vblank;
    intr->is_Data         = cid->boardinfo;
    intr->is_Node.ln_Name = "RTG VBlank";
    intr->is_Node.ln_Pri  = 0;
    intr->is_Node.ln_Type = NT_INTERRUPT;
    intr = (struct Interrupt*)(cid->boardinfo + PSSO_BoardInfo_SoftInterrupt);
    intr->is_Code = (APTR)rtg_vblank;
    intr->is_Data         = cid->boardinfo;
    intr->is_Node.ln_Name = "RTG VBlank";
    intr->is_Node.ln_Pri  = 0;
    intr->is_Node.ln_Type = NT_INTERRUPT;

    return cid;
}

VOID P96GFX__FreeCID(struct p96gfx_staticdata *csd, struct p96gfx_carddata *cid)
{
    D(bug("[HiddP96Gfx] %s()\n", __func__);)
    if (cid->vmem)
        FreeMem(cid->vmem, sizeof(struct MemHeader));
    FreeVec(cid->boardinfo);
    FreeMem(cid, sizeof(struct p96gfx_carddata));
}

VOID P96GFX__FreeCardCIDList(struct p96gfx_staticdata *csd)
{
    D(bug("[HiddP96Gfx] %s()\n", __func__);)
}

static BOOL P96GFX__InitCard(struct p96gfx_staticdata *csd, struct Library *lib)
{
    struct p96gfx_carddata *cid;
    DRTG(bug("[HiddP96Gfx] %s: attempting to init '%s'\n", __func__, lib->lib_Node.ln_Name);)

    cid = P96GFX__AllocCID(csd);
    if (!cid)
        return FALSE;

    pl(cid->boardinfo + PSSO_BoardInfo_CardBase, (ULONG)lib);
    cid->CardBase = lib;
    P96GFXRTG__Init(cid->boardinfo);
    if (FindCard(cid)) {
        DRTG(bug("[HiddP96Gfx] %s: FindCard succeeded\n", __func__);)
        if (InitCard(cid)) {
            DRTG(bug("[HiddP96Gfx] %s: InitCard succeeded\n", __func__);)
            SetInterrupt(cid, FALSE);
            /* Without this, the card may not be in linear memory map mode. */
            SetMemoryMode(cid, RGBFB_CLUT);
            P96DebugInfo(cid);
            AddTail(&csd->foundCards, &cid->p96gfx_Node);
            P96GFX__PopulateResolutionsList(csd, cid);
            cid->hardwaresprite = gl(cid->boardinfo + PSSO_BoardInfo_Flags) & (1 << BIB_HARDWARESPRITE);
            cid->hiressprite = (gl(cid->boardinfo + PSSO_BoardInfo_Flags) & BIF_HIRESSPRITE) != 0;
            return TRUE;
        }
    }
    pl(cid->boardinfo + PSSO_BoardInfo_CardBase, 0);
    P96GFX__FreeCID(csd, cid);
    return FALSE;
}


BOOL P96GFX__Initialise(LIBBASETYPEPTR LIBBASE)
{
    struct p96gfx_staticdata *csd = &LIBBASE->csd;
    struct Library *GfxBase = csd->cs_GfxBase;
    struct p96gfx_carddata *cid;
    struct MemChunk *mc;
    struct Interrupt *intr;
    struct Node *node;
    UBYTE i;
    BOOL retval = FALSE;
    ULONG driverres;

    D(bug("[HiddP96Gfx] %s()\n", __func__);)

    if (!(SysBase->AttnFlags & AFF_68020))
        return retval;

    if (!P96GFX__OpenPrivateLibs(csd)) {
        P96GFX__ClosePrivateLibs(csd);
        return retval;
    }

    NEWLIST(&csd->foundCards);
    D(bug("[HiddP96Gfx] %s: cardlist @ 0x%p\n", __func__, &csd->foundCards);)

    /* find available p96 '.card' drivers .. */
    Forbid();
    ForeachNode(&SysBase->LibList.lh_Head, node) {
        struct Library *lib = (struct Library*)node;
        char *name = node->ln_Name;
        int len = strlen(name);
        if (len > 5 && !stricmp(name + len - 5, ".card")) {
            Permit();
            retval = P96GFX__InitCard(csd, lib);
            Forbid();
            DRTG(
              if (!retval)
                  bug("[HiddP96Gfx] %s: %s init failed\n", __func__, name);
             )
        }
    }
    Permit();

    /* if none where found create UAE p96 romvector entry if available */
    if (IsListEmpty(&csd->foundCards)) {
        cid = P96GFX__AllocCID(csd);
        if (cid)
        {
            /* New UAE Boot ROM p96romvector query */
            APTR *res = OpenResource("uae.resource");
            if (res) {
                cid->p96romvector = AROS_LVO_CALL1(APTR, AROS_LCA(UBYTE*, "uaelib_demux", A0), APTR, res, 1,);
            }
            if (!cid->p96romvector) {
                /* Old F00000 UAE Boot ROM p96romvector check */
                cid->p96romvector = (APTR)(0xf00000 + 0xff60);
                if ((gl(cid->p96romvector) & 0xff00ffff) != 0xa0004e75) {
                    cid->p96romvector = NULL;
                }
            }
            if (!cid->p96romvector) {
                D(bug("[HiddP96Gfx] %s: P96 boot ROM entry point not found. P96GFX not enabled.\n", __func__);)
                P96GFX__FreeCID(csd, cid);
                P96GFX__ClosePrivateLibs(csd);
                return retval;
            }
            if (!FindCard(cid)) {
                D(bug("[HiddP96Gfx] %s: P96 FindCard() returned false\n", __func__);)
                P96GFX__FreeCID(csd, cid);
                P96GFX__ClosePrivateLibs(csd);
                return retval;
            }
            D(bug("[HiddP96Gfx] %s: P96 FindCard done\n", __func__);)
            InitCard(cid);
            cid->hardwaresprite = gl(cid->boardinfo + PSSO_BoardInfo_Flags) & (1 << BIB_HARDWARESPRITE);
            cid->hiressprite = (gl(cid->boardinfo + PSSO_BoardInfo_Flags) & BIF_HIRESSPRITE) != 0;
            AddTail(&csd->foundCards, &cid->p96gfx_Node);
        }
    }

    /* if we have found suitable drivers that have registered resolutions, setup the cards memory */
    ForeachNode(&csd->foundCards, cid) {
        if (IsListEmpty((struct List*)(cid->boardinfo + PSSO_BoardInfo_ResolutionsList))) {
            continue;
        }
        retval = TRUE;
        for (i = 0; i < MAXMODES; i++) {
            cid->maxwidth[i] = gw(cid->boardinfo + PSSO_BoardInfo_MaxHorResolution + i * 2);
            cid->maxheight[i] = gw(cid->boardinfo + PSSO_BoardInfo_MaxVerResolution + i * 2);
        }

        D(bug("[HiddP96Gfx] %s: InitCard done\n", __func__);)

        DRTG(bug("[HiddP96Gfx] %s: hardware sprite: %d\n", __func__, cid->hardwaresprite);)

        cid->vram_start = (UBYTE*)gl(cid->boardinfo + PSSO_BoardInfo_MemoryBase);

#ifdef USE_VRAM_HACK
        /* REMOVE-ME: 4MB hack (used for easier debug of vram <-> ram swapping) !!!!*/
        cid->vram_size = 4 * 1024 * 1024; /* gl(cid->boardinfo + PSSO_BoardInfo_MemorySize); */
#else
        cid->vram_size = gl(cid->boardinfo + PSSO_BoardInfo_MemorySize);
#endif

        DRTG(bug("[HiddP96Gfx] %s: P96RTG VRAM found at %08x size %08x\n", __func__, cid->vram_start, cid->vram_size);)
        mc = (struct MemChunk*)cid->vram_start;
        cid->vmem = AllocVec(sizeof(struct MemHeader), MEMF_CLEAR | MEMF_PUBLIC);
        cid->vmem->mh_Node.ln_Type = NT_MEMORY;
        cid->vmem->mh_First = mc;
        cid->vmem->mh_Lower = (APTR)mc;
        cid->vmem->mh_Upper = (APTR)((ULONG)mc + cid->vram_size);
        cid->vmem->mh_Free = cid->vram_size;
        mc->mc_Next = NULL;
        mc->mc_Bytes = cid->vmem->mh_Free;
    }
    D(bug("[P96CRASH] 9 init-vram-done retval=%d\n", retval));
    if (!retval)
    {
        // We dont fail if atleast one card successfully built its resolution list.
        D(bug("[HiddP96Gfx] %s: Resolutionlist(s) empty, init failed.\n", __func__);)
        P96GFX__FreeCardCIDList(csd);
        P96GFX__ClosePrivateLibs(csd);
        return retval;
    }

    D(bug("[HiddP96Gfx] %s: initialising common data ..\n", __func__);)

    retval = FALSE;

    driverres = OOP_ObtainAttrBasesArray(&csd->hiddAttrBase, attrbase_ids);
    OOP_ObtainMethodBasesArray(&csd->hiddBitMapBase, methodbase_ids);

    if (driverres)
    {
        D(bug("[HiddP96Gfx] %s: failed to obtain attribute bases!\n", __func__);)
        P96GFX__FreeAttrBases(LIBBASE, csd);
        P96GFX__FreeCardCIDList(csd);
        P96GFX__ClosePrivateLibs(csd);
        return retval;
    }

    D(bug("[HiddP96Gfx] %s: registering found cards ...\n", __func__);)
    ForeachNode(&csd->foundCards, cid) {
        D(bug("[HiddP96Gfx] %s:   carddata @ 0x%p\n", __func__, cid);)

        /* fake an opener for every card instance */
        LIBBASE->library.lib_OpenCnt += 1;

        cid->p96gfxcd_Tags = AllocVec((sizeof(struct TagItem) << 1), MEMF_PUBLIC | MEMF_CLEAR);
        if (cid->p96gfxcd_Tags)
        {
            D(bug("[HiddP96Gfx] %s:       tags @ 0x%p\n", __func__, cid->p96gfxcd_Tags);)
            cid->p96gfxcd_Tags[0].ti_Tag = aHidd_P96Gfx_CardData;
            cid->p96gfxcd_Tags[0].ti_Data = (IPTR)cid;
            if (DD_OK == (driverres = AddDisplayDriver(csd->gfxclass, cid->p96gfxcd_Tags,
                                   DDRV_KeepBootMode, TRUE,
                                   DDRV_IDMask      , 0xF0000000,
                                   TAG_DONE)))
            {
                D(bug("[HiddP96Gfx] %s:   AddDisplayDriver succeeded\n", __func__);)
                retval = TRUE;
            }
            else
            {
                bug("[HiddP96Gfx] AddDisplayDriver failed - %08x\n", driverres);
            }
        }
    }
    DRTG(bug("[HiddP96Gfx] %s: P96GFX init done\n", __func__);)
    D(bug("[P96CRASH] 1 init-done retval=%d\n", retval));
    return retval;
}

static int P96GFX_LibExpunge(LIBBASETYPEPTR LIBBASE)
{
    struct p96gfx_staticdata *csd = &LIBBASE->csd;
    int retval = FALSE;

    D(bug("[HiddP96Gfx] %s()\n", __func__);)

    if (LIBBASE->library.lib_OpenCnt == 0)
    {
        P96GFX__FreeAttrBases(LIBBASE, csd);
        P96GFX__ClosePrivateLibs(csd);
        retval = TRUE;
    }
    return retval;
}

ADD2EXPUNGELIB(P96GFX_LibExpunge, 1)

#undef SysBase

AROS_INTH1(rtg_vblank, APTR, boardinfo)
{
    AROS_INTFUNC_INIT

    return 0;

    AROS_INTFUNC_EXIT
}
