/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English.
*/

#include <exec/libraries.h>
#include <exec/rawfmt.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <graphics/displayinfo.h>
#include <aros/libcall.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include <hidd/hidd.h>

#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

#include "chipset.h"
#include "blitter.h"

#include <aros/debug.h>

#define SPECIALMODES 3
#define NATIVEMODES (3 * 4 * SPECIALMODES)
static const UWORD widthtable[] = { 320, 640, 1280, 0 };
static const UWORD heighttable[] = { 200, 256, 400, 512, 0 };
static const ULONG specialmask_aga[] = { 0, EXTRAHALFBRITE_KEY, HAM_KEY, 0xffffffff };

#define SPECIAL_MODE_MASK (EXTRAHALFBRITE_KEY | HAM_KEY)

VOID AmigaVideoCl__Hidd_Gfx__NominalDimensions(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NominalDimensions *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct GfxBase *GfxBase = (struct GfxBase *)csd->cs_GfxBase;

    DB2(bug("[AmigaVideo] %s()\n", __func__));

    if (msg->width)
        *(msg->width) = GfxBase->NormalDisplayColumns;
    if (msg->height)
        *(msg->height) = GfxBase->NormalDisplayRows;
    if (msg->depth)
        *(msg->depth) = 1;
}

ULONG AmigaVideoCl__Hidd_Gfx__ModeProperties(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ModeProperties *msg)
{
    ULONG flags = 0;
    ULONG modeid = msg->modeID;

    flags = DIPF_IS_SPRITES | DIPF_IS_DRAGGABLE |
        DIPF_IS_SPRITES_ATT | DIPF_IS_SPRITES_CHNG_BASE | DIPF_IS_SPRITES_CHNG_PRI |
        DIPF_IS_DBUFFER | DIPF_IS_BEAMSYNC | DIPF_IS_GENLOCK;
    msg->props->NumHWSprites = 8;
    if ((modeid & MONITOR_ID_MASK) == PAL_MONITOR_ID)
        flags |= DIPF_IS_PAL;
    if (modeid & LORESLACE_KEY)
        flags |= DIPF_IS_LACE;
    if (modeid & HAM_KEY) {
        flags |= DIPF_IS_HAM;
        if (modeid & SUPER_KEY)
            flags |= DIPF_IS_AA;
    }
    if (modeid & EXTRAHALFBRITE_KEY) {
        flags |= DIPF_IS_EXTRAHALFBRITE;
        if (modeid & SUPER_KEY)
            flags |= DIPF_IS_AA;
    }
    if ((modeid & SUPER_KEY) == SUPER_KEY && !(flags & DIPF_IS_AA))
        flags |= DIPF_IS_ECS;
    if (!(modeid & SPECIAL_MODE_MASK))
        flags |= DIPF_IS_WB;
    msg->props->DisplayInfoFlags = flags;
    msg->props->CompositionFlags = COMPF_ABOVE | COMPF_BELOW; // | COMPF_LEFT | COMPF_RIGHT;
    DB2(bug("ModeProp %08x = %08x\n", modeid, flags));
    return sizeof(struct HIDD_ModeProperties);
}

HIDDT_ModeID *AmigaVideoCl__Hidd_Gfx__QueryModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_QueryModeIDs *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct NativeChipsetMode *node;
    struct TagItem *tag, *tstate;
    ULONG minwidth = 0, maxwidth = 0xFFFFFFFF;
    ULONG minheight = 0, maxheight = 0xFFFFFFFF;
    HIDDT_ModeID *modeids;
    struct Library *UtilityBase = csd->cs_UtilityBase;
    WORD cnt;

   if (csd->superforward)
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
                /* all chipset modes have same pixelformat */
                break;

        }
    }
    DB2(bug("QueryModeIDs (%dx%d)-(%dx%d)\n", minwidth, minheight, maxwidth, maxheight));
    cnt = 0;
    ForeachNode(&csd->nativemodelist, node) {
        if (node->width >= minwidth && node->width <= maxwidth && node->height >= minheight && node->height <= maxheight) {
            cnt++;
        }
    }
    modeids = AllocVec((cnt + 1) * sizeof(HIDDT_ModeID), MEMF_PUBLIC);
    if (!modeids)
        return NULL;
    cnt = 0;
    ForeachNode(&csd->nativemodelist, node) {
        if (node->width >= minwidth && node->width <= maxwidth && node->height >= minheight && node->height <= maxheight) {
            DB2(bug("%d: %08x\n", cnt, node->modeid));
            modeids[cnt++] = node->modeid;
        }
    }
    modeids[cnt] = vHidd_ModeID_Invalid;
    return modeids;
}

VOID AmigaVideoCl__Hidd_Gfx__ReleaseModeIDs(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ReleaseModeIDs *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    if (csd->superforward)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    else
        FreeVec(msg->modeIDs);
}

HIDDT_ModeID AmigaVideoCl__Hidd_Gfx__NextModeID(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NextModeID *msg)
{
        struct amigavideo_staticdata *csd = CSD(cl);
        struct NativeChipsetMode *node = NULL;
        HIDDT_ModeID mid = vHidd_ModeID_Invalid;

        DB2(bug("NextModeID %08x\n", msg->modeID));
        if (msg->modeID != vHidd_ModeID_Invalid) {
                ForeachNode(&csd->nativemodelist, node) {
                        if (node->modeid == msg->modeID) {
                                node = (struct NativeChipsetMode*)node->node.ln_Succ;
                                break;
                        }
                }
        }
        if (!node)
                node = (struct NativeChipsetMode*)csd->nativemodelist.lh_Head;
        if (node->node.ln_Succ) {
                mid = node->modeid;
                *msg->syncPtr = node->sync;
                *msg->pixFmtPtr = node->pf;
        }
        DB2(bug("=%08x %p %p\n", mid, *msg->syncPtr, *msg->pixFmtPtr));
        return mid;
}

BOOL AmigaVideoCl__Hidd_Gfx__GetMode(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_GetMode *msg)
{
        struct amigavideo_staticdata *csd = CSD(cl);
        struct NativeChipsetMode *node;

        if (csd->superforward)
                return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

        DB2(bug("GetMode %08x\n", msg->modeID));
        ForeachNode(&csd->nativemodelist, node) {
                if (node->modeid == msg->modeID) {
                        *msg->syncPtr = node->sync;
                        *msg->pixFmtPtr = node->pf;
                        DB2(bug("= %p %p %dx%dx%d %d\n", node->sync, node->pf, node->width, node->height, node->depth, node->special));
                        return TRUE;
                }
        }
        DB2(bug("= FAIL\n"));
        return FALSE;
}

#define ADDTAG(tag,data) { *tagptr++ = tag; *tagptr++ = data; }


static void makemodename(ULONG modeid, UBYTE *bufptr)
{
    BOOL special = FALSE;
    
    special = (modeid & SPECIAL_MODE_MASK) != 0;
    bufptr[0] = 0;
    if ((modeid & MONITOR_ID_MASK) == PAL_MONITOR_ID)
        strcat(bufptr, "PAL");
    else if ((modeid & MONITOR_ID_MASK) == NTSC_MONITOR_ID)
        strcat (bufptr, "NTSC");
    strcat(bufptr, ":");
    if ((modeid & (HIRES_KEY | SUPER_KEY)) == LORES_KEY)
        strcat(bufptr, special ? "LowRes" : "Low Res");
    else if ((modeid & (HIRES_KEY | SUPER_KEY)) == HIRES_KEY)
        strcat(bufptr, special ? "HighRes" : "High Res");
    else
        strcat(bufptr, special ? "SuperHighRes" : "Super-High Res");
    if (modeid & HAM_KEY)
        strcat(bufptr, " HAM");
    if (modeid & EXTRAHALFBRITE_KEY)
        strcat(bufptr, " EHB");
    if ((modeid & LORESDPF2_KEY) == LORESDPF_KEY)
        strcat(bufptr, " DualPF");
    if ((modeid & LORESDPF2_KEY) == LORESDPF2_KEY)
        strcat(bufptr, " DualPF2");
    if (modeid & LORESLACE_KEY)
        strcat(bufptr, special ? " Interlace" : " Laced");
    DB2(bug("%08x '%s'\n", modeid, bufptr));
}

static struct NativeChipsetMode *addmodeid(struct amigavideo_staticdata *csd, ULONG modeid, WORD w, WORD h, WORD d, UBYTE special)
{
    struct NativeChipsetMode *m;

    m = AllocMem(sizeof(struct NativeChipsetMode), MEMF_CLEAR | MEMF_PUBLIC);
    DB2(bug("%p %08x %dx%dx%d %d\n", m, modeid, w, h, d, special));
    m->width = w;
    m->height = h;
    m->depth = d;
    m->special = special;
    m->modeid = modeid;
    AddTail(&csd->nativemodelist, &m->node);
    return m;
}

/* this is SOOO HORRIBLE, do not even attempt to understand it.. */

OOP_Object *AmigaVideoCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
        struct amigavideo_staticdata *csd = CSD(cl);
        struct Library *OOPBase = csd->cs_OOPBase;
        struct TagItem mytags[] =
        {
            { aHidd_Gfx_ModeTags    , (IPTR)NULL   },
            { aHidd_Name            , (IPTR)"amigavideo.hidd"     },
            { aHidd_HardwareName    , (IPTR)"Amiga Native Display Hardware"   },
            { aHidd_ProducerName    , (IPTR)"Commodore International"  },
            { TAG_MORE              , (IPTR)msg->attrList       }
        };
        struct pRoot_New mymsg;
        ULONG allocsize = 3000, allocsizebuf = 1000;
        WORD x, y, cnt, i, j;

        UBYTE *buf, *bufptr;
        ULONG *tags, *tagptr;
        ULONG *modetags[NATIVEMODES], modeids[NATIVEMODES];
        ULONG *pftags_aga[SPECIALMODES];
        ULONG *pftags_ecs_shres = NULL, *pftags_ecs_hires, *pftags_ecs_lores, *pftags_ecs_6;
        ULONG *mode_tags_aga;
        ULONG *mode_tags_ecs;

        if (csd->initialized)
                return NULL;

        NewList(&csd->nativemodelist);
        tags = tagptr = AllocVec(allocsize, MEMF_PUBLIC | MEMF_REVERSE);
        buf = bufptr = AllocVec(allocsizebuf, MEMF_PUBLIC | MEMF_REVERSE);

        cnt = 0;
        for (y = 0; heighttable[y]; y++) {
                WORD h = heighttable[y];
                for (x = 0; widthtable[x]; x++) {
                        WORD w = widthtable[x];
                        WORD d, res;
                        ULONG modeid;
                        
                        modeid = 0;
                        if (w == 1280) {
                                res = 2;
                                modeid |= SUPER_KEY;
                                if (!csd->aga && !csd->ecs_denise)
                                        continue;
                                d = csd->aga ? 8 : 2;
                        }
                        else if (w == 640) {
                                res = 1;
                                modeid |= HIRES_KEY;
                                d = csd->aga ? 8 : 4;
                        } else {
                                res = 0;
                                d = csd->aga ? 8 : 5;
                        }
                        if (h >= 400)
                                modeid |= LORESLACE_KEY;
                        if (h == 200 || h == 400)
                                modeid |= NTSC_MONITOR_ID;
                        else
                                modeid |= PAL_MONITOR_ID;		

                        for (i = 0; i < SPECIALMODES; i++) {
                                ULONG mid = modeid;
                                UWORD d2 = d;
                                if (i == 1) {
                                        if (!csd->aga && (modeid & SUPER_KEY))
                                                continue;
                                        mid |= EXTRAHALFBRITE_KEY;
                                        d2 = 6;
                                } else if (i == 2) {
                                        if (!csd->aga && (modeid & SUPER_KEY))
                                                continue;
                                        mid |= HAM_KEY;
                                        d2 = csd->aga ? 8 : 6;
                                }
                                
                                addmodeid(csd, mid, w, h, d2, csd->aga ? i : (i == 0 ? res : i - 1 + 3));
                                modetags[cnt] = tagptr;
                                modeids[cnt++] = mid;

                                ADDTAG(aHidd_Sync_HDisp, w);
                                ADDTAG(aHidd_Sync_VDisp, h);
                                ADDTAG(aHidd_Sync_Flags, h >= 400 ? vHidd_Sync_Interlaced : 0);
                                ADDTAG(aHidd_Sync_PixelClock, 1000000000 / (280 >> res));
                                ADDTAG(TAG_DONE, 0);
                        }
                        
                }
        }

        if (csd->aga) {

                pftags_aga[0] = tagptr;
                ADDTAG(aHidd_PixFmt_RedShift,		 8);
                ADDTAG(aHidd_PixFmt_GreenShift, 	16);
                ADDTAG(aHidd_PixFmt_BlueShift,		24);
                ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
                ADDTAG(aHidd_PixFmt_RedMask,		0x00FF0000);
                ADDTAG(aHidd_PixFmt_GreenMask,		0x0000FF00);
                ADDTAG(aHidd_PixFmt_BlueMask,		0x000000FF);
                ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
                ADDTAG(aHidd_PixFmt_CLUTMask,		0x000000FF);
                ADDTAG(aHidd_PixFmt_CLUTShift,		0);
                ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
                ADDTAG(aHidd_PixFmt_Depth,		8);
                ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
                ADDTAG(aHidd_PixFmt_BitsPerPixel,	8);
                ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
                ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
                ADDTAG(TAG_DONE, 0);
                
                pftags_aga[1] = tagptr;
                ADDTAG(aHidd_PixFmt_RedShift,		 8);
                ADDTAG(aHidd_PixFmt_GreenShift, 	16);
                ADDTAG(aHidd_PixFmt_BlueShift,		24);
                ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
                ADDTAG(aHidd_PixFmt_RedMask,		0x00FF0000);
                ADDTAG(aHidd_PixFmt_GreenMask,		0x0000FF00);
                ADDTAG(aHidd_PixFmt_BlueMask,		0x000000FF);
                ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
                ADDTAG(aHidd_PixFmt_CLUTMask,		0x000000FF);
                ADDTAG(aHidd_PixFmt_CLUTShift,		0);
                ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
                ADDTAG(aHidd_PixFmt_Depth,		6);
                ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
                ADDTAG(aHidd_PixFmt_BitsPerPixel,	6);
                ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
                ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
                ADDTAG(TAG_DONE, 0);

                pftags_aga[2] = NULL;

                mode_tags_aga = tagptr;
                ADDTAG(aHidd_Sync_HMin,		112);
                ADDTAG(aHidd_Sync_VMin,		112);
                ADDTAG(aHidd_Sync_HMax,		16384);
                ADDTAG(aHidd_Sync_VMax,		16384);
                
                for (j = 0; specialmask_aga[j] != 0xffffffff; j++) {
                        if (pftags_aga[j])
                                ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_aga[j]);
                        for (i = 0; i < cnt; i++) {
                                ULONG modeid = modeids[i];
                                if ((j == 0 && !(modeid & SPECIAL_MODE_MASK)) || (j > 0 && ((modeid & SPECIAL_MODE_MASK) == specialmask_aga[j]))) {
                                        makemodename(modeid, bufptr);
                                        ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
                                        bufptr += strlen(bufptr) + 1;
                                        ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
                                }
                        }
                }

                ADDTAG(TAG_DONE, 0);

                mytags[0].ti_Data = (IPTR)mode_tags_aga;
    
        } else {

                pftags_ecs_lores = tagptr;
                ADDTAG(aHidd_PixFmt_RedShift,		20);
                ADDTAG(aHidd_PixFmt_GreenShift, 	24);
                ADDTAG(aHidd_PixFmt_BlueShift,		28);
                ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
                ADDTAG(aHidd_PixFmt_RedMask,		0x00000F00);
                ADDTAG(aHidd_PixFmt_GreenMask,		0x000000F0);
                ADDTAG(aHidd_PixFmt_BlueMask,		0x0000000F);
                ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
                ADDTAG(aHidd_PixFmt_CLUTMask,		0x0000001F);
                ADDTAG(aHidd_PixFmt_CLUTShift,		0);
                ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
                ADDTAG(aHidd_PixFmt_Depth,		5);
                ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
                ADDTAG(aHidd_PixFmt_BitsPerPixel,	5);
                ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
                ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
                ADDTAG(TAG_DONE, 0);

                pftags_ecs_hires = tagptr;
                ADDTAG(aHidd_PixFmt_RedShift,		20);
                ADDTAG(aHidd_PixFmt_GreenShift, 	24);
                ADDTAG(aHidd_PixFmt_BlueShift,		28);
                ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
                ADDTAG(aHidd_PixFmt_RedMask,		0x00000F00);
                ADDTAG(aHidd_PixFmt_GreenMask,		0x000000F0);
                ADDTAG(aHidd_PixFmt_BlueMask,		0x0000000F);
                ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
                ADDTAG(aHidd_PixFmt_CLUTMask,		0x0000001F);
                ADDTAG(aHidd_PixFmt_CLUTShift,		0);
                ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
                ADDTAG(aHidd_PixFmt_Depth,		4);
                ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
                ADDTAG(aHidd_PixFmt_BitsPerPixel,	4);
                ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
                ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
                ADDTAG(TAG_DONE, 0);

                pftags_ecs_shres = tagptr;
                ADDTAG(aHidd_PixFmt_RedShift,		20);
                ADDTAG(aHidd_PixFmt_GreenShift, 	24);
                ADDTAG(aHidd_PixFmt_BlueShift,		28);
                ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
                ADDTAG(aHidd_PixFmt_RedMask,		0x00000F00);
                ADDTAG(aHidd_PixFmt_GreenMask,		0x000000F0);
                ADDTAG(aHidd_PixFmt_BlueMask,		0x0000000F);
                ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
                ADDTAG(aHidd_PixFmt_CLUTMask,		0x0000001F);
                ADDTAG(aHidd_PixFmt_CLUTShift,		0);
                ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
                ADDTAG(aHidd_PixFmt_Depth,		2);
                ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
                ADDTAG(aHidd_PixFmt_BitsPerPixel,	2);
                ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
                ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
                ADDTAG(TAG_DONE, 0);

                pftags_ecs_6 = tagptr;
                ADDTAG(aHidd_PixFmt_RedShift,		20);
                ADDTAG(aHidd_PixFmt_GreenShift, 	24);
                ADDTAG(aHidd_PixFmt_BlueShift,		28);
                ADDTAG(aHidd_PixFmt_AlphaShift,		 0);
                ADDTAG(aHidd_PixFmt_RedMask,		0x00000F00);
                ADDTAG(aHidd_PixFmt_GreenMask,		0x000000F0);
                ADDTAG(aHidd_PixFmt_BlueMask,		0x0000000F);
                ADDTAG(aHidd_PixFmt_AlphaMask,		0x00000000);
                ADDTAG(aHidd_PixFmt_CLUTMask,		0x0000001F);
                ADDTAG(aHidd_PixFmt_CLUTShift,		0);
                ADDTAG(aHidd_PixFmt_ColorModel,		vHidd_ColorModel_Palette);
                ADDTAG(aHidd_PixFmt_Depth,		6);
                ADDTAG(aHidd_PixFmt_BytesPerPixel,	1);
                ADDTAG(aHidd_PixFmt_BitsPerPixel,	6);
                ADDTAG(aHidd_PixFmt_StdPixFmt,		vHidd_StdPixFmt_Plane);
                ADDTAG(aHidd_PixFmt_BitMapType,		vHidd_BitMapType_Planar);
                ADDTAG(TAG_DONE, 0);

                mode_tags_ecs = tagptr;
                ADDTAG(aHidd_Sync_HMin,		112);
                ADDTAG(aHidd_Sync_VMin,		112);
                ADDTAG(aHidd_Sync_HMax,		csd->ecs_agnus ? 16384 : 1008);
                ADDTAG(aHidd_Sync_VMax,		csd->ecs_agnus ? 16384 : 1008);

                ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_lores);
                for (i = 0; i < cnt; i++) {
                        if ((modeids[i] & SPECIAL_MODE_MASK) || (modeids[i] & SUPER_KEY) != LORES_KEY)
                                continue;
                        makemodename(modeids[i], bufptr);
                        ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
                        bufptr += strlen(bufptr) + 1;
                        ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
                }

                ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_hires);
                for (i = 0; i < cnt; i++) {
                        if ((modeids[i] & SPECIAL_MODE_MASK) || (modeids[i] & SUPER_KEY) != HIRES_KEY)
                                continue;
                        makemodename(modeids[i], bufptr);
                        ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
                        bufptr += strlen(bufptr) + 1;
                        ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
                }

                ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_shres);
                if (csd->ecs_denise) {
                        for (i = 0; i < cnt; i++) {
                                if ((modeids[i] & SPECIAL_MODE_MASK) || (modeids[i] & SUPER_KEY) != SUPER_KEY)
                                        continue;
                                makemodename(modeids[i], bufptr);
                                ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
                                bufptr += strlen(bufptr) + 1;
                                ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
                        }
                }

                ADDTAG(aHidd_Gfx_PixFmtTags,	(IPTR)pftags_ecs_6);
                for (i = 0; i < cnt; i++) {
                        if (!(modeids[i] & SPECIAL_MODE_MASK))
                                continue;
                        makemodename(modeids[i], bufptr);
                        ADDTAG(aHidd_Sync_Description,	(IPTR)bufptr);
                        bufptr += strlen(bufptr) + 1;
                        ADDTAG(aHidd_Gfx_SyncTags, (IPTR)modetags[i]);
                }

                ADDTAG(TAG_DONE, 0);

                mytags[0].ti_Data = (IPTR)mode_tags_ecs;
        }

    D(bug("alloc=%d alloced=%d\n", allocsize, (ULONG)tagptr - (ULONG)tags));
    D(bug("allocbuf=%d allocedbuf=%d\n", allocsizebuf, bufptr - buf));

    EnterFunc(bug("AGFX::New()\n"));

    mymsg.mID	= msg->mID;
    mymsg.attrList = mytags;
    msg = &mymsg;

    /* Register gfxmodes */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL != o)
    {
        struct amigagfx_data *data = OOP_INST_DATA(cl, o);
        struct NativeChipsetMode *node;
        HIDDT_ModeID *midp;
        UWORD pfcnt;
        OOP_MethodID HiddGfxBase = csd->cs_HiddGfxBase;
        OOP_Object *pixelformats[8] = { 0 };

        D(bug("AGFX::New(): Got object from super\n"));
        NewList((struct List *)&data->bitmaps);
        csd->initialized = 1;

        /* this can't be the right way to match modes.. */
        csd->superforward = TRUE;
        midp = HIDD_Gfx_QueryModeIDs(o, NULL);
        for (pfcnt = 0, i = 0; midp[i] != vHidd_ModeID_Invalid; i++) {
            OOP_Object *sync, *pf;
            HIDDT_ModeID mid = midp[i];
            if (!HIDD_Gfx_GetMode(o, mid, &sync, &pf))
                continue;
            for (j = 0; j < pfcnt; j++) {
                if (pf == pixelformats[j])
                    break;
            }
            if (j < pfcnt)
                continue;
            pixelformats[pfcnt++] = pf;
        }
        while (pfcnt < 8) {
            if (csd->aga)
                pixelformats[pfcnt] = pixelformats[0];
            else
                pixelformats[pfcnt] = pixelformats[pfcnt - 1];
            pfcnt++;
        }
        for (i = 0; i < pfcnt; i++) {
            DB2(bug("pf %d: %p\n", i, pixelformats[i]));
        } 

        ForeachNode(&csd->nativemodelist, node) {
            if (!node->pf) {
                OOP_Object *sync = NULL;
                for (i = 0; midp[i] != vHidd_ModeID_Invalid; i++) {
                    HIDDT_ModeID mid = midp[i];
                    IPTR dwidth, dheight;
                    OOP_Object *pf;
                    struct NativeChipsetMode *node2;
                    BOOL found = FALSE;
                    if (!HIDD_Gfx_GetMode(o, mid, &sync, &pf))
                        continue;
                    OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
                    OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);
                    if (node->width != dwidth || node->height != dheight)
                        continue;
                    ForeachNode(&csd->nativemodelist, node2) {
                        if (node->width == dwidth && node->height == dheight) {
                            if (node2->sync == sync) {
                                found = TRUE;
                                break;
                            }
                        }
                    }
                    if (!found)
                        break;
                }
                if (midp[i] == vHidd_ModeID_Invalid) {
                    sync = NULL;
                    D(bug("sync not found!?\n"));
                }
                node->sync = sync;
                node->pf = pixelformats[node->special];
                DB2(bug("%08x %dx%dx%d sync = %p pf = %p\n",
                    node->modeid, node->width, node->height, node->depth, node->sync, node->pf));
            }
        }   	    
        HIDD_Gfx_ReleaseModeIDs(o, midp);
        csd->superforward = FALSE;
#if 1
        ForeachNode(&csd->nativemodelist, node) {
            DB2(bug("%08x %dx%dx%d sync = %p pf = %p\n", node->modeid, node->width, node->height, node->depth, node->sync, node->pf));
        }
#endif
        
    }
    FreeVec(buf);
    FreeVec(tags);
    ReturnPtr("AGFX::New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
VOID AmigaVideoCl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    EnterFunc(bug("AGFX::Dispose(o=%p)\n", o));
    
    D(bug("AGFX::Dispose: calling super\n"));    
    OOP_DoSuperMethod(cl, o, msg);
    
    ReturnVoid("AGFX::Dispose");
}


OOP_Object *AmigaVideoCl__Hidd_Gfx__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CreateObject *msg)
{  
    OOP_Object      *object = NULL;

    EnterFunc(bug("AGFX::CreateObject()\n"));

    if (msg->cl == CSD(cl)->cs_basebm)
    {
        struct amigavideo_staticdata *csd = CSD(cl);
        struct Library *UtilityBase = csd->cs_UtilityBase;
        HIDDT_ModeID		modeid;
        struct pHidd_Gfx_CreateObject   p;
        struct TagItem tags[] =
        {
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_MORE, (IPTR)msg->attrList }
        };

        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        D(bug("modeid=%08x\n", modeid));
        if (modeid != vHidd_ModeID_Invalid) {
            tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
            tags[0].ti_Data = (IPTR)CSD(cl)->amigabmclass;

        }
        p.mID = msg->mID;
        p.cl = msg->cl;
        p.attrList = tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    ReturnPtr("AGFX::CreateObject", OOP_Object *, object);
}

VOID AmigaVideoCl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    ULONG idx;
    
    //bug("AmigaVideoCl__Root__Get %x\n", msg->attrID);

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        //bug("=%x\n", idx);
        switch (idx)
        {
            case aoHidd_Gfx_HWSpriteTypes:
                *msg->storage = vHidd_SpriteType_3Plus1;
            return;
            case aoHidd_Gfx_SupportsHWCursor:
            case aoHidd_Gfx_NoFrameBuffer:
                *msg->storage = TRUE;
            return;
            case aoHidd_Gfx_IsWindowed:
                *msg->storage = FALSE;
            return;
            case aoHidd_Gfx_DriverName:
                *msg->storage = (IPTR)"AmigaVideo";
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID AmigaVideoCl__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *UtilityBase = csd->cs_UtilityBase;
    struct TagItem *tag, *tstate;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        ULONG idx;
        D(bug("AmigaVideoCl__Root__Set %x\n", tag->ti_Tag));
        if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
            D(bug("->%d\n", idx));
            switch(idx)
            {
            case aoHidd_Gfx_ActiveCallBack:
                csd->acb = (void *)tag->ti_Data;
                break;

            case aoHidd_Gfx_ActiveCallBackData:
                csd->acbdata = (APTR)tag->ti_Data;
                break;
            }
        }
    }
    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}

ULONG AmigaVideoCl__Hidd_Gfx__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    struct HIDD_ViewPortData *vpd = msg->Data;
    OOP_Object *bm = NULL;

    if (vpd)
        bm = vpd->Bitmap;

    D(bug("AmigaShowViewPorts %p %p\n", vpd, bm));

    if (bm) {
        struct amigabm_data *data = OOP_INST_DATA(OOP_OCLASS(bm), bm);
        IPTR tags[] = {aHidd_BitMap_Visible, TRUE, TAG_DONE};
        IPTR modeid = vHidd_ModeID_Invalid;

        OOP_GetAttr(bm, aHidd_BitMap_ModeID , &modeid);
        csd->modeid = modeid;
        setmode(csd, data);
        OOP_SetAttrs(bm, (struct TagItem *)tags);

        if (csd->acb)
             csd->acb(csd->acbdata, bm);

    } else {
        resetmode(csd);
    }

    return TRUE;
}

VOID AmigaVideoCl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    IPTR src, dst;
    BOOL ok = FALSE;
 
    OOP_GetAttr(msg->src,  aHidd_BitMap_AmigaVideo_Drawable, &src);
    OOP_GetAttr(msg->dest, aHidd_BitMap_AmigaVideo_Drawable, &dst);
    if (src && dst) {
        struct amigabm_data *sdata = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct amigabm_data *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
        ok = blit_copybox(csd, sdata->pbm, ddata->pbm, msg->srcX, msg->srcY, msg->width, msg->height, msg->destX, msg->destY, mode);
    }
    if (!ok)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);   
}

BOOL AmigaVideoCl__Hidd_Gfx__CopyBoxMasked(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBoxMasked *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    IPTR src, dst;
    BOOL ok = FALSE;
 
    OOP_GetAttr(msg->src,  aHidd_BitMap_AmigaVideo_Drawable, &src);
    OOP_GetAttr(msg->dest, aHidd_BitMap_AmigaVideo_Drawable, &dst);
    if (src && dst) {
        struct amigabm_data *sdata = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct amigabm_data *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
        ok = blit_copybox_mask(csd, sdata->pbm, ddata->pbm, msg->srcX, msg->srcY, msg->width, msg->height, msg->destX, msg->destY, mode, msg->mask);
    }
    if (!ok)
        return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return TRUE;
}

BOOL AmigaVideoCl__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    IPTR width, height;
    UWORD maxw, maxh;

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);

    maxw = (csd->aga ? 64 : 16);
    maxh = maxw * 2;

    if (width > maxw || height > maxh)
        return FALSE;

    return setsprite(cl, o, width, height, msg);
}

BOOL AmigaVideoCl__Hidd_Gfx__GetMaxSpriteSize(OOP_Class *cl, ULONG Type, ULONG *Width, ULONG *Height)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    *Width = csd->aga ? 64 : 16;
    *Height = *Width * 2;
    return TRUE;
}
BOOL AmigaVideoCl__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    setspritepos(csd, msg->x, msg->y);
    return TRUE;
}
VOID AmigaVideoCl__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    setspritevisible(csd, msg->visible);
}

ULONG AmigaVideoCl__Hidd_Gfx__MakeViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_MakeViewPort *msg)
{
    D(struct HIDD_ViewPortData *vpd = msg->Data);
    
    D(bug("AmigaVideoCl__Hidd_Gfx__MakeViewPort vp=%p bm=%p vpe=%p\n", vpd->vpe->ViewPort, vpd->Bitmap, vpd->vpe));
    /* TODO: implement this correctly */
    return MVP_OK;
}

void AmigaVideoCl__Hidd_Gfx__CleanViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CleanViewPort *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *GfxBase = csd->cs_GfxBase;
    struct HIDD_ViewPortData *vpd = msg->Data;
    struct ViewPort *vp = vpd->vpe->ViewPort;

    D(bug("AmigaVideoCl__Hidd_Gfx__CleanViewPort vp=%p bm=%p vpe=%p\n", vpd->vpe->ViewPort, vpd->Bitmap, vpd->vpe));
    /* It's safe to call these functions on NULL pointers */
    FreeCopList(vp->ClrIns);
    FreeCopList(vp->DspIns);
    FreeCopList(vp->SprIns);

    if (vp->UCopIns)
    {
        FreeCopList(vp->UCopIns->FirstCopList);
        FreeMem(vp->UCopIns, sizeof(struct UCopList));
    }

    /* Everything has been freed */
    vp->ClrIns  = NULL;
    vp->DspIns  = NULL;
    vp->SprIns  = NULL;
    vp->UCopIns = NULL;
}

static void freeattrbases(struct amigavideo_staticdata *csd)
{
    struct Library *OOPBase = csd->cs_OOPBase;

    OOP_ReleaseAttrBase(IID_Hidd);
    OOP_ReleaseAttrBase(IID_Hidd_BitMap);
    OOP_ReleaseAttrBase(IID_Hidd_PlanarBM);
    OOP_ReleaseAttrBase(IID_Hidd_BitMap_AmigaVideo);
    OOP_ReleaseAttrBase(IID_Hidd_GC);
    OOP_ReleaseAttrBase(IID_Hidd_Sync);
    OOP_ReleaseAttrBase(IID_Hidd_Gfx);
    OOP_ReleaseAttrBase(IID_Hidd_PixFmt);
    OOP_ReleaseAttrBase(IID_Hidd_ColorMap);
}

int Init_AmigaVideoClass(LIBBASETYPEPTR LIBBASE)
{
    struct amigavideo_staticdata *csd = &LIBBASE->csd;
    struct Library *OOPBase = csd->cs_OOPBase;
    
    D(bug("Init_AmigaVideoClass\n"));
    __IHidd             = OOP_ObtainAttrBase(IID_Hidd);
    __IHidd_BitMap  	= OOP_ObtainAttrBase(IID_Hidd_BitMap);
    __IHidd_PlanarBM  	= OOP_ObtainAttrBase(IID_Hidd_PlanarBM);
    __IHidd_BitMap_AmigaVideo  	= OOP_ObtainAttrBase(IID_Hidd_BitMap_AmigaVideo);
    __IHidd_GC      	= OOP_ObtainAttrBase(IID_Hidd_GC);
    __IHidd_Sync    	= OOP_ObtainAttrBase(IID_Hidd_Sync);
    __IHidd_Gfx     	= OOP_ObtainAttrBase(IID_Hidd_Gfx);
    __IHidd_PixFmt		= OOP_ObtainAttrBase(IID_Hidd_PixFmt);
    __IHidd_ColorMap 	= OOP_ObtainAttrBase(IID_Hidd_ColorMap);
    
    if (!__IHidd || !__IHidd_BitMap || !__IHidd_PlanarBM || !__IHidd_BitMap_AmigaVideo ||
        !__IHidd_GC || !__IHidd_Sync || !__IHidd_Gfx || !__IHidd_PixFmt || !__IHidd_ColorMap)
    {
        D(bug("Init_AmigaVideoClass fail\n"));
        freeattrbases(csd);
        return 0;
    }
    return TRUE;
}


static int Expunge_AmigaVideoClass(LIBBASETYPEPTR LIBBASE)
{
    struct amigavideo_staticdata *csd = &LIBBASE->csd;
    D(bug("Expunge_AmigaVideoClass\n"));
    freeattrbases(csd);
    return TRUE;
}

ADD2EXPUNGELIB(Expunge_AmigaVideoClass, 1)
