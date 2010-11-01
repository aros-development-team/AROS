/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for Vesa hidd.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <aros/symbolsets.h>
#define DEBUG 0
#include <aros/debug.h>

#include <string.h>

#include "bitmap.h"
#include "vesagfxclass.h"

#include LC_LIBDEFS_FILE

/* Don't initialize static variables with "=0", otherwise they go into DATA segment */

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVesaGfxAttrBase;
static OOP_AttrBase HiddVesaGfxBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap	    , &HiddBitMapAttrBase   	},
    { IID_Hidd_PixFmt	    , &HiddPixFmtAttrBase   	},
    { IID_Hidd_Gfx  	    , &HiddGfxAttrBase      	},
    { IID_Hidd_Sync 	    , &HiddSyncAttrBase     	},
    /* Private bases */
    { IID_Hidd_VesaGfx	    , &HiddVesaGfxAttrBase  	},
    { IID_Hidd_VesaGfxBitMap, &HiddVesaGfxBitMapAttrBase},
    { NULL  	    	    , NULL  	    	    	}
};

#define MNAME_ROOT(x) PCVesaBM__Root__ ## x
#define MNAME_BM(x) PCVesaBM__Hidd_BitMap__ ## x

/*********** BitMap::New() *************************************/
OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("VesaGfx.BitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	OOP_MethodID	     disp_mid;
	struct BitmapData   *data;
	IPTR 	    	     width, height, depth, multi;
	IPTR		     displayable;
	HIDDT_ModeID 	     modeid;

	data = OOP_INST_DATA(cl, o);

	/* clear all data  */
	memset(data, 0, sizeof(struct BitmapData));

	/* Get attr values */
	OOP_GetAttr(o, aHidd_BitMap_Width, &width);
	OOP_GetAttr(o, aHidd_BitMap_Height, &height);
	OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&data->gfxhidd);
	OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&data->pixfmtobj);
	OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
	OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_Depth, &depth);
	OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_BytesPerPixel, &multi);
	
	ASSERT (width != 0 && height != 0 && depth != 0);
	/* 
	   We must only create depths that are supported by the friend drawable
	   Currently we only support the default depth
	   */

	width=(width+15) & ~15;
	data->width = width;
	data->height = height;
	data->bpp = depth;

	data->bytesperpix = multi;
	data->bytesperline = width * multi;
	D(bug("[VesaBitMap] Size %dx%d, %u bytes per pixel, %u bytes per line, displayable: %u\n", width, height, multi, data->bytesperline, displayable));

	OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
	if (modeid != vHidd_ModeID_Invalid) {
	    OOP_Object *sync, *pf;
	    IPTR dwidth, dheight;

	    HIDD_Gfx_GetMode(data->gfxhidd, modeid, &sync, &pf);
	    OOP_GetAttr(sync, aHidd_Sync_HDisp, &dwidth);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp, &dheight);
	    data->disp_width  = dwidth;
	    data->disp_height = dheight;
	}

    	data->VideoData = AllocVec(data->bytesperline * height, MEMF_PUBLIC | MEMF_CLEAR);
	D(bug("[VesaBitMap] Video data at 0x%p (%u bytes)\n", data->VideoData, width * height * multi));

	if (data->VideoData) {
	    HIDDT_ColorModel cmod;
	    
	    if (!displayable)
	        ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);

	    OOP_GetAttr(data->pixfmtobj, aHidd_PixFmt_ColorModel, &cmod);
	    if (cmod != vHidd_ColorModel_Palette)
		ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);

	    data->DAC = AllocMem(768, MEMF_ANY);
	    D(bug("[VesaBitMap] Palette data at 0x%p\n", data->DAC));
	    if (data->DAC)
		ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);
	}

	disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

	OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
	o = NULL;
    } /* if created object */

    ReturnPtr("VesaGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/
VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    D(bug("[VesaBitMap] Dispose(0x%p)\n", o));

    if (data->DAC)
	FreeMem(data->DAC, 768);
    if (data->VideoData)
    	FreeVec(data->VideoData);

    OOP_DoSuperMethod(cl, o, msg);

    ReturnVoid("VesaGfx.BitMap::Dispose");
}

/*** init_bitmapclass *********************************************************/

static int PCVesaBM_Init(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("PCVesaOnBM_Init\n"));
    
    ReturnPtr("PCVesaOnBM_Init", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*** free_bitmapclass *********************************************************/

static int PCVesaBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("PCVesaOnBM_Expunge", int, TRUE);
}

ADD2INITLIB(PCVesaBM_Init, 0)
ADD2EXPUNGELIB(PCVesaBM_Expunge, 0)

/*********  BitMap::PutPixel()  ***************************/

VOID MNAME_BM(PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG   	       offset;
    HIDDT_Pixel       pixel = msg->pixel;
    UBYTE   	      *mem;
    
    offset = (msg->x * data->bytesperpix) + (msg->y * data->bytesperline);
    mem = data->VideoData + offset;
    
    switch(data->bytesperpix)
    {
    	case 1:
	    *(UBYTE *)mem = pixel;
	    break;
	   
	case 2:
	    *(UWORD *)mem = pixel;
	    break;
	    
	case 3:
#if AROS_BIG_ENDIAN
	    *(UBYTE *)(mem) = pixel >> 16;
	    *(UBYTE *)(mem + 1) = pixel >> 8;
	    *(UBYTE *)(mem + 2) = pixel;
#else
	    *(UBYTE *)(mem) = pixel;
	    *(UBYTE *)(mem + 1) = pixel >> 8;
	    *(UBYTE *)(mem + 2) = pixel >> 16;
#endif
 	    break;
	    
	case 4:
	    *(ULONG *)mem = pixel;
	    break;
    }
    
    return;
}

/*********  BitMap::GetPixel()  *********************************/

HIDDT_Pixel MNAME_BM(GetPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct BitmapData 	*data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	 pixel;
    ULONG   	    	 offset;
    UBYTE   	    	*mem;
    
    offset = (msg->x * data->bytesperpix)  +(msg->y * data->bytesperline);
    mem = data->VideoData + offset;
    
    switch(data->bytesperpix)
    {
    	case 1:
	    pixel = *(UBYTE *)mem;
	    break;
	    
	case 2:
	    pixel = *(UWORD *)mem;
	    break;
	    
	case 3:
#if AROS_BIG_ENDIAN
	    pixel = (mem[0] << 16) | (mem[1] << 8) | mem[2];
#else
	    pixel = (mem[2] << 16) | (mem[1] << 8) | mem[0];
#endif
	    break;
	    
	case 4:
	    pixel = *(ULONG *)mem;
	    break;
	    
	default:
	    pixel = 0;
	    break;
	    
    }
    
    return pixel;
}

/*********  BitMap::FillRect()  ***************************/

VOID MNAME_BM(FillRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct BitmapData  *data =OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	fg = GC_FG(msg->gc);
    HIDDT_DrawMode  	mode = GC_DRMD(msg->gc);
    ULONG   	    	mod;

    mod = data->bytesperline;

    switch(mode)
    {
        case vHidd_GC_DrawMode_Copy:
	    switch(data->bytesperpix)
	    {
	    	case 1:
		    HIDD_BM_FillMemRect8(o,
	    	    	    		 data->VideoData,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
		    
		case 2:
		    HIDD_BM_FillMemRect16(o,
	    	    	    		 data->VideoData,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
	    
	    	case 3:
		    HIDD_BM_FillMemRect24(o,
	    	    	    		 data->VideoData,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
		
	    	case 4:
		    HIDD_BM_FillMemRect32(o,
	    	    	    		 data->VideoData,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
		
	    }
	    break;
    
	case vHidd_GC_DrawMode_Invert:
	    HIDD_BM_InvertMemRect(o,
	    	    	    	 data->VideoData,
	    	    	    	 msg->minX * data->bytesperpix,
				 msg->minY,
				 msg->maxX * data->bytesperpix + data->bytesperpix - 1,
				 msg->maxY,
				 mod);
	    break;
	    
	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(mode) */
}

/*********  BitMap::PutImage()  ***************************/

VOID MNAME_BM(PutImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	    switch(data->bytesperpix)
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
	    switch(data->bytesperpix)
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
    	    	APTR 	    dstBuf = data->VideoData + msg->y * data->bytesperline + msg->x * data->bytesperpix;
		OOP_Object *srcpf;
		
		srcpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);
		
		HIDD_BM_ConvertPixels(o, &pixels, (HIDDT_PixelFormat *)srcpf, msg->modulo,
		    	    	      &dstBuf, (HIDDT_PixelFormat *)data->pixfmtobj, data->bytesperline,
				      msg->width, msg->height, NULL);    	    	
	    }
	    break;
	    
    } /* switch(msg->pixFmt) */	    
}

/*********  BitMap::GetImage()  ***************************/

VOID MNAME_BM(GetImage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	    switch(data->bytesperpix)
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
	    switch(data->bytesperpix)
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
    	    	APTR 	    srcPixels = data->VideoData + msg->y * data->bytesperline + msg->x * data->bytesperpix;
		OOP_Object *dstpf;
		
		dstpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);
		
		HIDD_BM_ConvertPixels(o, &srcPixels, (HIDDT_PixelFormat *)data->pixfmtobj, data->bytesperline,
		    	    	      &pixels, (HIDDT_PixelFormat *)dstpf, msg->modulo,
				      msg->width, msg->height, NULL);    	    	
	    }		
	    break;
	    
    } /* switch(msg->pixFmt) */	    
}

VOID MNAME_BM(PutImageLUT)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    switch(data->bytesperpix)
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

/*** BitMap::PutTemplate() **********************************************/

VOID MNAME_BM(PutTemplate)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    switch(data->bytesperpix)
    {
	case 1:
	    HIDD_BM_PutMemTemplate8(o,
	    	    	    	    msg->gc,
				    msg->template,
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
				     msg->template,
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
				     msg->template,
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
				     msg->template,
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
	    
    } /* switch(data->bytesperpix) */
}

/*** BitMap::PutPattern() **********************************************/

VOID MNAME_BM(PutPattern)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPattern *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    switch(data->bytesperpix)
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
	    
    } /* switch(data->bytesperpix) */
}

/*** BitMap::Get() *******************************************/

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG   	       idx;

    if (IS_VesaGfxBM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_VesaGfxBitMap_Drawable:
		*msg->storage = (IPTR)data->VideoData;
		return;
	}
    } else if (IS_BM_ATTR(msg->attrID, idx)) {
	switch (idx) {
	case aoHidd_BitMap_Visible:
	    *msg->storage = data->disp;
	    return;
	case aoHidd_BitMap_LeftEdge:
	    *msg->storage = data->xoffset;
	    return;
	case aoHidd_BitMap_TopEdge:
	    *msg->storage = data->yoffset;
	    return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::Set() *******************************************/

VOID MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    LONG xoffset = data->xoffset;
    LONG yoffset = data->yoffset;
    LONG limit;

    tstate = msg->attrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_Visible:
		D(bug("[VesaBitMap] Setting Visible to %d\n", tag->ti_Data));
		data->disp = tag->ti_Data;
		if (data->disp) {
		    if (data->DAC)
			DACLoad(XSD(cl), data->DAC, 0, 256);
		}
		break;
	    case aoHidd_BitMap_LeftEdge:
	        xoffset = tag->ti_Data;
		/* Our bitmap can not be smaller than display size
		   because of fakegfx.hidd limitations (it can't place
		   cursor beyond bitmap edges). Otherwize Intuition
		   will provide strange user experience (mouse cursor will
		   disappear) */
    		limit = data->disp_width - data->width;
    		if (xoffset > 0)
		    xoffset = 0;
		else if (xoffset < limit)
		    xoffset = limit;
		break;
	    case aoHidd_BitMap_TopEdge:
	        yoffset = tag->ti_Data;
		limit = data->disp_height - data->height;
		if (yoffset > 0)
		    yoffset = 0;
		else if (yoffset < limit)
		    yoffset = limit;
		break;
	    }
	}
    }

    if ((xoffset != data->xoffset) || (yoffset != data->yoffset)) {
	D(bug("[VesaBitMap] Scroll to (%d, %d)\n", xoffset, yoffset));
	data->xoffset = xoffset;
	data->yoffset = yoffset;

	LOCK_FRAMEBUFFER(XSD(cl));
	vesaDoRefreshArea(&XSD(cl)->data, data, 0, 0, data->width, data->height);
	UNLOCK_FRAMEBUFFER(XSD(cl));
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::SetColors() *************************************/

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct HWData *hwdata = &XSD(cl)->data;
    ULONG xc_i, col_i;
    UBYTE p_shift;
    UWORD red, green, blue;

    D(bug("[VesaBitMap] SetColors(%u, %u)\n", msg->firstColor, msg->numColors));

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) {
	D(bug("[VesaBitMap] DoSuperMethod() failed\n"));
	return FALSE;
    }

    if ((msg->firstColor + msg->numColors) > (1 << data->bpp))
	return FALSE;

    if (data->DAC) {
	for ( xc_i = msg->firstColor, col_i = 0;
    	      col_i < msg->numColors; 
	      xc_i ++, col_i ++) {
	    red   = msg->colors[col_i].red   >> 8;
	    green = msg->colors[col_i].green >> 8;
	    blue  = msg->colors[col_i].blue  >> 8;

	    /* Update DAC registers */
	    p_shift = 8 - hwdata->palettewidth;
	    data->DAC[xc_i*3] = red >> p_shift;
	    data->DAC[xc_i*3+1] = green >> p_shift;
	    data->DAC[xc_i*3+2] = blue >> p_shift;
	}

	/* Upload palette to the DAC if the current bitmap is on display */
	if (data->disp)
	    DACLoad(XSD(cl), data->DAC, msg->firstColor, msg->numColors);
    }
    return TRUE;
}

VOID MNAME_BM(UpdateRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    D(bug("[VesaBitMap] UpdateRect(%d, %d, %d, %d), bitmap 0x%p\n", msg->x, msg->y, msg->width, msg->height, o));
    if (data->disp) {
	LOCK_FRAMEBUFFER(XSD(cl));
        vesaDoRefreshArea(&XSD(cl)->data, data, msg->x, msg->y, msg->x + msg->width, msg->y + msg->height);
	UNLOCK_FRAMEBUFFER(XSD(cl));
    }
}
