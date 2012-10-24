/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
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
#include <hidd/graphics.h>
#include <aros/symbolsets.h>

#define DEBUG 0
#define DB2(x) ;
#define DEBUG_TEXT(x)
#define DVRAM(x) ;
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "uaegfx.h"
#include "uaegfxbitmap.h"
#include "uaertg.h"

static APTR allocrtgvrambitmap(struct uaegfx_staticdata *csd, struct bm_data *bm)
{
    APTR vmem;
    SetMemoryMode(csd, RGBFB_CLUT);
    vmem = Allocate(csd->vmem, bm->memsize);
    SetMemoryMode(csd, bm->rgbformat);
    DVRAM(bug("BM %p (%dx%dx%d %d): %p,%d VRAM allocated.\n", bm, bm->width, bm->height, bm->bytesperpixel, bm->bytesperline, vmem, bm->memsize));
    return vmem;
}

static void freertgbitmap(struct uaegfx_staticdata *csd, struct bm_data *bm)
{
    DVRAM(bug("BM %p: freeing %p:%d from %s\n", bm, bm->VideoData, bm->memsize, bm->invram ? "VRAM" : "RAM"));
    if (bm->invram) {
	SetMemoryMode(csd, RGBFB_CLUT);
	Deallocate(csd->vmem, bm->VideoData, bm->memsize);
	SetMemoryMode(csd, bm->rgbformat);
	csd->vram_used -= bm->memsize;
    } else if (bm->VideoData) {
    	FreeMem(bm->VideoData, bm->memsize);
    	csd->fram_used -= bm->memsize;
    }
    bm->VideoData = NULL;
    bm->invram = FALSE;
}	

static BOOL movebitmaptofram(struct uaegfx_staticdata *csd, struct bm_data *bm)
{
    BOOL ok = FALSE;
    APTR vmem;

    vmem = AllocMem(bm->memsize, MEMF_ANY);
    if (vmem) {
	SetMemoryMode(csd, bm->rgbformat);
	CopyMemQuick(bm->VideoData, vmem, bm->memsize);
	freertgbitmap(csd, bm);
	bm->VideoData = vmem;
	csd->fram_used += bm->memsize;
	ok = TRUE;
   }
   DVRAM(bug("BM %p: moved to RAM %p:%d. VRAM=%d\n", bm, bm->VideoData, bm->memsize, csd->vram_used));
   return ok;
}

static BOOL allocrtgbitmap(struct uaegfx_staticdata *csd, struct bm_data *bm, BOOL usevram)
{
    bm->memsize = (bm->bytesperline * bm->height + 7) & ~7;
    if (!(bm->VideoData = allocrtgvrambitmap(csd, bm))) {
    	if (usevram && bm->memsize < csd->vram_size) {
    	     struct bm_data *bmnode;
	     ForeachNode(&csd->bitmaplist, bmnode) {
		if (bmnode != bm && bmnode->invram && !bmnode->locked) {
		    if (movebitmaptofram(csd, bmnode)) {
			if ((bm->VideoData = allocrtgvrambitmap(csd, bm))) {
			    csd->vram_used += bm->memsize;
			    bm->invram = TRUE;
			    break;
			}
		    }
    		}
    	     }
	}
	if (!bm->VideoData) {
	    bm->VideoData = AllocMem(bm->memsize, MEMF_ANY);
	    if (bm->VideoData)
		csd->fram_used += bm->memsize;
	}
    } else {
	csd->vram_used += bm->memsize;
	bm->invram = TRUE;
    }
    DVRAM(bug("BM %p: %p,%d bytes allocated from %s. VRAM=%d\n", bm, bm->VideoData, bm->memsize, bm->invram ? "VRAM" : "RAM", csd->vram_used));
    return bm->VideoData != NULL;
}

static BOOL movethisbitmaptovram(struct uaegfx_staticdata *csd, struct bm_data *bm)
{
    APTR vmem = allocrtgvrambitmap(csd, bm);
    if (vmem) {
	SetMemoryMode(csd, bm->rgbformat);
	CopyMemQuick(bm->VideoData, vmem, bm->memsize);
	freertgbitmap(csd, bm);
	bm->VideoData = vmem;
	bm->invram = TRUE;
	csd->vram_used += bm->memsize;
	DVRAM(bug("BM %p: %p:%d moved back to VRAM\n", bm, bm->VideoData, bm->memsize));
	return TRUE;
    }
    return FALSE;
}

static BOOL movebitmaptovram(struct uaegfx_staticdata *csd, struct bm_data *bm)
{
     struct bm_data *bmnode;
 
     if (bm->invram)
	return TRUE;
     DVRAM(bug("BM %p: %p,%d needs to be in VRAM...\n", bm, bm->VideoData, bm->memsize));
     ForeachNode(&csd->bitmaplist, bmnode) {
	if (bmnode != bm && bmnode->invram && !bmnode->locked) {
	    if (movebitmaptofram(csd, bmnode)) {
	    	if (movethisbitmaptovram(csd, bm)) {
		    return TRUE;
		}
	    }
	}
     }
     DVRAM(bug("-> not enough memory, VRAM=%d\n", csd->vram_used));
     return FALSE;
}

static BOOL maybeputinvram(struct uaegfx_staticdata *csd, struct bm_data *bm)
{
    if (bm->invram)
	return TRUE;
    if (bm->memsize >= csd->vram_size - csd->vram_used)
	return FALSE;
    return movethisbitmaptovram(csd, bm);
}

static void hidescreen(struct uaegfx_staticdata *csd, struct bm_data *bm)
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

OOP_Object *UAEGFXBitmap__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    BOOL  ok = TRUE;      
    struct bm_data *data;
    IPTR 	    	     width, height, multi;
    IPTR		     displayable;
    HIDDT_ModeID 	     modeid;
    struct TagItem tags[2];

    DB2(bug("UAEGFXBitmap__Root__New\n"));

    o =(OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;
	
    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof  (*data));

    WaitBlitter(csd);

    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
    OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
    OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&data->gfxhidd);
    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&data->pixfmtobj);
    OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_BytesPerPixel, &multi);
 
    data->rgbformat = getrtgformat(csd, data->pixfmtobj);
    data->width = width;
    data->align = displayable ? 32 : 16;
   	width = (width + data->align - 1) & ~(data->align - 1);
    width = CalculateBytesPerRow(csd, width, data->rgbformat);
    data->bytesperline = width;
    data->height = height;
    data->bytesperpixel = multi;
    allocrtgbitmap(csd, data, TRUE);
    AddTail(&csd->bitmaplist, (struct Node*)&data->node);
 
    tags[0].ti_Tag = aHidd_BitMap_BytesPerRow;
    tags[0].ti_Data = data->bytesperline;
    tags[1].ti_Tag = TAG_DONE;
    OOP_SetAttrs(o, tags);

    DB2(bug("%dx%dx%d %d RGBF=%08x P=%08x\n", data->width, height, multi, width, data->rgbformat, data->VideoData));

    if (data->VideoData == NULL)
    	ok = FALSE;

    OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
    if (ok && modeid != vHidd_ModeID_Invalid) {
	OOP_Object *sync, *pf;
	IPTR dwidth, dheight;

	HIDD_Gfx_GetMode(data->gfxhidd, modeid, &sync, &pf);
	OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
	OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);
	data->disp_width  = dwidth;
	data->disp_height = dheight;
    }

    if (!ok) {
 	OOP_MethodID dispose_mid;
	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
	o = NULL;
    }
    
    DB2(bug("ret=%x bm=%p (%p:%d)\n", o, data, data->VideoData, data->memsize));
  	
    return o;
}

VOID UAEGFXBitmap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data    *data;
    
    data = OOP_INST_DATA(cl, o);
    WaitBlitter(csd);
    
    DB2(bug("UAEGFXBitmap__Root__Dispose %x bm=%x (%p,%d)\n", o, data, data->VideoData, data->memsize));
    if (csd->disp == data)
    	hidescreen(csd, data);

    FreeVec(data->palette);
    freertgbitmap(csd, data);
    Remove((struct Node*)&data->node);
    
    OOP_DoSuperMethod(cl, o, msg);
}

VOID UAEGFXBitmap__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    BOOL moved = FALSE;

    DB2(bug("UAEGFXBitmap__Root__Set %p (%p:%d)\n", data, data->VideoData, data->memsize));
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
	        if (tag->ti_Data) {
	     	    OOP_Object *gfxhidd, *sync, *pf;
    		    IPTR modeid = vHidd_ModeID_Invalid;
    		    IPTR dwidth, dheight, depth, width, height;
    		    struct ModeInfo *modeinfo;

		    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
		    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
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
		    /* PSSO_BitMapExtra_Width needs to be aligned */
		    pw(csd->bitmapextra + PSSO_BitMapExtra_Width, (width + data->align - 1) & ~(data->align - 1));
		    pw(csd->bitmapextra + PSSO_BitMapExtra_Height, height);
		    D(bug("Show %p: (%p:%d) %dx%dx%d (%dx%d) BF=%08x\n",
			data, data->VideoData, data->memsize,
			dwidth, dheight, depth, width, height, data->rgbformat));

		    if (!data->invram)
		    	movebitmaptovram(csd, data);

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
		    SetPanning(csd, data->VideoData, dwidth, 0, 0);
		    SetDisplay(csd, TRUE);
		    SetSwitch(csd, TRUE);
	    	    SetInterrupt(csd, TRUE);
	            csd->disp = data;
	            csd->disp->locked++;
		} else {
		    hidescreen(csd, data);
		}
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
    DB2(bug("UAEGFXBitmap__Root__Set Exit\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
#if 0
    if (moved && csd->disp == data)
    	setscroll(csd, data);
#else
    (void)moved;
#endif
}

VOID UAEGFXBitmap__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    //DB2(bug("UAEGFXBitmap__Root__Get\n"));
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
    //DB2(bug("UAEGFXBitmap__Root__Get Exit\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

static int UAEGFXBitmap_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("UAEGFXBitmap_Init\n"));
    return TRUE; //return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int UAEGFXBitmap_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("UAEGFXBitmap_Expunge\n"));
    //OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(UAEGFXBitmap_Init, 0);
ADD2EXPUNGELIB(UAEGFXBitmap_Expunge, 0);

BOOL UAEGFXBitmap__Hidd_BitMap__ObtainDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);

    if (!data->invram) {
	if (!movebitmaptovram(csd, data))
	    return FALSE;
    }

    *msg->addressReturn = data->VideoData;
    *msg->widthReturn = data->width;
    *msg->heightReturn = data->height;
    /* undocumented, just a guess.. */
    *msg->bankSizeReturn = *msg->memSizeReturn = data->bytesperline * data->height;
    data->locked++;
    WaitBlitter(csd);
    return TRUE;
}

VOID UAEGFXBitmap__Hidd_BitMap__ReleaseDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
    struct bm_data *data = OOP_INST_DATA(cl, o);
    data->locked--;
}

BOOL UAEGFXBitmap__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    WORD i, j;
    UBYTE *clut;
    
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
    	return FALSE;
    WaitBlitter(csd);
    clut = csd->boardinfo + PSSO_BoardInfo_CLUT;
    for (i = msg->firstColor, j = 0; j < msg->numColors; i++, j++) {
        clut[i * 3 + 0] = msg->colors[j].red >> 8;
        clut[i * 3 + 1] = msg->colors[j].green >> 8;
        clut[i * 3 + 2] = msg->colors[j].blue >> 8;
	//bug("%d %02x%02x%02x\n", i, msg->colors[j].red >> 8, msg->colors[j].green >> 8, msg->colors[j].blue >> 8);
    }
    SetColorArray(csd, msg->firstColor, msg->numColors);
    return TRUE;
}

VOID UAEGFXBitmap__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_PutPixel *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    ULONG   	       offset;
    HIDDT_Pixel       pixel = msg->pixel;
    UBYTE   	      *mem;
    
    WaitBlitter(csd);
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
    
    return;
}

/****************************************************************************************/

ULONG UAEGFXBitmap__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o,
				 struct pHidd_BitMap_GetPixel *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data 	*data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	 pixel = 0;
    ULONG   	    	 offset;
    UBYTE   	    	*mem;
    
    WaitBlitter(csd);
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
    
    return pixel;
}

/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__DrawLine(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_DrawLine *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
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
}

/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_PutImage *msg)
{
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
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
}

/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_BitMap_PutImageLUT *msg)
{
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
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
}

/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__GetImageLUT(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_BitMap_GetImageLUT *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct RenderInfo ri;
    BOOL v = FALSE;

    WaitBlitter(csd);
    maybeputinvram(csd, data);
    if (data->invram) {
	makerenderinfo(csd, &ri, data);
	if (mode == vHidd_GC_DrawMode_Clear || mode == vHidd_GC_DrawMode_Set) {
    	    ULONG pen = mode == vHidd_GC_DrawMode_Clear ? 0x00000000 : 0xffffffff;
	    v = FillRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, pen, 0xff, data->rgbformat);
	} else if (mode == vHidd_GC_DrawMode_Copy) {
	    v = FillRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, fg, 0xff, data->rgbformat);
	} else if (mode == vHidd_GC_DrawMode_Invert) {
	    v = InvertRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, 0xff, data->rgbformat);
	}
    }
    if (!v)
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__PutPattern(OOP_Class *cl, OOP_Object *o,
				 struct pHidd_BitMap_PutPattern *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel	fg = GC_FG(msg->gc);
    HIDDT_Pixel bg = GC_BG(msg->gc);
    struct Pattern pat;
    struct RenderInfo ri;
    UBYTE drawmode;
    BOOL v = FALSE;
    
    WaitBlitter(csd);

    DB2(bug("blitpattern(%d,%d)(%d,%d)(%x,%d,%d,%d,%d,%d)\n",
	msg->x, msg->y, msg->width, msg->height,
	msg->pattern, msg->patternsrcx, msg->patternsrcy, fg, bg, msg->patternheight));

    if (msg->mask || msg->patterndepth > 1) {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
    }

    maybeputinvram(csd, data);
    if (data->invram) {
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

	if (pat.Size <= 8) {
	    v = BlitPattern(csd, &ri, &pat, msg->x, msg->y, msg->width, msg->height, 0xff, data->rgbformat);
	}
    }

    if (!v)
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel	fg = GC_FG(msg->gc);
    HIDDT_Pixel bg = GC_BG(msg->gc);
    struct Template tmpl;
    struct RenderInfo ri;
    UBYTE drawmode;
    BOOL v = FALSE;
    
    WaitBlitter(csd);
    maybeputinvram(csd, data);
    if (data->invram) {
	makerenderinfo(csd, &ri, data);
	if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
	     drawmode = JAM1;
	else if (GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Invert)
	     drawmode = COMPLEMENT;
	else
	    drawmode = JAM2;
	if (msg->inverttemplate)
	     drawmode |= INVERSVID;

	tmpl.Memory = msg->masktemplate;
	tmpl.BytesPerRow = msg->modulo;
	tmpl.XOffset = msg->srcx;
	tmpl.DrawMode = drawmode;
	tmpl.FgPen = fg;
	tmpl.BgPen = bg;
	v = BlitTemplate(csd, &ri, &tmpl, msg->x, msg->y, msg->width, msg->height, 0xff, data->rgbformat);
    }
    if (!v)
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL UAEGFXBitmap__Hidd_PlanarBM__SetBitMap(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_PlanarBM_SetBitMap *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL UAEGFXBitmap__Hidd_PlanarBM__GetBitMap(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_PlanarBM_GetBitMap *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);

    WaitBlitter(csd);
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
