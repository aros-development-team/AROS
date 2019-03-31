/*
    Copyright  1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for p96 rtg card drivers.
    Lang: English.
    
*/

/****************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/alerts.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <oop/oop.h>
#include <hidd/gfx.h>
#include <aros/symbolsets.h>

#include <hardware/custom.h>

#define DEBUG 0
#define DB2(x)
#define DEBUG_TEXT(x)
#define DVRAM(x)
#define DCLUT(x)
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "p96gfx_intern.h"
#include "p96gfx_bitmap.h"
#include "p96gfx_rtg.h"

/* LOCK_BITMAP_MULTI:

  swap bitmap between RAM <-> VRAM allowed only if this lock is held
 
  the bitmap being swapped must also get LOCK_BITMAP lock, otherwise
  gfx functions (done by other tasks) could render into wrong place while
  bitmap is moved around.

*/

/* poke 0xFF into the videodata for debugging purposes - may be dangerous on some cards! */
/*#define DEBUG_POKE_VIDEODATA*/

static BOOL P96GFXBitmap__ToFRAM(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm);

static APTR P96GFXBitmap__AllocVRAM(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    APTR vmem;

    DVRAM(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    SetMemoryMode(cid, RGBFB_CLUT);
    vmem = Allocate(cid->vmem, bm->memsize);
    SetMemoryMode(cid, bm->rgbformat);
    if (vmem)
    {
        DVRAM(bug("[P96Gfx:Bitmap] %s: Bitmap @ 0x%p (%dx%dx%d %d): %p,%d bytes VRAM allocated.\n", __func__, bm, bm->width, bm->height, bm->bytesperpixel*8, bm->bytesperline, vmem, bm->memsize));
    }
    return vmem;
}

static BOOL P96GFXBitmap__AllocBM(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm, BOOL usevram)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    bm->memsize = (bm->bytesperline * bm->height + 7) & ~7;

    if (!(bm->VideoData = P96GFXBitmap__AllocVRAM(cl, o, bm)))
    {
        if (usevram && bm->memsize < cid->vram_size)
        {
             struct P96GfxBitMapData *bmnode;
             ForeachNode(&cid->bitmaplist, bmnode)
             {
                if (bmnode != bm && bmnode->invram && !bmnode->locked)
                {
                    if (P96GFXBitmap__ToFRAM(cl, o, bmnode))
                    {
                        if ((bm->VideoData = P96GFXBitmap__AllocVRAM(cl, o, bm)))
                        {
                            cid->vram_used += bm->memsize;
                            bm->invram = TRUE;
                            break;
                        }
                    }
                }
             }
        }

        if (!bm->VideoData)
        {
            bm->VideoData = AllocMem(bm->memsize, MEMF_ANY);
            if (bm->VideoData)
            {
                cid->fram_used += bm->memsize;
            }
        }
    }
    else
    {
        cid->vram_used += bm->memsize;
        bm->invram = TRUE;
    }
    DVRAM(bug("[P96Gfx:Bitmap] %s: Bitmap @ 0x%p (%p:%d) bytes allocated from %s. VRAM=%d\n", __func__, bm, bm->VideoData, bm->memsize, bm->invram ? "VRAM" : "RAM", cid->vram_used));

#ifdef DEBUG_POKE_VIDEODATA
    /* poke 0xFF into the videodata for debugging purposes - may be dangerous on some cards! */
    if (bm->VideoData != NULL)
    {
        int i;
    
        for(i = 0; i < bm->memsize; i++) bm->VideoData[i] = 0xFF;
    }
#endif
    return bm->VideoData != NULL;
}

static void P96GFXBitmap__FreeBM(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    DVRAM(
      bug("[P96Gfx:Bitmap] %s()\n", __func__);
      bug("[P96Gfx:Bitmap] %s: Bitmap @ 0x%p: freeing %p:%d from %s\n", __func__, bm, bm->VideoData, bm->memsize, bm->invram ? "VRAM" : "RAM");
     )

    if (bm->invram)
    {
        SetMemoryMode(cid, RGBFB_CLUT);
        Deallocate(cid->vmem, bm->VideoData, bm->memsize);
        SetMemoryMode(cid, bm->rgbformat);
        cid->vram_used -= bm->memsize;
    }
    else if (bm->VideoData)
    {
        FreeMem(bm->VideoData, bm->memsize);
        cid->fram_used -= bm->memsize;
    }
    bm->VideoData = NULL;
    bm->invram = FALSE;
}	

static BOOL P96GFXBitmap__ToFRAM(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    BOOL ok = FALSE;
    APTR vmem;

    DVRAM(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    /* TRYLOCK here as we are in wrong locking order (could deadlock):
    
      LOCK_BITMAP_MULTI -> LOCK_HW -> [...] <- UNLOCK_HW <- UNLOCK_BITMAP_MULTI
      (Correct locking order is: LOCK_BM_MULTI -> LOCK_BM -> LOCK_HW)
      
      Alternative would be to always lock all (swappable) bitmaps during bitmap
      allocation/freeing routines. */

    if (TRYLOCK_BITMAP(bm))
    {
        vmem = AllocMem(bm->memsize, MEMF_ANY);
        if (vmem)
        {
            SetMemoryMode(cid, bm->rgbformat);
            CopyMemQuick(bm->VideoData, vmem, bm->memsize);
            P96GFXBitmap__FreeBM(cl, o, bm);
            bm->VideoData = vmem;
            bm->invram = FALSE;
            cid->fram_used += bm->memsize;
            ok = TRUE;
       }
       DVRAM(bug("[P96Gfx:Bitmap] %s: Bitmap @ 0x%p (%p:%d) %d x %d moved to RAM . VRAM=%d\n", __func__, bm->VideoData, bm->memsize, bm, bm->width, bm->height, cid->vram_used));

       UNLOCK_BITMAP(bm)
    }

   return ok;
}

static BOOL P96GFXBitmap__BMToVRAM(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    APTR vmem;

    DVRAM(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    vmem = P96GFXBitmap__AllocVRAM(cl, o, bm);
    if (vmem)
    {
        SetMemoryMode(cid, bm->rgbformat);
        CopyMemQuick(bm->VideoData, vmem, bm->memsize);
        P96GFXBitmap__FreeBM(cl, o, bm);
        bm->VideoData = vmem;
        bm->invram = TRUE;
        cid->vram_used += bm->memsize;
        DVRAM(bug("[P96Gfx:Bitmap] %s: Bitmap @ 0x%p (%p:%d) %d x %d moved back to VRAM\n", __func__, bm, bm->VideoData, bm->memsize, bm->width, bm->height));
        return TRUE;
    }
    return FALSE;
}

static BOOL P96GFXBitmap__ToVRAM(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    struct P96GfxBitMapData *bmnode;

    DVRAM(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    if (bm->invram)
        return TRUE;

    DVRAM(bug("[P96Gfx:Bitmap] %s: Bitmap @ 0x%p (%p:%d) needs to be in VRAM...\n", __func__, bm, bm->VideoData, bm->memsize));

    ForeachNode(&cid->bitmaplist, bmnode)
    {
        if (bmnode != bm && bmnode->invram && !bmnode->locked)
        {
            if (P96GFXBitmap__ToFRAM(cl, o, bmnode))
            {
                if (P96GFXBitmap__BMToVRAM(cl, o, bm))
                {
                    return TRUE;
                }
            }
        }
    }

    DVRAM(bug("[P96Gfx:Bitmap] %s: -> not enough memory, VRAM=%d\n", __func__, cid->vram_used));

    return FALSE;
}

#if 0
static BOOL maybeputinvram(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    if (bm->invram)
        return TRUE;
    if (bm->memsize >= cid->vram_size - cid->vram_used)
        return FALSE;
    return P96GFXBitmap__BMToVRAM(cl, o, bm);
}
#endif

static void P96GFXBitmap__HideScreen(OOP_Class *cl, OOP_Object *o, struct P96GfxBitMapData *bm)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx:Bitmap] %s: Bitmap @ 0x%p (%p:%d)\n",
        __func__, bm, bm->VideoData, bm->memsize));

    SetInterrupt(cid, FALSE);
    SetDisplay(cid, FALSE);
    SetSwitch(cid, FALSE);
    cid->dmodeid = 0;
    bm->locked--;
    cid->disp = NULL;
}

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

/****************************************************************************************/

OOP_Object *P96GFXBitmap__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    BOOL  ok = TRUE;      
    struct P96GfxBitMapData *data;
    struct p96gfx_carddata *cid;
    IPTR 	    	     width, height, multi;
    IPTR		     displayable;
    struct TagItem tags[2];
    ULONG softsflags;

    DB2(bug("[P96Gfx:Bitmap] %s(%x)\n", __func__, o));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    {
        return NULL;
    }

    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof  (*data));
    InitSemaphore(&data->bmLock);

    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
    OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
    OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&data->gfxhidd);
    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&data->pixfmtobj);
    OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_BytesPerPixel, &multi);

    OOP_GetAttr(data->gfxhidd, aHidd_P96Gfx_CardData, (APTR)&data->gfxCardData);
    DB2(bug("[P96Gfx:Bitmap] %s: Card Data @ 0x%p\n", __func__, data->gfxCardData));
    cid = data->gfxCardData;

    data->rgbformat = P96GFXRTG__GetFormat(csd, cid, data->pixfmtobj);

    data->align = displayable ? 32 : 16;
    width = (width + data->align - 1) & ~(data->align - 1);
    data->bytesperline = CalculateBytesPerRow(cid, width, data->rgbformat);
    data->width = width;
    data->height = height;
    data->bytesperpixel = multi;

    LOCK_MULTI_BITMAP

    LOCK_HW /* alloc routines call SetMemoryMode */
    WaitBlitter(cid); /* in case bitmaps are swapped between RAM <-> VRAM during allocation */
    P96GFXBitmap__AllocBM(cl, o, data, TRUE);
    UNLOCK_HW

    AddTail(&cid->bitmaplist, (struct Node*)&data->node);

    UNLOCK_MULTI_BITMAP

    tags[0].ti_Tag = aHidd_BitMap_BytesPerRow;
    tags[0].ti_Data = data->bytesperline;
    tags[1].ti_Tag = TAG_DONE;
    OOP_SetAttrs(o, tags);

    DB2(bug("[P96Gfx:Bitmap] %s: %dx%dx%d %d RGBF=%08x P=%08x\n", __func__, width, height, multi, data->bytesperline, data->rgbformat, data->VideoData));

    if (data->VideoData == NULL)
        ok = FALSE;

    if (!ok) {
        OOP_MethodID dispose_mid;
        dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
        o = NULL;
    }

    DB2(bug("[P96Gfx:Bitmap] %s: ret=%x bm=%p (%p:%d)\n", __func__, o, data, data->VideoData, data->memsize));

    return o;
}

VOID P96GFXBitmap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx:Bitmap] %s(%x)\n", __func__, o));

    LOCK_HW
    WaitBlitter(cid);

    DB2(bug("[P96Gfx:Bitmap] %s: bm=%x (%p,%d)\n", __func__, data, data->VideoData, data->memsize));
    if (cid->disp == data)
        P96GFXBitmap__HideScreen(cl, o, data);

    UNLOCK_HW

    FreeVec(data->palette);

    LOCK_MULTI_BITMAP

    LOCK_HW /* free functions call SetMemoryMode */
    P96GFXBitmap__FreeBM(cl, o, data);
    UNLOCK_HW

    Remove((struct Node*)&data->node);

    UNLOCK_MULTI_BITMAP

    OOP_DoSuperMethod(cl, o, msg);
}

VOID P96GFXBitmap__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    BOOL moved = FALSE;

    DB2(bug("[P96Gfx:Bitmap] %s: %p (%p:%d)\n", __func__, data, data->VideoData, data->memsize));
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        DB2(bug("[P96Gfx:Bitmap] %s: Tag %d/%d\n", __func__, tag->ti_Tag, tag->ti_Data));
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            DB2(bug("[P96Gfx:Bitmap] %s: BitMap Attr ->%d\n", __func__, idx));
            switch(idx)
            {
                case aoHidd_BitMap_Focus:
                    {
                        ULONG boardType = gl(data->gfxCardData->boardinfo + PSSO_BoardInfo_BoardType);
                        DB2(bug("[P96Gfx:Bitmap] %s: aoHidd_BitMap_Focus\n", __func__);)
                        if (boardType == P96BoardType_Vampire)
                        {
                            struct GfxBase *GfxBase = (struct GfxBase *)csd->cs_GfxBase;
                            volatile struct Custom *custom = (struct Custom*)0xdff000;
                            /* Tell the Vampire SAGA chipset we are visible */
                            custom->bplcon0 = GfxBase->system_bplcon0 | 0x80;
                        }
                    }
                    break;

                case aoHidd_BitMap_Visible:
                LOCK_MULTI_BITMAP
                LOCK_BITMAP(data)
                LOCK_HW
                if (tag->ti_Data) {
                    OOP_Object *gfxhidd, *sync, *pf;
                    IPTR modeid = vHidd_ModeID_Invalid;
                    IPTR dwidth, dheight, depth, width, height;
                    struct ModeInfo *modeinfo;

                    width = data->width;
                    height = data->height;
                    OOP_GetAttr(o, aHidd_BitMap_ModeID , &modeid);
                    OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
                    HIDD_Gfx_GetMode(gfxhidd, modeid, &sync, &pf);
                    OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
                    OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);
                    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
                    data->rgbformat = P96GFXRTG__GetFormat(csd, cid, pf);
                    modeinfo = P96GFXRTG__GetModeInfo(csd, cid, sync, pf, cid->fakemodeinfo);
                    cid->modeinfo = modeinfo;
                    *cid->rgbformat = data->rgbformat;
                    pw(cid->bitmapextra + PSSO_BitMapExtra_Width, width);
                    pw(cid->bitmapextra + PSSO_BitMapExtra_Height, height);
                    D(bug("[P96Gfx:Bitmap] %s: Show %p: (%p:%d) %dx%dx%d (%dx%d) BF=%08x\n",
                        __func__, data, data->VideoData, data->memsize,
                        dwidth, dheight, depth, width, height, data->rgbformat));

                    if (!data->invram)
                    {
                        WaitBlitter(cid); /* in case other bitmaps are swapped from VRAM to RAM */
                        if (!P96GFXBitmap__ToVRAM(cl, o, data))
                        {
                            struct P96GfxBitMapData *bmnode;

                            /* Second try. Now lock all bitmaps first. UNLOCK_HW first, to ensure
                               correct locking order: multibm -> bm -> hw */

                            UNLOCK_HW

                            ForeachNode(&cid->bitmaplist, bmnode)
                            {
                                if (bmnode != data) LOCK_BITMAP(bmnode)
                            }

                            LOCK_HW                            
                            WaitBlitter(cid); /* in case other bitmaps are swapped from VRAM to RAM */
                            P96GFXBitmap__ToVRAM(cl, o, data); /* shouldn't fail this time. If it does we are screwed ... */                          
                            UNLOCK_HW

                            ForeachNode(&cid->bitmaplist, bmnode)
                            {
                                if (bmnode != data) UNLOCK_BITMAP(bmnode)
                            }                            

                            LOCK_HW
                        }
                    }

                    cid->dwidth = dwidth;
                    cid->dheight = dheight;
                    cid->dmodeid = modeid;

                    if (cid->hardwaresprite && depth <= 8) {
                        HIDDT_Color c;
                        UWORD i;
                        UBYTE *clut = data->gfxCardData->boardinfo + PSSO_BoardInfo_CLUT;
                        for (i = cid->spritepencnt + 1; i < cid->spritepencnt + 4; i++)
                        {
                            c.red = clut[i * 3 + 0];
                            c.green = clut[i * 3 + 1];
                            c.blue = clut[i * 3 + 2];
                            HIDD_P96GFX_SetCursorPen(gfxhidd, i, c);
                        }
                    }
                    SetInterrupt(cid, FALSE);
                    SetColorArray(cid, 0, 256);
                    SetDisplay(cid, FALSE);
                    SetGC(cid, modeinfo, 0);
                    SetClock(cid);
                    SetDAC(cid);
                    SetPanning(cid, data->VideoData, width, 0, 0);
                    SetDisplay(cid, TRUE);
                    SetSwitch(cid, TRUE);
                    SetInterrupt(cid, TRUE);
                    cid->disp = data;
                    cid->disp->locked++;
                } else {
                    P96GFXBitmap__HideScreen(cl, o, data);
                }
                UNLOCK_HW
                UNLOCK_BITMAP(data)
                UNLOCK_MULTI_BITMAP

                break;
                case aoHidd_BitMap_LeftEdge:
                    if (data->leftedge != tag->ti_Data) {
                        data->leftedge = tag->ti_Data;
                        moved = TRUE;
                    }
                break;
                case aoHidd_BitMap_TopEdge:
                    if (data->topedge != tag->ti_Data) {
                        data->topedge = tag->ti_Data;
                        moved = TRUE;
                    }
                break;
            }
        }
    }
    D(bug("[P96Gfx:Bitmap] %s: Exit\n", __func__));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
#if 0
    if (moved && cid->disp == data)
        setscroll(cid, data);
#else
    (void)moved;
#endif
}

VOID P96GFXBitmap__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    ULONG idx;

    //DB2(bug("P96GFXBitmap__Root__Get\n"));
    if (IS_BITMAP_ATTR(msg->attrID, idx)) {
        //DB2(bug("=%d\n", idx));
        switch (idx) {
        case aoHidd_BitMap_LeftEdge:
            *msg->storage = 0;//data->leftedge;
            return;
        case aoHidd_BitMap_TopEdge:
            *msg->storage = 0;//data->topedge;
            return;
        case aoHidd_BitMap_Visible:
            *msg->storage = cid->disp == data;
            return;
        case aoHidd_BitMap_Align:
            *msg->storage = data->align;
            return;
        case aoHidd_BitMap_BytesPerRow:
                *msg->storage = data->bytesperline;
                return;
        case aoHidd_BitMap_IsLinearMem:
            *msg->storage = TRUE;
            return;
        }
    }
    //DB2(bug("P96GFXBitmap__Root__Get Exit\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

static int P96GFXBitmap_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));
    return TRUE; //return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int P96GFXBitmap_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));
    //OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(P96GFXBitmap_Init, 0);
ADD2EXPUNGELIB(P96GFXBitmap_Expunge, 0);

BOOL P96GFXBitmap__Hidd_BitMap__ObtainDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    LOCK_BITMAP(data)

#if 0
    if (!data->invram) {
        if (!P96GFXBitmap__ToVRAM(cl, o, data))
            return FALSE;
    }
#endif

    *msg->addressReturn = data->VideoData;
    *msg->widthReturn = data->width;
    *msg->heightReturn = data->height;
    /* undocumented, just a guess.. */
    *msg->bankSizeReturn = *msg->memSizeReturn = data->bytesperline * data->height;
    data->locked++;

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    return TRUE;
}

VOID P96GFXBitmap__Hidd_BitMap__ReleaseDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    data->locked--;

    UNLOCK_BITMAP(data)
}

BOOL P96GFXBitmap__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    WORD i, j;
    UBYTE *clut;

    DCLUT(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
        return FALSE;

    LOCK_HW

    WaitBlitter(cid);
    clut = data->gfxCardData->boardinfo + PSSO_BoardInfo_CLUT;
    DCLUT(bug("[P96Gfx:Bitmap] %s: CLUT @ %p\n", __func__, clut));

    for (i = msg->firstColor, j = 0; j < msg->numColors; i++, j++) {
        clut[i * 3 + 0] = msg->colors[j].red >> 8;
        clut[i * 3 + 1] = msg->colors[j].green >> 8;
        clut[i * 3 + 2] = msg->colors[j].blue >> 8;
        DCLUT(bug("[P96Gfx:Bitmap] %s: color %d %02x%02x%02x\n", __func__, i, msg->colors[j].red >> 8, msg->colors[j].green >> 8, msg->colors[j].blue >> 8));
    }
    SetColorArray(cid, msg->firstColor, msg->numColors);

    UNLOCK_HW

    return TRUE;
}

VOID P96GFXBitmap__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_PutPixel *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    ULONG   	       offset;
    HIDDT_Pixel       pixel = msg->pixel;
    UBYTE   	      *mem;

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    offset = (msg->x * data->bytesperpixel) + (msg->y * data->bytesperline);
    mem = data->VideoData + offset;

    switch(data->bytesperpixel)
    {
        case 1:
            *(UBYTE *)mem = pixel;
            break;

        case 2:
            *(UWORD *)mem = pixel;
            break;

        case 3:
            *(UBYTE *)(mem) = pixel >> 16;
            *(UBYTE *)(mem + 1) = pixel >> 8;
            *(UBYTE *)(mem + 2) = pixel;
            break;

        case 4:
            *(ULONG *)mem = pixel;
            break;
    }

    UNLOCK_BITMAP(data)

    return;
}

/****************************************************************************************/

ULONG P96GFXBitmap__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o,
                                 struct pHidd_BitMap_GetPixel *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    HIDDT_Pixel     	 pixel = 0;
    ULONG   	    	 offset;
    UBYTE   	    	*mem;

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    offset = (msg->x * data->bytesperpixel)  +(msg->y * data->bytesperline);
    mem = data->VideoData + offset;

    switch(data->bytesperpixel)
    {
        case 1:
            pixel = *(UBYTE *)mem;
            break;

        case 2:
            pixel = *(UWORD *)mem;
            break;

        case 3:
            pixel = (mem[0] << 16) | (mem[1] << 8) | mem[2];
            break;

        case 4:
            pixel = *(ULONG *)mem;
            break;
    }

    UNLOCK_BITMAP(data)

    return pixel;
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__DrawLine(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_DrawLine *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    BOOL v = FALSE;

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_HW
    WaitBlitter(cid);
#if (0)
    if (data->invram) {
        struct RenderInfo ri;
        struct Line renderLine;
        P96GFXRTG__MakeRenderInfo(csd, cid, &ri, data);
        renderline.FgPen = GC_FG(msg->gc);
        renderline.BgPen = GC_BG(msg->gc);
        renderline.LinePtrn = GC_LINEPAT(gc);
        renderline.X = msg->x1;
        renderline.Y = msg->y1;
        renderline.dX = msg->x2;
        renderline.dY = msg->y2;
        v = DrawLine(cid, &ri, &renderLine, data->rgbformat);
    }
#endif
    UNLOCK_HW

    if (!v) OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    switch(msg->pixFmt)
    {
        case vHidd_StdPixFmt_Native:
            switch(data->bytesperpixel)
            {
                case 1:
                    HIDD_BM_CopyMemBox8(o,
                                        data->VideoData,
                                        msg->x,
                                        msg->y,
                                        msg->pixels,
                                        0,
                                        0,
                                        msg->width,
                                        msg->height,
                                        data->bytesperline,
                                        msg->modulo);
                    break;

                case 2:
                    HIDD_BM_CopyMemBox16(o,
                                         data->VideoData,
                                         msg->x,
                                         msg->y,
                                         msg->pixels,
                                         0,
                                         0,
                                         msg->width,
                                         msg->height,
                                         data->bytesperline,
                                         msg->modulo);
                    break;

                case 3:
                    HIDD_BM_CopyMemBox24(o,
                                         data->VideoData,
                                         msg->x,
                                         msg->y,
                                         msg->pixels,
                                         0,
                                         0,
                                         msg->width,
                                         msg->height,
                                         data->bytesperline,
                                         msg->modulo);
                    break;

                case 4:
                    HIDD_BM_CopyMemBox32(o,
                                         data->VideoData,
                                         msg->x,
                                         msg->y,
                                         msg->pixels,
                                         0,
                                         0,
                                         msg->width,
                                         msg->height,
                                         data->bytesperline,
                                         msg->modulo);
                    break;
             } /* switch(data->bytesperpix) */
            break;

        case vHidd_StdPixFmt_Native32:
            switch(data->bytesperpixel)
            {
                case 1:
                    HIDD_BM_GetMem32Image8(o,
                                           data->VideoData,
                                           msg->x,
                                           msg->y,
                                           msg->pixels,
                                           msg->width,
                                           msg->height,
                                           data->bytesperline,
                                           msg->modulo);
                    break;

                case 2:
                    HIDD_BM_GetMem32Image16(o,
                                            data->VideoData,
                                            msg->x,
                                            msg->y,
                                            msg->pixels,
                                            msg->width,
                                            msg->height,
                                            data->bytesperline,
                                            msg->modulo);
                    break;

                case 3:
                    HIDD_BM_GetMem32Image24(o,
                                            data->VideoData,
                                            msg->x,
                                            msg->y,
                                            msg->pixels,
                                            msg->width,
                                            msg->height,
                                            data->bytesperline,
                                            msg->modulo);
                    break;

                case 4:		    
                    HIDD_BM_CopyMemBox32(o,
                                         data->VideoData,
                                         msg->x,
                                         msg->y,
                                         msg->pixels,
                                         0,
                                         0,
                                         msg->width,
                                         msg->height,
                                         data->bytesperline,
                                         msg->modulo);
                    break;
            } /* switch(data->bytesperpix) */
            break;

        default:
            {
                APTR 	    pixels = msg->pixels;
                APTR 	    srcPixels = data->VideoData + msg->y * data->bytesperline + msg->x * data->bytesperpixel;
                OOP_Object *dstpf;

                dstpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);

                HIDD_BM_ConvertPixels(o, &srcPixels, (HIDDT_PixelFormat *)data->pixfmtobj, data->bytesperline,
                                      &pixels, (HIDDT_PixelFormat *)dstpf, msg->modulo,
                                      msg->width, msg->height, NULL);    	    	
            }		
            break;
    } /* switch(msg->pixFmt) */

    UNLOCK_BITMAP(data)
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_PutImage *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    BOOL v = FALSE;

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    if (data->invram)
    {
        if ((msg->pixFmt == vHidd_StdPixFmt_Native
            || msg->pixFmt == vHidd_StdPixFmt_Native32
            || msg->pixFmt == vHidd_StdPixFmt_BGRA32
            || msg->pixFmt == vHidd_StdPixFmt_BGR032))
        {
#if (0)
            struct RenderInfo ri;
            WORD sx, sy, dx, dy;

            P96GFXRTG__MakeRenderInfo(csd, cid, &ri, data);
            if (msg->pixels < data->VideoData)
            {
            }

            LOCK_HW

//       v =BlitRect(cid, &ri,
//    WORD sx, WORD sy, WORD dx, WORD dy, msg->width, msg->height, UBYTE mask, data->rgbformat);

            UNLOCK_HW
#endif
        }
    }

    if (!v) switch(msg->pixFmt)
    {
        case vHidd_StdPixFmt_Native:
            switch(data->bytesperpixel)
            {
                case 1:
                    HIDD_BM_CopyMemBox8(o,
                                        msg->pixels,
                                        0,
                                        0,
                                        data->VideoData,
                                        msg->x,
                                        msg->y,
                                        msg->width,
                                        msg->height,
                                        msg->modulo,
                                        data->bytesperline);
                    break;

                case 2:
                    HIDD_BM_CopyMemBox16(o,
                                         msg->pixels,
                                         0,
                                         0,
                                         data->VideoData,
                                         msg->x,
                                         msg->y,
                                         msg->width,
                                         msg->height,
                                         msg->modulo,
                                         data->bytesperline);
                    break;

                case 3:
                    HIDD_BM_CopyMemBox24(o,
                                         msg->pixels,
                                         0,
                                         0,
                                         data->VideoData,
                                         msg->x,
                                         msg->y,
                                         msg->width,
                                         msg->height,
                                         msg->modulo,
                                         data->bytesperline);
                    break;

                case 4:
                    HIDD_BM_CopyMemBox32(o,
                                         msg->pixels,
                                         0,
                                         0,
                                         data->VideoData,
                                         msg->x,
                                         msg->y,
                                         msg->width,
                                         msg->height,
                                         msg->modulo,
                                         data->bytesperline);
                    break;
            } /* switch(data->bytesperpix) */
            break;

        case vHidd_StdPixFmt_Native32:
            switch(data->bytesperpixel)
            {
                case 1:
                    HIDD_BM_PutMem32Image8(o,
                                           msg->pixels,
                                           data->VideoData,
                                           msg->x,
                                           msg->y,
                                           msg->width,
                                           msg->height,
                                           msg->modulo,
                                           data->bytesperline);
                    break;

                case 2:
                    HIDD_BM_PutMem32Image16(o,
                                            msg->pixels,
                                            data->VideoData,
                                            msg->x,
                                            msg->y,
                                            msg->width,
                                            msg->height,
                                            msg->modulo,
                                            data->bytesperline);
                    break;

                case 3:
                    HIDD_BM_PutMem32Image24(o,
                                            msg->pixels,
                                            data->VideoData,
                                            msg->x,
                                            msg->y,
                                            msg->width,
                                            msg->height,
                                            msg->modulo,
                                            data->bytesperline);
                    break;

                case 4:		    
                    HIDD_BM_CopyMemBox32(o,
                                         msg->pixels,
                                         0,
                                         0,
                                         data->VideoData,
                                         msg->x,
                                         msg->y,
                                         msg->width,
                                         msg->height,
                                         msg->modulo,
                                         data->bytesperline);
                    break;
            } /* switch(data->bytesperpix) */
            break;

        default:
            {
                APTR 	    pixels = msg->pixels;
                APTR 	    dstBuf = data->VideoData + msg->y * data->bytesperline + msg->x * data->bytesperpixel;
                OOP_Object *srcpf;

                srcpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);

                HIDD_BM_ConvertPixels(o, &pixels, (HIDDT_PixelFormat *)srcpf, msg->modulo,
                                      &dstBuf, (HIDDT_PixelFormat *)data->pixfmtobj, data->bytesperline,
                                      msg->width, msg->height, NULL);    	    	
            }
            break;
    } /* switch(msg->pixFmt) */	  

    UNLOCK_BITMAP(data)  
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_BitMap_PutImageLUT *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    switch(data->bytesperpixel)
    {
        case 2:
            HIDD_BM_CopyLUTMemBox16(o,
                                 msg->pixels,
                                 0,
                                 0,
                                 data->VideoData,
                                 msg->x,
                                 msg->y,
                                 msg->width,
                                 msg->height,
                                 msg->modulo,
                                 data->bytesperline,
                                 msg->pixlut);
            break;

        case 3:
            HIDD_BM_CopyLUTMemBox24(o,
                                 msg->pixels,
                                 0,
                                 0,
                                 data->VideoData,
                                 msg->x,
                                 msg->y,
                                 msg->width,
                                 msg->height,
                                 msg->modulo,
                                 data->bytesperline,
                                 msg->pixlut);
            break;

        case 4:
            HIDD_BM_CopyLUTMemBox32(o,
                                    msg->pixels,
                                    0,
                                    0,
                                    data->VideoData,
                                    msg->x,
                                    msg->y,
                                    msg->width,
                                    msg->height,
                                    msg->modulo,
                                    data->bytesperline,
                                    msg->pixlut);
            break;

        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
    } /* switch(data->bytesperpix) */	 

    UNLOCK_BITMAP(data)   
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__GetImageLUT(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_BitMap_GetImageLUT *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    HIDDT_Pixel fg = GC_FG(msg->gc);
    struct p96gfx_staticdata *csd = CSD(cl);
    BOOL v = FALSE;

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW
#if 0    
    maybeputinvram(cid, data);
#endif

    if (data->invram) {
        struct RenderInfo ri;

        P96GFXRTG__MakeRenderInfo(csd, cid, &ri, data);

        LOCK_HW

        if (mode == vHidd_GC_DrawMode_Clear || mode == vHidd_GC_DrawMode_Set) {
            ULONG pen = mode == vHidd_GC_DrawMode_Clear ? 0x00000000 : 0xffffffff;
            v = FillRect(cid, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, pen, 0xff, data->rgbformat);
        } else if (mode == vHidd_GC_DrawMode_Copy) {
            v = FillRect(cid, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, fg, 0xff, data->rgbformat);
        } else if (mode == vHidd_GC_DrawMode_Invert) {
            v = InvertRect(cid, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, 0xff, data->rgbformat);
        }

        UNLOCK_HW
    }

    if (!v) switch(mode)
    {
        case vHidd_GC_DrawMode_Copy:
            switch(data->bytesperpixel)
            {
                case 1:
                    HIDD_BM_FillMemRect8(o,
                                         data->VideoData,
                                         msg->minX,
                                         msg->minY,
                                         msg->maxX,
                                         msg->maxY,
                                         data->bytesperline,
                                         fg);
                    break;

                case 2:
                    HIDD_BM_FillMemRect16(o,
                                         data->VideoData,
                                         msg->minX,
                                         msg->minY,
                                         msg->maxX,
                                         msg->maxY,
                                         data->bytesperline,
                                         fg);
                    break;

                case 3:
                    HIDD_BM_FillMemRect24(o,
                                         data->VideoData,
                                         msg->minX,
                                         msg->minY,
                                         msg->maxX,
                                         msg->maxY,
                                         data->bytesperline,
                                         fg);
                    break;

                case 4:
                    HIDD_BM_FillMemRect32(o,
                                         data->VideoData,
                                         msg->minX,
                                         msg->minY,
                                         msg->maxX,
                                         msg->maxY,
                                         data->bytesperline,
                                         fg);
                    break;
            }
            break;

        case vHidd_GC_DrawMode_Invert:
            HIDD_BM_InvertMemRect(o,
                                 data->VideoData,
                                 msg->minX * data->bytesperpixel,
                                 msg->minY,
                                 msg->maxX * data->bytesperpixel + data->bytesperpixel - 1,
                                 msg->maxY,
                                 data->bytesperline);
            break;

        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
    } /* switch(mode) */

    UNLOCK_BITMAP(data)
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__PutPattern(OOP_Class *cl, OOP_Object *o,
                                 struct pHidd_BitMap_PutPattern *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    HIDDT_Pixel	fg = GC_FG(msg->gc);
    HIDDT_Pixel bg = GC_BG(msg->gc);
    struct Pattern pat;
    UBYTE drawmode;
    BOOL v = FALSE;

    D(bug("[P96Gfx:Bitmap] %s(%d,%d)(%d,%d)(%x,%d,%d,%d,%d,%d)\n", __func__,
        msg->x, msg->y, msg->width, msg->height,
        msg->pattern, msg->patternsrcx, msg->patternsrcy, fg, bg, msg->patternheight));

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    if ((msg->mask == NULL) && (msg->patterndepth == 1))
    {
        switch (msg->patternheight)
        {
            case 1:
            pat.Size = 0;
            break;
            case 2:
            pat.Size = 1;
            break;
            case 4:
            pat.Size = 2;
            break;
            case 8:
            pat.Size = 3;
            break;
            case 16:
            pat.Size = 4;
            break;
            case 32:
            pat.Size = 5;
            break;
            case 64:
            pat.Size = 6;
            break;
            case 128:
            pat.Size = 7;
            break;
            case 256:
            pat.Size = 8;
            break;
            default:
            pat.Size = 0xff;
        }

        if (pat.Size <= 8)
        {
#if 0        
            maybeputinvram(cid, data);
#endif
            if (data->invram)
            {
                struct RenderInfo ri;

                P96GFXRTG__MakeRenderInfo(csd, cid, &ri, data);

                if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
                     drawmode = JAM1;
                else if (GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Invert)
                     drawmode = COMPLEMENT;
                else
                    drawmode = JAM2;
                if (msg->invertpattern)
                     drawmode |= INVERSVID;

                pat.Memory = msg->pattern;
                pat.XOffset = msg->patternsrcx;
                pat.YOffset = msg->patternsrcy;
                pat.FgPen = fg;
                pat.BgPen = bg;
                pat.DrawMode = drawmode;

                LOCK_HW

                v = BlitPattern(cid, &ri, &pat, msg->x, msg->y, msg->width, msg->height, 0xff, data->rgbformat);

                UNLOCK_HW
            }
        }
    }

    if (!v) switch(data->bytesperpixel)
    {
        case 1:
            HIDD_BM_PutMemPattern8(o,
                                   msg->gc,
                                   msg->pattern,
                                   msg->patternsrcx,
                                   msg->patternsrcy,
                                   msg->patternheight,
                                   msg->patterndepth,
                                   msg->patternlut,
                                   msg->invertpattern,
                                   msg->mask,
                                   msg->maskmodulo,
                                   msg->masksrcx,
                                   data->VideoData,
                                   data->bytesperline,
                                   msg->x,
                                   msg->y,
                                   msg->width,
                                   msg->height);
            break;

        case 2:
            HIDD_BM_PutMemPattern16(o,
                                    msg->gc,
                                    msg->pattern,
                                    msg->patternsrcx,
                                    msg->patternsrcy,
                                    msg->patternheight,
                                    msg->patterndepth,
                                    msg->patternlut,
                                    msg->invertpattern,
                                    msg->mask,
                                    msg->maskmodulo,
                                    msg->masksrcx,
                                    data->VideoData,
                                    data->bytesperline,
                                    msg->x,
                                    msg->y,
                                    msg->width,
                                    msg->height);
            break;

        case 3:
            HIDD_BM_PutMemPattern24(o,
                                    msg->gc,
                                    msg->pattern,
                                    msg->patternsrcx,
                                    msg->patternsrcy,
                                    msg->patternheight,
                                    msg->patterndepth,
                                    msg->patternlut,
                                    msg->invertpattern,
                                    msg->mask,
                                    msg->maskmodulo,
                                    msg->masksrcx,
                                    data->VideoData,
                                    data->bytesperline,
                                    msg->x,
                                    msg->y,
                                    msg->width,
                                    msg->height);
            break;

        case 4:
            HIDD_BM_PutMemPattern32(o,
                                    msg->gc,
                                    msg->pattern,
                                    msg->patternsrcx,
                                    msg->patternsrcy,
                                    msg->patternheight,
                                    msg->patterndepth,
                                    msg->patternlut,
                                    msg->invertpattern,
                                    msg->mask,
                                    msg->maskmodulo,
                                    msg->masksrcx,
                                    data->VideoData,
                                    data->bytesperline,
                                    msg->x,
                                    msg->y,
                                    msg->width,
                                    msg->height);
            break;

        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
    } /* switch(data->bytesperpixel) */

    UNLOCK_BITMAP(data)
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);
    HIDDT_Pixel	fg = GC_FG(msg->gc);
    HIDDT_Pixel bg = GC_BG(msg->gc);
    BOOL v = FALSE;

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

#if 0
    maybeputinvram(cid, data);
#endif

    if (data->invram) {
        struct Template tmpl;
        struct RenderInfo ri;
        UBYTE drawmode;

        P96GFXRTG__MakeRenderInfo(csd, cid, &ri, data);

        if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
             drawmode = JAM1;
        else if (GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Invert)
             drawmode = COMPLEMENT;
        else
            drawmode = JAM2;
        if (msg->inverttemplate)
             drawmode |= INVERSVID;

        /* tmpl.XOffset has only UBYTE size so we must fix params up [1] [2] */

        tmpl.Memory = msg->masktemplate + ((msg->srcx / 8) & ~1); /* [1] */
        tmpl.BytesPerRow = msg->modulo;
        tmpl.XOffset = msg->srcx & 0XF; /* [2] */
        tmpl.DrawMode = drawmode;
        tmpl.FgPen = fg;
        tmpl.BgPen = bg;

        LOCK_HW
        v = BlitTemplate(cid, &ri, &tmpl, msg->x, msg->y, msg->width, msg->height, 0xff, data->rgbformat);
        UNLOCK_HW
    }

    if (!v) switch(data->bytesperpixel)
    {
        case 1:
            HIDD_BM_PutMemTemplate8(o,
                                    msg->gc,
                                    msg->masktemplate,
                                    msg->modulo,
                                    msg->srcx,
                                    data->VideoData,
                                    data->bytesperline,
                                    msg->x,
                                    msg->y,
                                    msg->width,
                                    msg->height,
                                    msg->inverttemplate);
            break;

        case 2:
            HIDD_BM_PutMemTemplate16(o,
                                     msg->gc,
                                     msg->masktemplate,
                                     msg->modulo,
                                     msg->srcx,
                                     data->VideoData,
                                     data->bytesperline,
                                     msg->x,
                                     msg->y,
                                     msg->width,
                                     msg->height,
                                     msg->inverttemplate);
            break;

        case 3:
            HIDD_BM_PutMemTemplate24(o,
                                     msg->gc,
                                     msg->masktemplate,
                                     msg->modulo,
                                     msg->srcx,
                                     data->VideoData,
                                     data->bytesperline,
                                     msg->x,
                                     msg->y,
                                     msg->width,
                                     msg->height,
                                     msg->inverttemplate);
            break;

        case 4:
            HIDD_BM_PutMemTemplate32(o,
                                     msg->gc,
                                     msg->masktemplate,
                                     msg->modulo,
                                     msg->srcx,
                                     data->VideoData,
                                     data->bytesperline,
                                     msg->x,
                                     msg->y,
                                     msg->width,
                                     msg->height,
                                     msg->inverttemplate);
            break;

        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
    } /* switch(data->bytesperpixel) */

    UNLOCK_BITMAP(data)
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL P96GFXBitmap__Hidd_PlanarBM__SetBitMap(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_PlanarBM_SetBitMap *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL P96GFXBitmap__Hidd_PlanarBM__GetBitMap(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_PlanarBM_GetBitMap *msg)
{
    struct P96GfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct p96gfx_carddata *cid = data->gfxCardData;
    struct p96gfx_staticdata *csd = CSD(cl);

    D(bug("[P96Gfx:Bitmap] %s()\n", __func__));

    LOCK_HW
    WaitBlitter(cid);
    UNLOCK_HW

    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
