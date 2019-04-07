/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: P96 rtg card Gfx Hidd wrapper.
    Lang: English.
*/

#include <exec/libraries.h>
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

#include LC_LIBDEFS_FILE

#include "p96gfx_intern.h"
#include "p96gfx_bitmap.h"
#include "p96gfx_rtg.h"

#define SDEBUG                  0
#define DEBUG                   0
#define DRTG(x)
#define DEXTRA(x)
#include <aros/debug.h>

#define SIZE_RESLIST            5
#define SIZE_PFLIST             19
#define SIZE_MODELIST           (5 + RGBFB_MaxFormats)

#define SPRITE_PEN_COUNT        16
/* REMOVE-ME: 4MB Hack (used for easier debug of vram <-> ram swapping) !!!!*/
/*#define USE_VRAM_HACK*/

HIDDT_ModeID *P96GFXCl__Hidd_Gfx__QueryModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_QueryModeIDs *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
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
                minwidth = (ULONG)tag->ti_Tag;
                break;

            case tHidd_GfxMode_MaxWidth:
                maxwidth = (ULONG)tag->ti_Tag;
                break;

            case tHidd_GfxMode_MinHeight:
                minheight = (ULONG)tag->ti_Tag;
                break;

            case tHidd_GfxMode_MaxHeight:
                maxheight = (ULONG)tag->ti_Tag;
                break;
                
            case tHidd_GfxMode_PixFmts:
                pf = (OOP_Object**)tag->ti_Tag;
                break;
        }
    }
    DB2(bug("[P96Gfx] %s: (%dx%d)-(%dx%d) %p\n", __func__, minwidth, minheight, maxwidth, maxheight, pf);)
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
            if (!pfp)
                cnt++;
        }
    }
    modeids = AllocVec((cnt + 1) * sizeof(HIDDT_ModeID), MEMF_PUBLIC);
    if (!modeids)
        return NULL;
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
                DB2(bug("[P96Gfx] %s:   #%d  %08x\n", __func__, cnt, node->modeid);)
                modeids[cnt++] = node->modeid;
            }
        }
    }
    modeids[cnt] = vHidd_ModeID_Invalid;
    return modeids;
}

VOID P96GFXCl__Hidd_Gfx__ReleaseModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ReleaseModeIDs *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    if (cid->superforward)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    else
        FreeVec(msg->modeIDs);
}

HIDDT_ModeID P96GFXCl__Hidd_Gfx__NextModeID(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NextModeID *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
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

BOOL P96GFXCl__Hidd_Gfx__GetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetMode *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
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
    return FALSE;
}

struct RTGFormat
{
    UWORD rgbformat;
    IPTR pixfmt;
    ULONG rm, gm, bm, am;
    UWORD rs, gs, bs, as;
    BOOL endianswap;
};

static const struct RTGFormat formats[] =
{
    { RGBFB_CLUT,	    vHidd_StdPixFmt_LUT8,       0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,  8, 16, 24,  0, FALSE },

    { RGBFB_B8G8R8A8,	vHidd_StdPixFmt_BGRA32,     0x0000ff00, 0x00ff0000, 0xff000000, 0x000000ff, 16,  8,  0, 24, FALSE },
    { RGBFB_R8G8B8A8,	vHidd_StdPixFmt_RGBA32,     0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff,  0,  8, 16, 24, FALSE },
    { RGBFB_A8B8G8R8,	vHidd_StdPixFmt_ABGR32,     0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, 24, 16,  8,  0, FALSE },
    { RGBFB_A8R8G8B8,	vHidd_StdPixFmt_ARGB32,     0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000,  8, 16, 24,  0, FALSE },

    { RGBFB_B8G8R8,	    vHidd_StdPixFmt_BGR24,      0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000, 24, 16,  8,  0, FALSE },
    { RGBFB_R8G8B8,	    vHidd_StdPixFmt_RGB24,      0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,  8, 16, 24,  0, FALSE },

    { RGBFB_R5G5B5,	    vHidd_StdPixFmt_RGB15,      0x00007c00, 0x000003e0, 0x0000001f, 0x00000000, 17, 22, 27,  0, FALSE },
    { RGBFB_R5G6B5,	    vHidd_StdPixFmt_RGB16,      0x0000f800, 0x000007e0, 0x0000001f, 0x00000000, 16, 21, 27,  0, FALSE },

    { RGBFB_R5G5B5PC,	vHidd_StdPixFmt_RGB15_LE,   0x00007c00, 0x000003e0, 0x0000001f, 0x00000000, 17, 22, 27,  0, TRUE },
    { RGBFB_R5G6B5PC,	vHidd_StdPixFmt_RGB16_LE,   0x0000f800, 0x000007e0, 0x0000001f, 0x00000000, 16, 21, 27,  0, TRUE },

    { RGBFB_B5G5R5PC,	vHidd_StdPixFmt_BGR15_LE,   0x0000003e, 0x000007c0, 0x0000f800, 0x00000000, 26, 21, 16,  0, TRUE },
    { RGBFB_B5G6R5PC,	vHidd_StdPixFmt_BGR16_LE,   0x0000001f, 0x000007e0, 0x0000f800, 0x00000000, 27, 21, 16,  0, TRUE },

    { 0 }
};

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

OOP_Object *P96GFXCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct LibResolution *r;
    struct TagItem *reslist, *restags, *pflist, *modetags;
    struct TagItem mytags[] = {
        { aHidd_Gfx_ModeTags,	        0                   },
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
    mytags[2].ti_Data = (IPTR)cid->p96gfx_HWName;

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

    i = 0;
    ForeachNode(cid->boardinfo + PSSO_BoardInfo_ResolutionsList, r) {
        reslist[i * SIZE_RESLIST + 0].ti_Tag = aHidd_Sync_HDisp;
        reslist[i * SIZE_RESLIST + 0].ti_Data = r->Width;
        reslist[i * SIZE_RESLIST + 1].ti_Tag = aHidd_Sync_VDisp;
        reslist[i * SIZE_RESLIST + 1].ti_Data = r->Height;
        reslist[i * SIZE_RESLIST + 2].ti_Tag = aHidd_Sync_Description;
        reslist[i * SIZE_RESLIST + 2].ti_Data = (IPTR)cid->p96gfx_HWResTmplt;
        reslist[i * SIZE_RESLIST + 3].ti_Tag = aHidd_Sync_PixelClock;
        reslist[i * SIZE_RESLIST + 3].ti_Data = r->Modes[CHUNKY]->PixelClock;
        reslist[i * SIZE_RESLIST + 4].ti_Tag = TAG_DONE;
        reslist[i * SIZE_RESLIST + 4].ti_Data = 0;
        D(bug("[P96Gfx] %s:     %08x %d*%d\n", __func__, r, r->Width, r->Height);)
        restags[i].ti_Tag = aHidd_Gfx_SyncTags;
        restags[i].ti_Data = (IPTR)&reslist[i * SIZE_RESLIST];
        i++;
    }
    restags[i].ti_Tag = TAG_DONE;
    restags[i].ti_Data = 0;

    gotmodes = 0;
    k = 0;
    j = 0;
    for (i = 0; rgbtypelist[i]; i++) {
        UBYTE rgbtype = rgbtypelist[i];
        WORD depth = P96GFXRTG__GetDepth(1 << rgbtype);
        if (!((1 << rgbtype) & RGBFB_SUPPORTMASK) || depth == 0 || !((1 << rgbtype) & supportedformats)) {
            pflist[j].ti_Tag = TAG_DONE;
            pflist[j].ti_Data = 0;
            j++;
            continue;
        }
        for (l = 0; formats[l].rgbformat; l++) {
            if (formats[l].rgbformat == rgbtype)
                break;
        }
        if (formats[l].rgbformat == 0) {
            pflist[j].ti_Tag = TAG_DONE;
            pflist[j].ti_Data = 0;
            j++;
            continue;
        }
        D(bug("[P96Gfx] %s: RTGFORMAT=%d found. Depth=%d\n", __func__, rgbtype, depth);)
        
        if (gotmodes & (1 << (depth / 8))) {
            D(bug("[P96Gfx] %s:   -> skipped\n", __func__);)
            pflist[j].ti_Tag = TAG_DONE;
            pflist[j].ti_Data = 0;
            j++;
            continue;
        }

        gotmodes |= 1 << (depth / 8);

        modetags[k].ti_Tag = aHidd_Gfx_PixFmtTags;
        modetags[k].ti_Data = (IPTR)&pflist[j];
        k++;

        pflist[j].ti_Tag = aHidd_PixFmt_RedShift;
        pflist[j].ti_Data = formats[l].rs;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_GreenShift;
        pflist[j].ti_Data = formats[l].gs;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_BlueShift;
        pflist[j].ti_Data = formats[l].bs;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_AlphaShift;
        pflist[j].ti_Data = formats[l].as;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_RedMask;
        pflist[j].ti_Data = formats[l].rm;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_GreenMask;
        pflist[j].ti_Data = formats[l].gm;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_BlueMask;
        pflist[j].ti_Data = formats[l].bm;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_AlphaMask;
        pflist[j].ti_Data = formats[l].am;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_CLUTMask;
        pflist[j].ti_Data = 0x000000FF;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_CLUTShift;
        pflist[j].ti_Data = 0;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_ColorModel;
        pflist[j].ti_Data = depth <= 8 ? vHidd_ColorModel_Palette : vHidd_ColorModel_TrueColor;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_Depth;
        pflist[j].ti_Data = depth;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_BytesPerPixel;
        pflist[j].ti_Data = (depth + 7) / 8;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_BitsPerPixel;
        pflist[j].ti_Data = depth;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_StdPixFmt;
        pflist[j].ti_Data = formats[l].pixfmt;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_BitMapType;
        pflist[j].ti_Data = vHidd_BitMapType_Chunky;
        j++;
        pflist[j].ti_Tag = aHidd_PixFmt_SwapPixelBytes;
        pflist[j].ti_Data = formats[l].endianswap;
        j++;
        pflist[j].ti_Tag = TAG_DONE;
        pflist[j].ti_Data = 0;
        j++;
    }

    modetags[k].ti_Tag = aHidd_Sync_HMin;
    modetags[k].ti_Data = 112;
    k++;
    modetags[k].ti_Tag = aHidd_Sync_VMin;
    modetags[k].ti_Data = 112;
    k++;
    modetags[k].ti_Tag = aHidd_Sync_HMax;
    modetags[k].ti_Data = 16384;
    k++;
    modetags[k].ti_Tag = aHidd_Sync_VMax;
    modetags[k].ti_Data = 16384;
    k++;
    modetags[k].ti_Tag = TAG_MORE;
    modetags[k].ti_Data = (IPTR)restags;

    mytags[0].ti_Data = (IPTR)modetags;

    EnterFunc(bug("[P96Gfx] %s: tags @ %x\n", __func__, mytags);)

    mymsg.mID	= msg->mID;
    mymsg.attrList = mytags;
    msg = &mymsg;

    /* Register gfxmodes */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL != o)
    {
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

        cid->initialized = 1;

        cid->superforward = TRUE;
        midp = HIDD_Gfx_QueryModeIDs(o, NULL);
        for (i = 0; midp[i] != vHidd_ModeID_Invalid; i++) {
            OOP_Object *sync, *pf;
            HIDDT_ModeID mid = midp[i];
            IPTR dwidth, dheight;
            struct RTGMode *node1, *node2;
            ULONG modeid, rtgmodeid, p96mode;

            if (!HIDD_Gfx_GetMode(o, mid, &sync, &pf))
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

            p96mode = P96GFXRTG__GetFormat(csd, cid, pf);
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
        HIDD_Gfx_ReleaseModeIDs(o, midp);
        cid->superforward = FALSE;
    }
    
    FreeVec(restags);
    FreeVec(reslist);
    FreeVec(pflist);
    FreeVec(modetags);

    ReturnPtr("[P96Gfx]:New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
OOP_Object *P96GFXCl__Hidd_Gfx__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    D(bug("[P96Gfx] %s()\n", __func__));

    if (msg->cl == CSD(cl)->basebm)
    {
        struct p96gfx_staticdata        *csd = CSD(cl);
        HIDDT_ModeID		        modeid;
        struct pHidd_Gfx_CreateObject   p;
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
            case aoHidd_Gfx_HWSpriteTypes:
                *msg->storage = cid->hardwaresprite ? vHidd_SpriteType_3Plus1 : 0;
                return;
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
#if 0
ULONG P96GFXCl__Hidd_Gfx__MakeViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_MakeViewPort *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx] %s()\n", __func__));

    csd->vpe = NULL;
    if (!msg)
        return MVP_OK;
    bug("[P96Gfx] %s: %p\n", __func__, msg->Data->vpe);
    csd->vpe = msg->Data->vpe;
    return MVP_OK;
}

void P96GFXCl__Hidd_Gfx__CleanViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CleanViewPort *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx] %s()\n", __func__));

    csd->vpe = NULL;
}
#endif

static void P96GFXCl__DoShow(OOP_Class *cl, OOP_Object *o, OOP_Object *bm, struct ViewPort *vp, BOOL offonly)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    struct IntuitionBase *ib = (struct IntuitionBase*)csd->cs_IntuitionBase;
    struct ViewPort *vpi = NULL;

    D(bug("[P96Gfx] %s()\n", __func__));

    if (ib->FirstScreen)
        vpi = &ib->FirstScreen->ViewPort;

    D(bug("[P96Gfx] %s: b=%p vp=%p vpi=%p acb=%p acbd=%p\n", __func__, bm, vp, vpi, cid->acb, cid->acbdata);)

    if (bm && vpi == vp) {
        /* we are topmost screen -> show our display */
        IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};

        if (offonly)
            return;

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
        SetDisplay(cid, FALSE);
        SetSwitch(cid, FALSE);

        UNLOCK_HW
    }
}

OOP_Object *P96GFXCl__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;

    D(bug("[P96Gfx] %s()\n", __func__));

    P96GFXCl__DoShow(cl, o, msg->bitMap, cid->viewport, FALSE);
    return msg->bitMap;
}

ULONG P96GFXCl__Hidd_Gfx__PrepareViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
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
    P96GFXCl__DoShow(cl, o, bm, vp, FALSE);

    D(bug("[P96Gfx] %s: viewport=%p\n", __func__, cid->viewport);)
    return MCOP_OK;
}

ULONG P96GFXCl__Hidd_Gfx__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
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
    P96GFXCl__DoShow(cl, o, bm, vp, FALSE);
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

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW
    
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
            switch(sdata->bytesperpixel)
            {
                case 1:
                    HIDD_BM_CopyMemBox8(msg->dest,
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
                    break;

                case 2:
                    HIDD_BM_CopyMemBox16(msg->dest,
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
                    break;

                case 3:
                    HIDD_BM_CopyMemBox24(msg->dest,
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
                    break;

                case 4:
                    HIDD_BM_CopyMemBox32(msg->dest,
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
                    break;

            } /* switch(data->bytesperpix) */
            
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

        P96GFXRTG__MakeRenderInfo(csd, cid, &risrc, sdata);
        P96GFXRTG__MakeRenderInfo(csd, cid, &ridst, ddata);
        
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

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

static UBYTE *P96GFXCl__PrepareSprite(OOP_Class *cl, OOP_Object *o, ULONG store, ULONG size, ULONG width, ULONG height, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
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

BOOL P96GFXCl__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    OOP_Object *cm = NULL;
    OOP_Object *bmPFObj = NULL;
    HIDDT_PixelFormat *bmPF = NULL;
    IPTR pf, bmcmod, width, height;
    WORD x, y, hiressprite, i;
    ULONG flags;

    D(bug("[P96Gfx] %s()\n", __func__);)

    if (!(gl(cid->boardinfo + PSSO_BoardInfo_Flags ) & BIF_HARDWARESPRITE))
        return FALSE;

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (IPTR*)&cm);
    OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (IPTR*)&bmPFObj);
    OOP_GetAttr(bmPFObj, aHidd_PixFmt_ColorModel, &bmcmod);
    if (bmcmod == vHidd_ColorModel_TrueColor)
    {
        OOP_GetAttr(bmPFObj, aHidd_PixFmt_StdPixFmt, (IPTR*)&pf);
        bmPF = (HIDDT_PixelFormat *)HIDD_Gfx_GetPixFmt(o, pf);
    }

    LOCK_HW
    if (cm) {
        for (i = 0; i < 3; i++) {
            HIDDT_Color c;
            HIDD_CM_GetColor(cm, i + 1, &c);
            HIDD_P96GFX_SetCursorPen(o, i, c);
        }
    }
    else
    {
        // TODO: Calculate histogram of image and choose best colors
    }

    flags = gl(cid->boardinfo + PSSO_BoardInfo_Flags);
    flags &= ~BIF_HIRESSPRITE;
    hiressprite = 1;
    if (width > 16) {
        flags |= BIF_HIRESSPRITE;
        hiressprite = 2;
    }
    pl(cid->boardinfo + PSSO_BoardInfo_Flags, flags);

    pb(cid->boardinfo + PSSO_BoardInfo_MouseWidth, width / hiressprite);
    pb(cid->boardinfo + PSSO_BoardInfo_MouseHeight, height);

    Forbid();
    DB2(bug("[P96Gfx] %s: filling planar buffer ...\n", __func__);)
    {
        UWORD *pw;

        pw = (UWORD *)P96GFXCl__PrepareSprite(cl, o, PSSO_BoardInfo_MouseImage, 4 + 4 + ((width + 15) & ~15) / 8 * height * 2, width, height, msg);
        if (!pw)
        {
            Permit();
            UNLOCK_HW
            return FALSE;
        }
        pw += 2 * hiressprite;
        for(y = 0; y < height; y++) {
            UWORD pix1 = 0, pix2 = 0, xcnt = 0;
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
                pix1 <<= 1;
                pix2 <<= 1;
                pix1 |= (c & 1) ? 1 : 0;
                pix2 |= (c & 2) ? 1 : 0;
                xcnt++;
                if (xcnt == 15) {
                    xcnt = 0;
                    pw[x / 16] = pix1;
                    pw[width / 16 + x / 16] = pix2;
                }
            }
            pw += (width / 16) * 2;
        }
    }

#if (0)
    DB2(bug("[P96Gfx] %s: filling clut buffer ...\n", __func__);)
    {
        UBYTE *pb;

        pb = P96GFXCl__PrepareSprite(cl, o, PSSO_BoardInfo_MouseChunky, 4 + 4 + ((width + 15) & ~15) * height, width, height, msg);
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
    SetSpriteImage(cid);

    UNLOCK_HW
    DB2(bug("[P96Gfx] %s: hw sprite loaded\n", __func__));

    return TRUE;
}
                             
BOOL P96GFXCl__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx] %s()\n", __func__));

    LOCK_HW
    pw(cid->boardinfo + PSSO_BoardInfo_MouseX, msg->x + (BYTE)cid->boardinfo[PSSO_BoardInfo_MouseXOffset]);
    pw(cid->boardinfo + PSSO_BoardInfo_MouseY, msg->y + (BYTE)cid->boardinfo[PSSO_BoardInfo_MouseYOffset]);
    SetSpritePosition(cid);
    UNLOCK_HW

    return TRUE;
}

VOID P96GFXCl__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx] %s()\n", __func__));

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

BOOL P96GFXCl__Hidd_Gfx__CheckMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CheckMode *msg)
{
    struct P96GfxData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->cardData;
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

static void P96GFX__FreeAttrBases(LIBBASETYPEPTR LIBBASE, struct p96gfx_staticdata *csd)
{
    OOP_ReleaseAttrBase(IID_Hidd);
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    OOP_ReleaseAttrBase(IID_Hidd_BitMap_P96);
    OOP_ReleaseAttrBase(IID_Hidd_GC);
    OOP_ReleaseAttrBase(IID_Hidd_Sync);
    OOP_ReleaseAttrBase(IID_Hidd_Gfx);
    OOP_ReleaseAttrBase(IID_Hidd_P96Gfx);
    OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    OOP_ReleaseAttrBase(IID_Hidd_ColorMap);
}

AROS_INTP(rtg_vblank);

struct P96RTGmode
{
    UWORD w, h, id;
    ULONG clock;
    UWORD htotal, vtotal;
    UWORD hborder, vborder;
    UWORD hpos, vpos;
    UWORD hsync, vsync;
    UBYTE flags;
};

static const struct P96RTGmode rtgmodes[] =
{
    {  320, 240, 1,  13020000,  408, 262, 0, 0, 32,  5,  24,  1, GMF_HPOLARITY | GMF_VPOLARITY | GMF_DOUBLESCAN },
    {  640, 480, 3,  25060000,  800, 525, 0, 0, 40, 11,  88,  2, GMF_HPOLARITY | GMF_VPOLARITY },
    {  800, 600, 4,  32010000, 1024, 625, 0, 0, 56,  1,  40,  2, 0 },
    { 1024, 768, 5,  64430000, 1336, 800, 0, 0, 48,  4, 112,  5, GMF_HPOLARITY | GMF_VPOLARITY },
    { 1152, 900, 6,  85010000, 1496, 973, 0, 0, 48,  5,  40,  4, GMF_HPOLARITY | GMF_VPOLARITY },
    { 1440, 900, 7, 106360000, 1904, 932, 0, 0, 80,  0,   0, 10, GMF_HPOLARITY | GMF_VPOLARITY },
    { 0 }
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
        mi->HorBlankSize = mode->hborder;
        mi->VerBlankSize = mode->vborder;
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
    for (cnt = 0; rtgmodes[cnt].clock; cnt++) {
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

    /* if none where found create the p96 romvector entry if available */
    if (IsListEmpty(&csd->foundCards)) {
        cid = P96GFX__AllocCID(csd);
        if (cid)
        {
            cid->p96romvector = (APTR)(0xf00000 + 0xff60);
            if ((gl(cid->p96romvector) & 0xff00ffff) != 0xa0004e75) {
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
            cid->hardwaresprite = (gl(cid->boardinfo + PSSO_BoardInfo_Flags) & (1 << BIB_HARDWARESPRITE)) && SetSprite(cid, FALSE);
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

    __IHidd             = OOP_ObtainAttrBase(IID_Hidd);
    __IHidd_BitMap      = OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_BitMap_P96  = OOP_ObtainAttrBase(IID_Hidd_BitMap_P96);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_P96Gfx      = OOP_ObtainAttrBase(IID_Hidd_P96Gfx);
    __IHidd_PixFmt	= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_ColorMap 	= OOP_ObtainAttrBase(IID_Hidd_ColorMap);

    HiddBitMapBase	= OOP_GetMethodID(IID_Hidd_BitMap, 0);
    HiddColorMapBase	= OOP_GetMethodID(IID_Hidd_ColorMap, 0);
    HiddGfxBase		= OOP_GetMethodID(IID_Hidd_Gfx, 0);
    HiddP96GfxBase      = OOP_GetMethodID(IID_Hidd_P96Gfx, 0);
    
    if (!__IHidd || !__IHidd_BitMap || !__IHidd_BitMap_P96 || !__IHidd_GC ||
        !__IHidd_Sync || !__IHidd_Gfx || !__IHidd_P96Gfx || !__IHidd_PixFmt ||
        !__IHidd_ColorMap)
    {
        D(bug("[HiddP96Gfx] %s: failed to obtain attribute bases!\n", __func__);)
        P96GFX__FreeAttrBases(LIBBASE, csd);
        P96GFX__FreeCardCIDList(csd);
        P96GFX__ClosePrivateLibs(csd);
        return retval;
    }

    D(bug("[HiddP96Gfx] %s: registering found cards ...\n", __func__);)
    ULONG driverres;
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
