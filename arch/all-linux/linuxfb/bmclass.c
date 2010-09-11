/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Onscreen bitmap class for linux fb device
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include <aros/symbolsets.h>

#include "linux_intern.h"
#include "bitmap.h"

#include LC_LIBDEFS_FILE

static OOP_AttrBase HiddBitMapAttrBase = 0;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase 	},
    { NULL  	    	, NULL      	    	}
};

#define DEBUG 0
#include <aros/debug.h>
 
/*********** BitMap::New() *************************************/

OOP_Object *LinuxBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL ok = TRUE;
    
// kill(getpid(), 19);
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
//kprintf("LINUXFB: GOT OBJ %p\n", o);
    if (NULL != o)
    {
    	struct BitmapData *data;
	BOOL framebuffer;
	IPTR val;
	
	data = OOP_INST_DATA(cl, o);

    	OOP_GetAttr(o, aHidd_BitMap_Width,  &val); data->width = val;
    	OOP_GetAttr(o, aHidd_BitMap_Height, &val); data->height = val;

    	framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
	if (framebuffer)
	{
	    data->VideoData = LSD(cl)->baseaddr;
    	    data->bytesperpix = LSD(cl)->pf.bytes_per_pixel;
    	    data->bytesperline = LSD(cl)->fsi.line_length;
	    
	#if BUFFERED_VRAM
	    data->width = (data->width + 15) & ~15;

	    data->RealVideoData = data->VideoData;
	    data->realbytesperline = data->bytesperline;
	    
	    data->bytesperline = data->bytesperpix * data->width;	    
	    data->VideoData = AllocVec(data->bytesperline * data->height, MEMF_CLEAR);
	    if (!data->VideoData)
	    {
	    	ok = FALSE;
	    }
	    else
	    {
	    	data->VideoDataAllocated = TRUE;
	    }
	#endif
	}
	else
	{
	    data->width = (data->width + 15) & ~15;
	    data->bytesperpix = LSD(cl)->pf.bytes_per_pixel;
	    data->bytesperline = data->bytesperpix * data->width;
	    
	    data->VideoData = AllocVec(data->bytesperline * data->height, MEMF_CLEAR);
	    if (!data->VideoData)
	    {
	    	ok = FALSE;
	    }
	    else
	    {
	    	data->VideoDataAllocated = TRUE;
	    }
	}
	
	if (!ok)
	{
	    ULONG dispose_mid;
	    dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	
	    OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
	    o = NULL;
    	}
    }
    
    return o;
}

/**********  Bitmap::Dispose()  ***********************************/
VOID LinuxBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);

    if (data->VideoDataAllocated)
    	FreeVec(data->VideoData);

    OOP_DoSuperMethod(cl, o, msg);
}

/*********  BitMap::ObtainDirectAccess()  *************************************/
BOOL LinuxBM__Hidd_BitMap__ObtainDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ObtainDirectAccess *msg)
{
    ULONG width, height;
    
    /* Get width & height from bitmap object */
  
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    
    *msg->addressReturn	= LSD(cl)->baseaddr;
    *msg->widthReturn	= LSD(cl)->vsi.xres_virtual;
    *msg->heightReturn	= LSD(cl)->vsi.yres_virtual;
    *msg->bankSizeReturn = *msg->memSizeReturn = LSD(cl)->fsi.smem_len;
    
    return TRUE;
}

VOID LinuxBM__Hidd_BitMap__ReleaseDirectAccess(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ReleaseDirectAccess *msg)
{
     /* Do nothing */
#warning Here we can use mprotect() to detect accesses while no access is granted
     return;
}

/*********** BitMap::PutPixel() ***********************************************/
VOID LinuxBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct BitmapData  *data;
    UBYTE   	    	*addr;
    HIDDT_Pixel     	 pix = msg->pixel;
    
    data = OOP_INST_DATA(cl, o);
    
    addr = data->VideoData + msg->y * data->bytesperline + msg->x * data->bytesperpix;
    pix = msg->pixel;
    
    switch(data->bytesperpix)
    {
    	case 1:
	    *addr = pix;
	    break;
	    
	case 2:
	    *(UWORD *)addr = pix;
	    break;
	    
	case 3:
	#if AROS_BIG_ENDIAN
    	    *addr ++ = (pix >> 16) & 0x000000FF;
    	    *addr ++ = (pix >> 8 ) & 0x000000FF;
    	    *addr ++ = pix & 0x000000FF;
	#else
    	    *addr ++ = pix & 0x000000FF;
    	    *addr ++ = (pix >> 8 ) & 0x000000FF;
    	    *addr ++ = (pix >> 16) & 0x000000FF;
	#endif
	    break;
	    
	case 4:
	default:
	    *(ULONG *)addr = pix;
	    break;	    
    }

#if BUFFERED_VRAM
    if (data->RealVideoData)
    {
    	addr = data->RealVideoData + msg->y * data->realbytesperline + msg->x * data->bytesperpix;

	switch(data->bytesperpix)
	{
    	    case 1:
		*addr = pix;
		break;

	    case 2:
		*(UWORD *)addr = pix;
		break;

	    case 3:
	    #if AROS_BIG_ENDIAN
    		*addr ++ = (pix >> 16) & 0x000000FF;
    		*addr ++ = (pix >> 8 ) & 0x000000FF;
    		*addr ++ = pix & 0x000000FF;
	    #else
    		*addr ++ = pix & 0x000000FF;
    		*addr ++ = (pix >> 8 ) & 0x000000FF;
    		*addr ++ = (pix >> 16) & 0x000000FF;
	    #endif
		break;

	    case 4:
	    default:
		*(ULONG *)addr = pix;
		break;	    
	}
    }
#endif
    
    return;
}

/*********** BitMap::GetPixel() ***********************************************/
HIDDT_Pixel  LinuxBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct BitmapData  *data;
    UBYTE   	    	*addr;
    HIDDT_Pixel     	 pix;
    
    data = OOP_INST_DATA(cl, o);  
    addr = data->VideoData + msg->y * data->bytesperline + msg->x * data->bytesperpix;
    
    /* Set pixel according to pixelformat */

    switch(data->bytesperpix)
    {
    	case 1:
	    pix = *addr;
	    break;
	    
	case 2:
	    pix = *(UWORD *)addr;
	    break;
	    
	case 3:
	#if AROS_BIG_ENDIAN
    	    pix = (addr[0] << 16) | (addr[1] << 8) | (addr[2]);
	#else
    	    pix = (addr[2] | (addr[1] << 8) | (addr[0] << 16));
	#endif
	    break;
	    
	case 4:
	default:
	    pix = *(ULONG *)addr;
	    break;
	    
    }

    return pix;
}

/*********  BitMap::FillRect()  ***************************/

VOID LinuxBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct BitmapData  *data =OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	 fg = GC_FG(msg->gc);
    HIDDT_DrawMode  	 mode = GC_DRMD(msg->gc);
    ULONG   	    	 mod;

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

#if BUFFERED_VRAM
    if (data->RealVideoData)
    {
    	LOCK_FRAMEBUFFER(LSD(cl));
    	fbRefreshArea(data, msg->minX, msg->minY, msg->maxX, msg->maxY);    	    
    	UNLOCK_FRAMEBUFFER(LSD(cl));
    }
#endif
}

/*********  BitMap::PutImage()  ***************************/

VOID LinuxBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
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
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(msg->pixFmt) */

#if BUFFERED_VRAM
    if (data->RealVideoData)
    {
    	LOCK_FRAMEBUFFER(LSD(cl));
    	fbRefreshArea(data, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);    	    
    	UNLOCK_FRAMEBUFFER(LSD(cl));
    }
#endif
	    
}

/*********  BitMap::GetImage()  ***************************/

VOID LinuxBM__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
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
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(msg->pixFmt) */
	    
}

/*** BitMap::PutImageLUT() **********************************************/

VOID LinuxBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
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

    } /* switch(data->bytesperpix) */

#if BUFFERED_VRAM
    if (data->RealVideoData)
    {
    	LOCK_FRAMEBUFFER(LSD(cl));
    	fbRefreshArea(data, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);    	    
    	UNLOCK_FRAMEBUFFER(LSD(cl));
    }
#endif
	    
}

/*** BitMap::BlitColorExpansion() **********************************************/

VOID LinuxBM__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    struct BitmapData  *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	fg, bg, pix;
    ULONG   	    	cemd;
    LONG    	    	x, y;
    ULONG   	    	mod, bpp;
    UBYTE   	       *mem;
    BOOL    	    	opaque;
    
    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    cemd = GC_COLEXP(msg->gc);

    bpp = data->bytesperpix;
    
    mem = data->VideoData + msg->destY * data->bytesperline + msg->destX * bpp;
    mod = data->bytesperline - msg->width * bpp;
    
    opaque = (cemd & vHidd_GC_ColExp_Opaque) ? TRUE : FALSE;
    
    for (y = 0; y < msg->height; y ++)
    {
        for (x = 0; x < msg->width; x ++)
        {
	    ULONG is_set;

	    is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);
	    if (is_set)
	    {
		pix = fg;
	    }
	    else if (opaque)
	    {
		pix = bg;
	    }
	    else
	    {
		mem += bpp;
		continue;
	    }

    	    switch(bpp)
	    {
		case 1:
   	    	    *mem++ = pix;
		    break;

		case 2:
		    *(UWORD *)mem = pix;
		    mem += 2;
    	    	    break;

		case 3:
		#if AROS_BIG_ENDIAN
		    *mem++ = pix >> 16;
		    *mem++ = pix >> 8;
		    *mem++ = pix;
		#else
		    *mem++ = pix;
		    *mem++ = pix >> 8;
		    *mem++ = pix >> 16;
		#endif
		    break;

		case 4:
		    *(ULONG *)mem++ = pix;
		    mem += 4;
		    break;

	    }
	    
	} /* for (each x) */

    	mem += mod;

    } /* for (each y) */

#if BUFFERED_VRAM
    if (data->RealVideoData)
    {
    	LOCK_FRAMEBUFFER(LSD(cl));
    	fbRefreshArea(data, msg->destX, msg->destY, msg->destX + msg->width - 1, msg->destY + msg->height - 1);
     	UNLOCK_FRAMEBUFFER(LSD(cl));
    }
#endif

}

/*** init_onbmclass *********************************************************/

static int Init_BMClass(LIBBASETYPEPTR LIBBASE)
{
    /* Get attrbase for the BitMap interface */
     return OOP_ObtainAttrBases(attrbases);
}


/*** free_bitmapclass *********************************************************/

static int Expunge_BMClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

ADD2INITLIB(Init_BMClass, 0)
ADD2EXPUNGELIB(Expunge_BMClass, 0)
