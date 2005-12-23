/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics chunky bitmap class implementation.
    Lang: english
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

OOP_Object *CBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct chunkybm_data    *data;
    
    IPTR   	    	    width, height;

#if 0
    UBYTE   	    	    alignoffset	= 15;
    UBYTE   	    	    aligndiv	= 2;
#endif
    
    BOOL    	    	    ok = TRUE;
    OOP_Object      	    *pf;
    IPTR   	    	    bytesperpixel;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;
	
    /* Initialize the instance data to 0 */
    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof (*data));

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
    /* Get some dimensions of the bitmap */
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel,	&bytesperpixel);
    
    width = (width + 15) & ~15;
    
    data->bytesperpixel = bytesperpixel;
    data->bytesperrow	= data->bytesperpixel * width;

    data->buffer = AllocVec(height * data->bytesperrow, MEMF_ANY|MEMF_CLEAR);
    if (NULL == data->buffer)
    	ok = FALSE;

    /* free all on error */
    
    if(ok == FALSE)
    {
        OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        if(o) OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
        o = NULL;
    }
   
    return o;
    
}

/****************************************************************************************/

void CBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct chunkybm_data *data;
    
    data = OOP_INST_DATA(cl, o);
    
    if (NULL != data->buffer)
    	FreeVec(data->buffer);
	
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
				struct pHidd_BitMap_PutPixel *msg)
{
    UBYTE *dest;
    
    struct chunkybm_data *data;
    
    data = OOP_INST_DATA(cl, o);

    /* bitmap in chunky-mode */
    dest = data->buffer + msg->x * data->bytesperpixel + msg->y * data->bytesperrow;
    
    switch(data->bytesperpixel)
    {
	case 1:
	    *((UBYTE *) dest) = (UBYTE) msg->pixel;
	    break;
		
	case 2:
	    *((UWORD *) dest) = (UWORD) msg->pixel;
	    break;
	    
	case 3:
    	#if AROS_BIG_ENDIAN
	    dest[0] = (UBYTE)(msg->pixel >> 16) & 0x000000FF;
	    dest[1] = (UBYTE)(msg->pixel >> 8) & 0x000000FF;
	    dest[2] = (UBYTE)msg->pixel & 0x000000FF;
	#else
	    dest[0] = (UBYTE)msg->pixel & 0x000000FF;
	    dest[1] = (UBYTE)(msg->pixel >> 8) & 0x000000FF;
	    dest[2] = (UBYTE)(msg->pixel >> 16) & 0x000000FF;
	#endif
	    break;
		
/*	 if (1 == ( ((IPTR)dest) & 1) )
                {
                  *((UBYTE *) dest++) = (UBYTE) msg->pixel >> 16;
                  *((UWORD *) dest  ) = (UWORD) msg->pixel;
                }
                else
                {
                  *((UWORD *) dest++) = (UWORD) msg->pixel >> 8; 
                  *((UBYTE *) dest  ) = (UBYTE) msg->pixel;
                }
		break;
*/
	case 4:
	    *((ULONG *) dest) = (ULONG) msg->pixel;
	    break;
    }
	
}

/****************************************************************************************/

ULONG CBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o,
				 struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel     	    retval = 0;
    UBYTE   	    	    *src;
    struct chunkybm_data    *data;
    
    data = OOP_INST_DATA(cl, o);
          
    src = data->buffer + msg->x * data->bytesperpixel + msg->y * data->bytesperrow;

    switch(data->bytesperpixel)
    {
	case 1:
	    retval = (HIDDT_Pixel) *((UBYTE *) src);
	    break;
	    
	case 2:
	    retval = (HIDDT_Pixel) *((UWORD *) src);
	    break;
	    
	case 3:
	#if AROS_BIG_ENDIAN
	    retval = (HIDDT_Pixel) (src[0] << 16) + (src[1] << 8) + src[2];
    	#else
	    retval = (HIDDT_Pixel) (src[2] << 16) + (src[1] << 8) + src[0];
	#endif
	    break;
	    
	    //(*((UBYTE *) src++) << 16) | *((UWORD *) src));
	    //break;
	
	case 4:
	    retval = ((ULONG) *((ULONG *) src));
	    break;
    }

    return retval;
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct chunkybm_data  *data =OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	   fg = GC_FG(msg->gc);
    HIDDT_DrawMode  	   mode = GC_DRMD(msg->gc);
    ULONG   	    	   mod;

    mod = data->bytesperrow;

    switch(mode)
    {
        case vHidd_GC_DrawMode_Copy:
	    switch(data->bytesperpixel)
	    {
	    	case 1:
		    HIDD_BM_FillMemRect8(o,
	    	    	    		 data->buffer,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
		    
		case 2:
		    HIDD_BM_FillMemRect16(o,
	    	    	    		 data->buffer,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
	    
	    	case 3:
		    HIDD_BM_FillMemRect24(o,
	    	    	    		 data->buffer,
	    	    	    		 msg->minX,
					 msg->minY,
					 msg->maxX,
					 msg->maxY,
					 mod,
					 fg);
		    break;
		
	    	case 4:
		    HIDD_BM_FillMemRect32(o,
	    	    	    		 data->buffer,
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
	    	    	    	 data->buffer,
	    	    	    	 msg->minX * data->bytesperpixel,
				 msg->minY,
				 msg->maxX * data->bytesperpixel + data->bytesperpixel - 1,
				 msg->maxY,
				 mod);
	    break;
	    
	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(mode) */
    
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

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
					data->buffer,
					msg->x,
					msg->y,
					msg->width,
					msg->height,
					msg->modulo,
					data->bytesperrow);
		    break;
		    
		case 2:
	    	    HIDD_BM_CopyMemBox16(o,
		    	    		 msg->pixels,
					 0,
					 0,
					 data->buffer,
					 msg->x,
					 msg->y,
					 msg->width,
					 msg->height,
					 msg->modulo,
					 data->bytesperrow);
		    break;
		   
		case 3:
	    	    HIDD_BM_CopyMemBox24(o,
		    	    		 msg->pixels,
					 0,
					 0,
					 data->buffer,
					 msg->x,
					 msg->y,
					 msg->width,
					 msg->height,
					 msg->modulo,
					 data->bytesperrow);
		    break;
		
		case 4:
	    	    HIDD_BM_CopyMemBox32(o,
		    	    		 msg->pixels,
					 0,
					 0,
					 data->buffer,
					 msg->x,
					 msg->y,
					 msg->width,
					 msg->height,
					 msg->modulo,
					 data->bytesperrow);
		    break;
		     
    	    } /* switch(data->bytesperpixel) */
	    break;
	
    	case vHidd_StdPixFmt_Native32:
	    switch(data->bytesperpixel)
	    {
	    	case 1:
		    HIDD_BM_PutMem32Image8(o,
		    	    	    	   msg->pixels,
					   data->buffer,
					   msg->x,
					   msg->y,
					   msg->width,
					   msg->height,
					   msg->modulo,
					   data->bytesperrow);
		    break;
		    
		case 2:
		    HIDD_BM_PutMem32Image16(o,
		    	    	    	    msg->pixels,
					    data->buffer,
					    msg->x,
					    msg->y,
					    msg->width,
					    msg->height,
					    msg->modulo,
					    data->bytesperrow);
		    break;

		case 3:
		    HIDD_BM_PutMem32Image24(o,
		    	    	    	    msg->pixels,
					    data->buffer,
					    msg->x,
					    msg->y,
					    msg->width,
					    msg->height,
					    msg->modulo,
					    data->bytesperrow);
		    break;

		case 4:		    
	    	    HIDD_BM_CopyMemBox32(o,
		    	    		 msg->pixels,
					 0,
					 0,
					 data->buffer,
					 msg->x,
					 msg->y,
					 msg->width,
					 msg->height,
					 msg->modulo,
					 data->bytesperrow);
		    break;
		    
	    } /* switch(data->bytesperpixel) */
	    break;
	    
	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(msg->pixFmt) */

}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
	    switch(data->bytesperpixel)
	    {
	    	case 1:
	    	    HIDD_BM_CopyMemBox8(o,
		    	    		data->buffer,
					msg->x,
					msg->y,
					msg->pixels,
					0,
					0,
					msg->width,
					msg->height,
					data->bytesperrow,
					msg->modulo);
		    break;
		    
		case 2:
	    	    HIDD_BM_CopyMemBox16(o,
		    	    		 data->buffer,
					 msg->x,
					 msg->y,
					 msg->pixels,
					 0,
					 0,
					 msg->width,
					 msg->height,
					 data->bytesperrow,
					 msg->modulo);
		    break;

		case 3:
	    	    HIDD_BM_CopyMemBox24(o,
		    	    		 data->buffer,
					 msg->x,
					 msg->y,
					 msg->pixels,
					 0,
					 0,
					 msg->width,
					 msg->height,
					 data->bytesperrow,
					 msg->modulo);
		    break;
		   
		case 4:
	    	    HIDD_BM_CopyMemBox32(o,
		    	    		 data->buffer,
					 msg->x,
					 msg->y,
					 msg->pixels,
					 0,
					 0,
					 msg->width,
					 msg->height,
					 data->bytesperrow,
					 msg->modulo);
		    break;
		     
    	    } /* switch(data->bytesperpix) */
	    break;

    	case vHidd_StdPixFmt_Native32:
	    switch(data->bytesperpixel)
	    {
	    	case 1:
		    HIDD_BM_GetMem32Image8(o,
		    	    	    	   data->buffer,
					   msg->x,
					   msg->y,
					   msg->pixels,
					   msg->width,
					   msg->height,
					   data->bytesperrow,
					   msg->modulo);
		    break;
		    
		case 2:
		    HIDD_BM_GetMem32Image16(o,
		    	    	    	    data->buffer,
					    msg->x,
					    msg->y,
					    msg->pixels,
					    msg->width,
					    msg->height,
					    data->bytesperrow,
					    msg->modulo);
		    break;

		case 3:
		    HIDD_BM_GetMem32Image24(o,
		    	    	    	    data->buffer,
					    msg->x,
					    msg->y,
					    msg->pixels,
					    msg->width,
					    msg->height,
					    data->bytesperrow,
					    msg->modulo);
		    break;

		case 4:		    
	    	    HIDD_BM_CopyMemBox32(o,
		    	    		 data->buffer,
					 msg->x,
					 msg->y,
					 msg->pixels,
					 0,
					 0,
					 msg->width,
					 msg->height,
					 data->bytesperrow,
					 msg->modulo);
		    break;
		    
	    } /* switch(data->bytesperpixel) */
	    break;
	    
	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
	    
    } /* switch(msg->pixFmt) */
	    
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    switch(data->bytesperpixel)
    {
	case 2:
	    HIDD_BM_CopyLUTMemBox16(o,
		    	    	 msg->pixels,
				 0,
				 0,
				 data->buffer,
				 msg->x,
				 msg->y,
				 msg->width,
				 msg->height,
				 msg->modulo,
				 data->bytesperrow,
				 msg->pixlut);
	    break;

	case 3:
	    HIDD_BM_CopyLUTMemBox24(o,
		    	    	 msg->pixels,
				 0,
				 0,
				 data->buffer,
				 msg->x,
				 msg->y,
				 msg->width,
				 msg->height,
				 msg->modulo,
				 data->bytesperrow,
				 msg->pixlut);
	    break;

	case 4:
	    HIDD_BM_CopyLUTMemBox32(o,
		    	    	    msg->pixels,
				    0,
				    0,
				    data->buffer,
				    msg->x,
				    msg->y,
				    msg->width,
				    msg->height,
				    msg->modulo,
				    data->bytesperrow,
				    msg->pixlut);
	    break;
	    
	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    } /* switch(data->bytesperpixel) */

}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    struct chunkybm_data   *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	    fg, bg, pix;
    ULONG   	    	    cemd;
    LONG    	    	    x, y;
    ULONG   	    	    mod, bpp;
    UBYTE   	           *mem;
    BOOL    	    	    opaque;
    
    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    cemd = GC_COLEXP(msg->gc);

    bpp = data->bytesperpixel;
    
    mem = data->buffer + msg->destY * data->bytesperrow + msg->destX * bpp;
    mod = data->bytesperrow - msg->width * bpp;
    
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

}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    switch(data->bytesperpixel)
    {
	case 1:
	    HIDD_BM_PutMemTemplate8(o,
	    	    	    	    msg->gc,
				    msg->template,
				    msg->modulo,
				    msg->srcx,
				    data->buffer,
				    data->bytesperrow,
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
				     data->buffer,
				     data->bytesperrow,
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
				     data->buffer,
				     data->bytesperrow,
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
				     data->buffer,
				     data->bytesperrow,
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

}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__PutPattern(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPattern *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    switch(data->bytesperpixel)
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
				   data->buffer,
				   data->bytesperrow,
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
				    data->buffer,
				    data->bytesperrow,
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
				    data->buffer,
				    data->bytesperrow,
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
				    data->buffer,
				    data->bytesperrow,
				    msg->x,
				    msg->y,
				    msg->width,
				    msg->height);
	    break;

	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    	    break;
	    
    } /* switch(data->bytesperpixel) */
    
}

/****************************************************************************************/
