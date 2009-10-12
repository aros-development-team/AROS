/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English.
*/

#include <exec/alerts.h>
#include <string.h>    // memset() prototype
#include <aros/macros.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*********  BitMap::PutPixel()  ***************************/

VOID MNAME_BM(PutPixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    ULONG   	       offset;
#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    ULONG   	       offset2;
#endif
    HIDDT_Pixel       pixel = msg->pixel;
    UBYTE   	      *mem;
#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    UBYTE   	      *mem2;
#endif
    
    offset = (msg->x * data->bytesperpix) + (msg->y * data->bytesperline);
    mem = data->VideoData + offset;

#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    offset2 = (msg->x * data->bytesperpix) + (msg->y * data->data->bytesperline);
    mem2 = data->data->framebuffer + offset2;
#endif
    
    switch(data->bytesperpix)
    {
    	case 1:
	    *(UBYTE *)mem = pixel;
    	#if defined(OnBitmap) && defined(BUFFERED_VRAM)
	    if (data->data->use_updaterect == FALSE)
	    {
	    	*(UBYTE *)mem2 = pixel;
	    }
    	#endif
	    break;
	   
	case 2:
	    *(UWORD *)mem = pixel;
    	#if defined(OnBitmap) && defined(BUFFERED_VRAM)
	    if (data->data->use_updaterect == FALSE)
	    {
	    	*(UWORD *)mem2 = pixel;
	    }
	#endif
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

        #if defined(OnBitmap) && defined(BUFFERED_VRAM)
    	    if (data->data->use_updaterect == FALSE)
	    {
	    #if AROS_BIG_ENDIAN
		*(UBYTE *)(mem2) = pixel >> 16;
		*(UBYTE *)(mem2 + 1) = pixel >> 8;
		*(UBYTE *)(mem2 + 2) = pixel;
	    #else
		*(UBYTE *)(mem2) = pixel;
		*(UBYTE *)(mem2 + 1) = pixel >> 8;
		*(UBYTE *)(mem2 + 2) = pixel >> 16;
	    #endif
	    }
        #endif
 	    break;
	    
	case 4:
	    *(ULONG *)mem = pixel;
	#if defined(OnBitmap) && defined(BUFFERED_VRAM)
	    if (data->data->use_updaterect == FALSE)
	    {
	    	*(ULONG *)mem2 = pixel;
	    }
    	#endif
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

#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    LOCK_FRAMEBUFFER(XSD(cl));    
    vesaRefreshArea(data, msg->minX, msg->minY, msg->maxX, msg->maxY);    
    UNLOCK_FRAMEBUFFER(XSD(cl));
#endif
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
	#if 1
	    {
	    	APTR 	    pixels = msg->pixels;
    	    	APTR 	    dstBuf = data->VideoData + msg->y * data->bytesperline + msg->x * data->bytesperpix;
		OOP_Object *srcpf;
		
		srcpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);
		
		HIDD_BM_ConvertPixels(o, &pixels, (HIDDT_PixelFormat *)srcpf, msg->modulo,
		    	    	      &dstBuf, (HIDDT_PixelFormat *)data->pixfmtobj, data->bytesperline,
				      msg->width, msg->height, NULL);    	    	
	    }
		
	#else
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	#endif
	    break;
	    
    } /* switch(msg->pixFmt) */

#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    LOCK_FRAMEBUFFER(XSD(cl));    
    vesaRefreshArea(data, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);    
    UNLOCK_FRAMEBUFFER(XSD(cl));
#endif
	    
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
	#if 1
	    {
	    	APTR 	    pixels = msg->pixels;
    	    	APTR 	    srcPixels = data->VideoData + msg->y * data->bytesperline + msg->x * data->bytesperpix;
		OOP_Object *dstpf;
		
		dstpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);
		
		HIDD_BM_ConvertPixels(o, &srcPixels, (HIDDT_PixelFormat *)data->pixfmtobj, data->bytesperline,
		    	    	      &pixels, (HIDDT_PixelFormat *)dstpf, msg->modulo,
				      msg->width, msg->height, NULL);    	    	
	    }		
	#else		
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    	#endif
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

#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    LOCK_FRAMEBUFFER(XSD(cl));    
    vesaRefreshArea(data, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);    
    UNLOCK_FRAMEBUFFER(XSD(cl));
#endif
	    
}

/*** BitMap::BlitColorExpansion() **********************************************/
VOID MNAME_BM(BlitColorExpansion)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
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
		    *((UWORD *)mem) = pix;
		    mem += 2;
    	    	    break;

		case 3:
		#if AROS_BIG_ENDIAN
		    mem[0] = pix >> 16;
		    mem[1] = pix >> 8;
		    mem[2] = pix;
		#else
		    mem[0] = pix;
		    mem[1] = pix >> 8;
		    mem[2] = pix >> 16;
		#endif
		    mem += 3;
		    break;

		case 4:
		    *((ULONG *)mem) = pix;
		    mem += 4;
		    break;

	    }
	    
	} /* for (each x) */

    	mem += mod;

    } /* for (each y) */

#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    LOCK_FRAMEBUFFER(XSD(cl));    
    vesaRefreshArea(data, msg->destX, msg->destY, msg->destX + msg->width - 1, msg->destY + msg->height - 1);
    UNLOCK_FRAMEBUFFER(XSD(cl));    
#endif
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

#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    LOCK_FRAMEBUFFER(XSD(cl));    
    vesaRefreshArea(data, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);    
    UNLOCK_FRAMEBUFFER(XSD(cl));
#endif
	    
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

#if defined(OnBitmap) && defined(BUFFERED_VRAM)
    LOCK_FRAMEBUFFER(XSD(cl));    
    vesaRefreshArea(data, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);    
    UNLOCK_FRAMEBUFFER(XSD(cl));
#endif
	    
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
		*msg->storage = (ULONG)data->VideoData;
		break;
	    default:
		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	}
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/*** BitMap::SetColors() *************************************/

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct HWData *hwdata = &XSD(cl)->data;
    HIDDT_PixelFormat *pf;
    
    ULONG xc_i, col_i;
    UBYTE p_shift;
    
    HIDDT_Pixel	red, green, blue;
    
    D(bug("[VESA] SetColors(%u, %u)\n", msg->firstColor, msg->numColors));
    pf = BM_PIXFMT(o);

    if (    vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf)
    	 || vHidd_ColorModel_TrueColor	   == HIDD_PF_COLMODEL(pf) ) {
        D(bug("[VESA] SetColors: not a palette bitmap\n"));
	 
	 /* Superclass takes care of this case */
	 
	 return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    /* We have a vHidd_GT_Palette bitmap */    
    
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) {
	D(bug("[VESA] DoSuperMethod() failed\n"));
	return FALSE;
    }
    
    if ((msg->firstColor + msg->numColors) > (1 << data->bpp))
	return FALSE;
    
    for ( xc_i = msg->firstColor, col_i = 0;
    		col_i < msg->numColors; 
		xc_i ++, col_i ++ )
    {
	red   = msg->colors[col_i].red   >> 8;
	green = msg->colors[col_i].green >> 8;
	blue  = msg->colors[col_i].blue  >> 8;

	/* Set given color as allocated */
	data->cmap[xc_i] = 0x01000000 | red | (green << 8) | (blue << 16);

	/* Update DAC registers */
	p_shift = 8 - hwdata->palettewidth;
	hwdata->DAC[xc_i*3] = red >> p_shift;
	hwdata->DAC[xc_i*3+1] = green >> p_shift;
	hwdata->DAC[xc_i*3+2] = blue >> p_shift;
	
	msg->colors[col_i].pixval = xc_i;
    }

    /* Upload palette to the DAC if OnBitmap */
#ifdef OnBitmap
#ifdef TRACK_POINTER_PALETTE
    if ((msg->firstColor <= 20) && (msg->firstColor + msg->numColors - 1 >= 17))
	bug("%d colors from %d changed\n", msg->firstColor, msg->numColors);
#endif
    ObtainSemaphore(&XSD(cl)->HW_acc);
    DACLoad(hwdata, msg->firstColor, msg->numColors);
    ReleaseSemaphore(&XSD(cl)->HW_acc);
#endif /* OnBitmap */

    return TRUE;
}

#if defined(OnBitmap) && defined(BUFFERED_VRAM)

VOID MNAME_BM(UpdateRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    struct HWData *hwdata = &XSD(cl)->data;
    
    if (hwdata->use_updaterect)
    {
        LOCK_FRAMEBUFFER(XSD(cl));    
        vesaDoRefreshArea(data, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1);    
        UNLOCK_FRAMEBUFFER(XSD(cl));    	
    }  
}

#endif
