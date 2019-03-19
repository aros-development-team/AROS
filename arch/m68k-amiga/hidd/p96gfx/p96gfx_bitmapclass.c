/*
    Copyright  1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for native Amiga chipset.
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
#include <oop/oop.h>
#include <hidd/gfx.h>
#include <aros/symbolsets.h>

#define DEBUG 0
#define DB2(x) ;
#define DEBUG_TEXT(x)
#define DVRAM(x) ;
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "p96gfx_intern.h"
#include "p96gfx_bitmap.h"
#include "p96_rtg.h"

/* LOCK_BITMAP_MULTI:

  swap bitmap between RAM <-> VRAM allowed only if this lock is held
 
  the bitmap being swapped must also get LOCK_BITMAP lock, otherwise
  gfx functions (done by other tasks) could render into wrong place while
  bitmap is moved around.

*/
  
static APTR allocrtgvrambitmap(struct p96gfx_staticdata *csd, struct bm_data *bm)
{
    APTR vmem;
    SetMemoryMode(csd, RGBFB_CLUT);
    vmem = Allocate(csd->vmem, bm->memsize);
    SetMemoryMode(csd, bm->rgbformat);
    if (vmem)
    {
        DVRAM(bug("BM %p (%dx%dx%d %d): %p,%d bytes VRAM allocated.\n", bm, bm->width, bm->height, bm->bytesperpixel*8, bm->bytesperline, vmem, bm->memsize));
    }
    return vmem;
}

static void freertgbitmap(struct p96gfx_staticdata *csd, struct bm_data *bm)
{
    DVRAM(bug("BM %p: freeing %p:%d from %s\n", bm, bm->VideoData, bm->memsize, bm->invram ? "VRAM" : "RAM"));
    if (bm->invram)
    {
        SetMemoryMode(csd, RGBFB_CLUT);
        Deallocate(csd->vmem, bm->VideoData, bm->memsize);
        SetMemoryMode(csd, bm->rgbformat);
        csd->vram_used -= bm->memsize;
    }
    else if (bm->VideoData)
    {
        FreeMem(bm->VideoData, bm->memsize);
        csd->fram_used -= bm->memsize;
    }
    bm->VideoData = NULL;
    bm->invram = FALSE;
}	

static BOOL movebitmaptofram(struct p96gfx_staticdata *csd, struct bm_data *bm)
{
    BOOL ok = FALSE;
    APTR vmem;

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
            SetMemoryMode(csd, bm->rgbformat);
            CopyMemQuick(bm->VideoData, vmem, bm->memsize);
            freertgbitmap(csd, bm);
            bm->VideoData = vmem;
            bm->invram = FALSE;
            csd->fram_used += bm->memsize;
            ok = TRUE;
       }
       DVRAM(bug("BM %p: %d x %d moved to RAM %p:%d. VRAM=%d\n", bm, bm->width, bm->height, bm->VideoData, bm->memsize, csd->vram_used));

       UNLOCK_BITMAP(bm)
    }

   return ok;
}

static BOOL allocrtgbitmap(struct p96gfx_staticdata *csd, struct bm_data *bm, BOOL usevram)
{
    bm->memsize = (bm->bytesperline * bm->height + 7) & ~7;

    if (!(bm->VideoData = allocrtgvrambitmap(csd, bm)))
    {
        if (usevram && bm->memsize < csd->vram_size)
        {
             struct bm_data *bmnode;
             ForeachNode(&csd->bitmaplist, bmnode)
             {
                if (bmnode != bm && bmnode->invram && !bmnode->locked)
                {
                    if (movebitmaptofram(csd, bmnode))
                    {
                        if ((bm->VideoData = allocrtgvrambitmap(csd, bm)))
                        {
                            csd->vram_used += bm->memsize;
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
                csd->fram_used += bm->memsize;
            }
        }
    }
    else
    {
        csd->vram_used += bm->memsize;
        bm->invram = TRUE;
    }
    DVRAM(bug("BM %p: %p,%d bytes allocated from %s. VRAM=%d\n", bm, bm->VideoData, bm->memsize, bm->invram ? "VRAM" : "RAM", csd->vram_used));

    //if (bm->VideoData != NULL)
    //{
    //    int i;
    //
    //    for(i = 0; i < bm->memsize; i++) bm->VideoData[i] = 0xFF;
    //}

    return bm->VideoData != NULL;
}

static BOOL movethisbitmaptovram(struct p96gfx_staticdata *csd, struct bm_data *bm)
{
    APTR vmem = allocrtgvrambitmap(csd, bm);
    if (vmem)
    {
        SetMemoryMode(csd, bm->rgbformat);
        CopyMemQuick(bm->VideoData, vmem, bm->memsize);
        freertgbitmap(csd, bm);
        bm->VideoData = vmem;
        bm->invram = TRUE;
        csd->vram_used += bm->memsize;
        DVRAM(bug("BM %p: %p:%d (%d x %d) moved back to VRAM\n", bm, bm->VideoData, bm->memsize, bm->width, bm->height));
        return TRUE;
    }
    return FALSE;
}

static BOOL movebitmaptovram(struct p96gfx_staticdata *csd, struct bm_data *bm)
{
    struct bm_data *bmnode;

    if (bm->invram)
        return TRUE;

    DVRAM(bug("BM %p: %p,%d needs to be in VRAM...\n", bm, bm->VideoData, bm->memsize));

    ForeachNode(&csd->bitmaplist, bmnode)
    {
        if (bmnode != bm && bmnode->invram && !bmnode->locked)
        {
            if (movebitmaptofram(csd, bmnode))
            {
                if (movethisbitmaptovram(csd, bm))
                {
                    return TRUE;
                }
            }
        }
    }

    DVRAM(bug("-> not enough memory, VRAM=%d\n", csd->vram_used));

    return FALSE;
}

#if 0
static BOOL maybeputinvram(struct p96gfx_staticdata *csd, struct bm_data *bm)
{
    if (bm->invram)
        return TRUE;
    if (bm->memsize >= csd->vram_size - csd->vram_used)
        return FALSE;
    return movethisbitmaptovram(csd, bm);
}
#endif

static void hidescreen(struct p96gfx_staticdata *csd, struct bm_data *bm)
{
    D(bug("Hide %p: (%p:%d)\n",
        bm, bm->VideoData, bm->memsize));
    SetInterrupt(csd, FALSE);
    SetDisplay(csd, FALSE);
    SetSwitch(csd, FALSE);
    csd->dmodeid = 0;
    bm->locked--;
    csd->disp = NULL;
}

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

/****************************************************************************************/

OOP_Object *P96GFXBitmap__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    BOOL  ok = TRUE;      
    struct bm_data *data;
    IPTR 	    	     width, height, multi;
    IPTR		     displayable;
    struct TagItem tags[2];

    DB2(bug("P96GFXBitmap__Root__New\n"));

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

    data->rgbformat = getrtgformat(csd, data->pixfmtobj);
    data->align = displayable ? 32 : 16;
    width = (width + data->align - 1) & ~(data->align - 1);
    data->bytesperline = CalculateBytesPerRow(csd, width, data->rgbformat);
    data->width = width;
    data->height = height;
    data->bytesperpixel = multi;

    LOCK_MULTI_BITMAP

    LOCK_HW /* alloc routines call SetMemoryMode */
    WaitBlitter(csd); /* in case bitmaps are swapped between RAM <-> VRAM during allocation */
    allocrtgbitmap(csd, data, TRUE);
    UNLOCK_HW

    AddTail(&csd->bitmaplist, (struct Node*)&data->node);

    UNLOCK_MULTI_BITMAP

    tags[0].ti_Tag = aHidd_BitMap_BytesPerRow;
    tags[0].ti_Data = data->bytesperline;
    tags[1].ti_Tag = TAG_DONE;
    OOP_SetAttrs(o, tags);

    DB2(bug("%dx%dx%d %d RGBF=%08x P=%08x\n", width, height, multi, data->bytesperline, data->rgbformat, data->VideoData));

    if (data->VideoData == NULL)
        ok = FALSE;

    if (!ok) {
        OOP_MethodID dispose_mid;
        dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
        o = NULL;
    }

    DB2(bug("ret=%x bm=%p (%p:%d)\n", o, data, data->VideoData, data->memsize));

    return o;
}

VOID P96GFXBitmap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data    *data;

    data = OOP_INST_DATA(cl, o);

    LOCK_HW
    WaitBlitter(csd);

    DB2(bug("P96GFXBitmap__Root__Dispose %x bm=%x (%p,%d)\n", o, data, data->VideoData, data->memsize));
    if (csd->disp == data)
        hidescreen(csd, data);

    UNLOCK_HW

    FreeVec(data->palette);

    LOCK_MULTI_BITMAP

    LOCK_HW /* free functions call SetMemoryMode */
    freertgbitmap(csd, data);
    UNLOCK_HW

    Remove((struct Node*)&data->node);

    UNLOCK_MULTI_BITMAP

    OOP_DoSuperMethod(cl, o, msg);
}

VOID P96GFXBitmap__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    BOOL moved = FALSE;

    DB2(bug("P96GFXBitmap__Root__Set %p (%p:%d)\n", data, data->VideoData, data->memsize));
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        DB2(bug("%d/%d\n", tag->ti_Tag, tag->ti_Data));
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            DB2(bug("->%d\n", idx));
            switch(idx)
            {
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
                    data->rgbformat = getrtgformat(csd, pf);
                    modeinfo = getrtgmodeinfo(csd, sync, pf, csd->fakemodeinfo);
                    csd->modeinfo = modeinfo;
                    csd->rgbformat = data->rgbformat;
                    pw(csd->bitmapextra + PSSO_BitMapExtra_Width, width);
                    pw(csd->bitmapextra + PSSO_BitMapExtra_Height, height);
                    D(bug("Show %p: (%p:%d) %dx%dx%d (%dx%d) BF=%08x\n",
                        data, data->VideoData, data->memsize,
                        dwidth, dheight, depth, width, height, data->rgbformat));

                    if (!data->invram)
                    {
                        WaitBlitter(csd); /* in case other bitmaps are swapped from VRAM to RAM */
                        if (!movebitmaptovram(csd, data))
                        {
                            struct bm_data *bmnode;

                            /* Second try. Now lock all bitmaps first. UNLOCK_HW first, to ensure
                               correct locking order: multibm -> bm -> hw */

                            UNLOCK_HW

                            ForeachNode(&csd->bitmaplist, bmnode)
                            {
                                if (bmnode != data) LOCK_BITMAP(bmnode)
                            }

                            LOCK_HW                            
                            WaitBlitter(csd); /* in case other bitmaps are swapped from VRAM to RAM */
                            movebitmaptovram(csd, data); /* shouldn't fail this time. If it does we are screwed ... */                          
                            UNLOCK_HW

                            ForeachNode(&csd->bitmaplist, bmnode)
                            {
                                if (bmnode != data) UNLOCK_BITMAP(bmnode)
                            }                            

                            LOCK_HW
                        }
                    }

                    csd->dwidth = dwidth;
                    csd->dheight = dheight;
                    csd->dmodeid = modeid;

                    if (csd->hardwaresprite && depth <= 8) {
                        UWORD i;
                        UBYTE *clut = csd->boardinfo + PSSO_BoardInfo_CLUT;
                        for (i = csd->spritecolors + 1; i < csd->spritecolors + 4; i++)
                            SetSpriteColor(csd, i - (csd->spritecolors + 1),  clut[i * 3 + 0],  clut[i * 3 + 1],  clut[i * 3 + 2]);
                    }
                    SetInterrupt(csd, FALSE);
                    SetColorArray(csd, 0, 256);
                    SetDisplay(csd, FALSE);
                    SetGC(csd, modeinfo, 0);
                    SetClock(csd);
                    SetDAC(csd);
                    SetPanning(csd, data->VideoData, width, 0, 0);
                    SetDisplay(csd, TRUE);
                    SetSwitch(csd, TRUE);
                    SetInterrupt(csd, TRUE);
                    csd->disp = data;
                    csd->disp->locked++;
                } else {
                    hidescreen(csd, data);
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
    DB2(bug("P96GFXBitmap__Root__Set Exit\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
#if 0
    if (moved && csd->disp == data)
        setscroll(csd, data);
#else
    (void)moved;
#endif
}

VOID P96GFXBitmap__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
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
            *msg->storage = csd->disp == data;
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
    D(bug("P96GFXBitmap_Init\n"));
    return TRUE; //return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int P96GFXBitmap_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("P96GFXBitmap_Expunge\n"));
    //OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(P96GFXBitmap_Init, 0);
ADD2EXPUNGELIB(P96GFXBitmap_Expunge, 0);

BOOL P96GFXBitmap__Hidd_BitMap__ObtainDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);

    LOCK_BITMAP(data)

#if 0
    if (!data->invram) {
        if (!movebitmaptovram(csd, data))
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
    WaitBlitter(csd);
    UNLOCK_HW

    return TRUE;
}

VOID P96GFXBitmap__Hidd_BitMap__ReleaseDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    data->locked--;

    UNLOCK_BITMAP(data)
}

BOOL P96GFXBitmap__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    WORD i, j;
    UBYTE *clut;

    D(bug("[P96Gfx] %()\n", __func__));

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
        return FALSE;

    LOCK_HW

    WaitBlitter(csd);
    clut = csd->boardinfo + PSSO_BoardInfo_CLUT;
    D(bug("[P96Gfx] %s: clut @ %p\n", __func__, clut));

    for (i = msg->firstColor, j = 0; j < msg->numColors; i++, j++) {
        clut[i * 3 + 0] = msg->colors[j].red >> 8;
        clut[i * 3 + 1] = msg->colors[j].green >> 8;
        clut[i * 3 + 2] = msg->colors[j].blue >> 8;
        D(bug("[P96Gfx] %s: color %d %02x%02x%02x\n", __func__, i, msg->colors[j].red >> 8, msg->colors[j].green >> 8, msg->colors[j].blue >> 8));
    }
    SetColorArray(csd, msg->firstColor, msg->numColors);

    UNLOCK_HW

    return TRUE;
}

VOID P96GFXBitmap__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_PutPixel *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    ULONG   	       offset;
    HIDDT_Pixel       pixel = msg->pixel;
    UBYTE   	      *mem;

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(csd);
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
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data 	*data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	 pixel = 0;
    ULONG   	    	 offset;
    UBYTE   	    	*mem;

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(csd);
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
#if (0)
    struct bm_data *data = OOP_INST_DATA(cl, o);
#endif
    struct p96gfx_staticdata *csd = CSD(cl);
    BOOL v = FALSE;

    LOCK_HW
    WaitBlitter(csd);
#if (0)
    if (data->invram) {
        struct RenderInfo ri;
        struct Line renderLine;
        makerenderinfo(csd, &ri, data);

        v = DrawLine(csd, &ri, &renderLine, data->rgbformat);
    }
#endif
    UNLOCK_HW

    if (!v) OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(csd);
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
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(csd);
    UNLOCK_HW

    switch(msg->pixFmt)
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
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct p96gfx_staticdata *csd = CSD(cl);

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(csd);
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
    struct p96gfx_staticdata *csd = CSD(cl);

    LOCK_HW
    WaitBlitter(csd);
    UNLOCK_HW

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID P96GFXBitmap__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    HIDDT_Pixel fg = GC_FG(msg->gc);
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    BOOL v = FALSE;

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(csd);
    UNLOCK_HW
#if 0    
    maybeputinvram(csd, data);
#endif

    if (data->invram) {
        struct RenderInfo ri;

        makerenderinfo(csd, &ri, data);

        LOCK_HW

        if (mode == vHidd_GC_DrawMode_Clear || mode == vHidd_GC_DrawMode_Set) {
            ULONG pen = mode == vHidd_GC_DrawMode_Clear ? 0x00000000 : 0xffffffff;
            v = FillRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, pen, 0xff, data->rgbformat);
        } else if (mode == vHidd_GC_DrawMode_Copy) {
            v = FillRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, fg, 0xff, data->rgbformat);
        } else if (mode == vHidd_GC_DrawMode_Invert) {
            v = InvertRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, 0xff, data->rgbformat);
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
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel	fg = GC_FG(msg->gc);
    HIDDT_Pixel bg = GC_BG(msg->gc);
    struct Pattern pat;
    UBYTE drawmode;
    BOOL v = FALSE;

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(csd);
    UNLOCK_HW

    DB2(bug("blitpattern(%d,%d)(%d,%d)(%x,%d,%d,%d,%d,%d)\n",
        msg->x, msg->y, msg->width, msg->height,
        msg->pattern, msg->patternsrcx, msg->patternsrcy, fg, bg, msg->patternheight));

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
            maybeputinvram(csd, data);
#endif
            if (data->invram)
            {
                struct RenderInfo ri;

                makerenderinfo(csd, &ri, data);

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

                v = BlitPattern(csd, &ri, &pat, msg->x, msg->y, msg->width, msg->height, 0xff, data->rgbformat);

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
    struct p96gfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel	fg = GC_FG(msg->gc);
    HIDDT_Pixel bg = GC_BG(msg->gc);
    BOOL v = FALSE;

    LOCK_BITMAP(data)

    LOCK_HW
    WaitBlitter(csd);
    UNLOCK_HW

#if 0
    maybeputinvram(csd, data);
#endif

    if (data->invram) {
        struct Template tmpl;
        struct RenderInfo ri;
        UBYTE drawmode;

        makerenderinfo(csd, &ri, data);

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
        v = BlitTemplate(csd, &ri, &tmpl, msg->x, msg->y, msg->width, msg->height, 0xff, data->rgbformat);
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
    struct p96gfx_staticdata *csd = CSD(cl);

    LOCK_HW
    WaitBlitter(csd);
    UNLOCK_HW

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL P96GFXBitmap__Hidd_PlanarBM__SetBitMap(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_PlanarBM_SetBitMap *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);

    LOCK_HW
    WaitBlitter(csd);
    UNLOCK_HW

    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL P96GFXBitmap__Hidd_PlanarBM__GetBitMap(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_PlanarBM_GetBitMap *msg)
{
    struct p96gfx_staticdata *csd = CSD(cl);

    LOCK_HW
    WaitBlitter(csd);
    UNLOCK_HW

    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
