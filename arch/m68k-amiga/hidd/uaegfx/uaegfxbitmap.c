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
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "uaegfx.h"
#include "uaegfxbitmap.h"
#include "uaertg.h"

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

    DB2(bug("UAEGFXBitmap__Root__New\n"));

    o =(OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;
	
    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof  (*data));

    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
    OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
    OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&data->gfxhidd);
    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&data->pixfmtobj);
    OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_BytesPerPixel, &multi);
 
    data->rgbformat = getrtgformat(csd, data->pixfmtobj);
    data->width = width;
    width = CalculateBytesPerRow(csd, width, data->rgbformat);
    data->bytesperline = width;
    data->height = height;
    data->bytesperpixel = multi;
    SetMemoryMode(csd, RGBFB_CLUT);
    data->VideoData = Allocate(csd->vmem, data->bytesperline * data->height);
    SetMemoryMode(csd, data->rgbformat);
 
    DB2(bug("%dx%dx%d RGBF=%08x P=%08x\n", width, height, multi, data->rgbformat, data->VideoData));

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
    
    DB2(bug("ret=%x bm=%x\n", o, data));
  	
    return o;
}

VOID UAEGFXBitmap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data    *data;
    
    data = OOP_INST_DATA(cl, o);
    
    DB2(bug("UAEGFXBitmap__Root__Dispose %x bm=%x\n", o, data));
    if (data->disp)
    	DB2(bug("removing displayed bitmap?!\n"));
    
    FreeVec(data->palette);
    SetMemoryMode(csd, RGBFB_CLUT);
    Deallocate(csd->vmem, data->VideoData, data->bytesperline * data->height);
    SetMemoryMode(csd, data->rgbformat);
    
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

VOID UAEGFXBitmap__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    BOOL moved = FALSE;

    DB2(bug("UAEGFXBitmap__Root__Set\n"));
    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
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
		    modeinfo = getrtgmodeinfo(csd, sync, pf, &data->modeinfo);
		    csd->modeinfo = modeinfo;
		    csd->rgbformat = data->rgbformat;
		    pw(csd->bitmapextra + PSSO_BitMapExtra_Width, width);
		    pw(csd->bitmapextra + PSSO_BitMapExtra_Height, height);
		    D(bug("%dx%dx%d (%dx%d) BF=%08x\n", dwidth, dheight, depth, width, height, data->rgbformat));

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
	            data->disp = TRUE;
	            csd->disp = data;
		} else {
	    	    SetInterrupt(csd, FALSE);
		    SetDisplay(csd, FALSE);
		    SetSwitch(csd, FALSE);
		    csd->dmodeid = 0;
		    data->disp = FALSE;
		    csd->disp = NULL;
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
//    if (moved && csd->disp == data)
//    	setscroll(csd, data);
}

VOID UAEGFXBitmap__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    DB2(bug("UAEGFXBitmap__Root__Get\n"));
    if (IS_BITMAP_ATTR(msg->attrID, idx)) {
 	DB2(bug("=%d\n", idx));
	switch (idx) {
	case aoHidd_BitMap_LeftEdge:
	    *msg->storage = 0;//data->leftedge;
	    return;
	case aoHidd_BitMap_TopEdge:
	    *msg->storage = 0;//data->topedge;
	    return;
	case aoHidd_BitMap_Visible:
	    *msg->storage = data->disp;
	    return;
	case aoHidd_BitMap_Align:
	    *msg->storage = 16;
	    return;
	case aoHidd_BitMap_IsLinearMem:
	    *msg->storage = TRUE;
	    return;
	}
    }
    DB2(bug("UAEGFXBitmap__Root__Get Exit\n"));
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
    struct bm_data *data = OOP_INST_DATA(cl, o);

    *msg->addressReturn = data->VideoData;
    *msg->widthReturn = data->width;
    *msg->heightReturn = data->height;
    /* undocumented, just a guess.. */
    *msg->bankSizeReturn = *msg->memSizeReturn = data->bytesperline * data->height;
    data->locked++;
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
    struct bm_data *data = OOP_INST_DATA(cl, o);
    ULONG   	       offset;
    HIDDT_Pixel       pixel = msg->pixel;
    UBYTE   	      *mem;
    
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
    struct bm_data 	*data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	 pixel = 0;
    ULONG   	    	 offset;
    UBYTE   	    	*mem;
    
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

VOID UAEGFXBitmap__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct uaegfx_staticdata *csd = CSD(cl);

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

VOID UAEGFXBitmap__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    struct uaegfx_staticdata *csd = CSD(cl);
    struct bm_data *data = OOP_INST_DATA(cl, o);
    struct RenderInfo ri;
    BOOL v = FALSE;

    makerenderinfo(csd, &ri, data);
    if (mode == vHidd_GC_DrawMode_Clear || mode == vHidd_GC_DrawMode_Set) {
    	ULONG pen = mode == vHidd_GC_DrawMode_Clear ? 0x00000000 : 0xffffffff;
    	v = FillRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, pen, 0xff, data->rgbformat);
    } else if (mode == vHidd_GC_DrawMode_Copy) {
        v = FillRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, fg, 0xff, data->rgbformat);
    } else if (mode == vHidd_GC_DrawMode_Invert) {
       v = InvertRect(csd, &ri, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1, 0xff, data->rgbformat);
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
    
    makerenderinfo(csd, &ri, data);
    if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
    	drawmode = JAM1;
    else if (GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Invert)
     	drawmode = COMPLEMENT;
    else
     	drawmode = JAM2;
    if (msg->inverttemplate)
    	drawmode |= INVERSVID;

    tmpl.Memory = msg->Template;
    tmpl.BytesPerRow = msg->modulo;
    tmpl.XOffset = msg->srcx;
    tmpl.DrawMode = drawmode;
    tmpl.FgPen = fg;
    tmpl.BgPen = bg;
    
    if (!BlitTemplate(csd, &ri, &tmpl, msg->x, msg->y, msg->width, msg->height, 0xff, data->rgbformat))
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


/****************************************************************************************/

VOID UAEGFXBitmap__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL UAEGFXBitmap__Hidd_PlanarBM__SetBitMap(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_PlanarBM_SetBitMap *msg)
{
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL UAEGFXBitmap__Hidd_PlanarBM__GetBitMap(OOP_Class *cl, OOP_Object *o,
				   struct pHidd_PlanarBM_GetBitMap *msg)
{
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
