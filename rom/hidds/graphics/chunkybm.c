/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct chunkybm_data    *data;
    OOP_Object      	    *pf;
    IPTR   	    	    bytesperrow, bytesperpixel;
    struct TagItem	    *tag;
    OOP_MethodID	    dispose_mid;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;

    /* Initialize the instance data to 0 */
    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof (*data));

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
    OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (APTR)&data->gfxhidd);
    /* Get some dimensions of the bitmap */
    OOP_GetAttr(o, aHidd_BitMap_BytesPerRow, &bytesperrow);
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel,	&bytesperpixel);

    data->bytesperpixel = bytesperpixel;
    data->bytesperrow	= bytesperrow;

    tag = FindTagItem(aHidd_ChunkyBM_Buffer, msg->attrList);
    if (tag)
    {
    	/*
    	 * NULL user-supplied buffer is valid.
    	 * In this case we create a bitmap with no buffer. We can attach it later.
    	 */
    	data->own_buffer = FALSE;
    	data->buffer     = (APTR)tag->ti_Data;

    	return o;
    }
    else
    {
    	IPTR height;

    	OOP_GetAttr(o, aHidd_BitMap_Height, &height);

    	data->own_buffer = TRUE;
    	data->buffer = AllocVec(height * bytesperrow, MEMF_ANY | MEMF_CLEAR);
    	
    	if (data->buffer)
    	    return o;
    }

    /* free all on error */
    dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

    OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
    return NULL;
}

/****************************************************************************************/

void CBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct chunkybm_data *data;
    
    data = OOP_INST_DATA(cl, o);
    
    if (data->own_buffer)
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
    APTR dst_pixels, src_pixels;
    OOP_Object *srcpf;

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
            src_pixels = msg->pixels;
            dst_pixels = data->buffer + msg->y * data->bytesperrow
                + msg->x * data->bytesperpixel;
            srcpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);

            HIDD_BM_ConvertPixels(o, &src_pixels,
                (HIDDT_PixelFormat *)srcpf, msg->modulo, &dst_pixels,
                BM_PIXFMT(o), data->bytesperrow, msg->width, msg->height,
                NULL);

    } /* switch(msg->pixFmt) */

}

/**************************************************************************/

int static inline
__attribute__((always_inline, const)) do_alpha(int a, int v)
{
    int tmp = a * v;
    return (tmp + (tmp >> 8) + 0x80) >> 8;
}

VOID CBM__Hidd_BitMap__PutAlphaImage(OOP_Class *cl, OOP_Object *o,
    struct pHidd_BitMap_PutAlphaImage *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    HIDDT_StdPixFmt pixFmt = BM_PIXFMT(o)->stdpixfmt;
    WORD x, y, src_step, dst_step;
    UBYTE *p, *q;
    UBYTE src_red, src_green, src_blue, src_alpha;
    UBYTE dst_red, dst_green, dst_blue;

    switch(pixFmt)
    {
        case vHidd_StdPixFmt_BGR032:

            p = msg->pixels;
            q = data->buffer + msg->y * data->bytesperrow
                + msg->x * data->bytesperpixel;
            src_step = msg->modulo - msg->width * 4;
            dst_step = data->bytesperrow - data->bytesperpixel * msg->width;

            for(y = 0; y < msg->height; y++)
            {
                for(x = 0; x < msg->width; x++)
                {
                    src_alpha = *p++;
                    src_red   = *p++;
                    src_green = *p++;
                    src_blue  = *p++;

                    switch(src_alpha)
                    {
                    case 0:
                        q += 4;
                        break;

                    case 0xff:
                        *q++ = src_blue;
                        *q++ = src_green;
                        *q++ = src_red;
                        *q++ = 0;
                        break;

                    default:
                        dst_blue = *q;
                        dst_blue += do_alpha(src_alpha, src_blue - dst_blue);
                        *q++ = dst_blue;

                        dst_green = *q;
                        dst_green += do_alpha(src_alpha, src_green - dst_green);
                        *q++ = dst_green;

                        dst_red = *q;
                        dst_red  += do_alpha(src_alpha, src_red - dst_red);
                        *q++ = dst_red;

                        *q++ = 0;
                    }
                }
                p += src_step;
                q += dst_step;
            }
            break;

        case vHidd_StdPixFmt_RGB16_LE:

            p = msg->pixels;
            q = data->buffer + msg->y * data->bytesperrow
                + msg->x * data->bytesperpixel;
            src_step = msg->modulo - msg->width * 4;
            dst_step = data->bytesperrow - data->bytesperpixel * msg->width;

            for(y = 0; y < msg->height; y++)
            {
                for(x = 0; x < msg->width; x++)
                {
                    src_alpha = *p++;
                    src_red   = *p++;
                    src_green = *p++;
                    src_blue  = *p++;

                    switch(src_alpha)
                    {
                    case 0:
                        q += 2;
                        break;

                    case 0xff:
                        *q++ = (src_green << 3) & 0xe0 | src_blue >> 3;
                        *q++ = src_red & 0xf8 | src_green >> 5;
                        break;

                    default:
                        dst_blue = *q;
                        dst_red = *(q + 1);
                        dst_green = dst_red << 5 | dst_blue >> 3 & 0x1c;
                        dst_blue <<= 3;
                        dst_red &= 0xf8;

                        dst_blue += do_alpha(src_alpha, src_blue - dst_blue);
                        dst_green += do_alpha(src_alpha, src_green - dst_green);
                        dst_red  += do_alpha(src_alpha, src_red - dst_red);

                        *q++ = (dst_green << 3) & 0xe0 | dst_blue >> 3;
                        *q++ = dst_red & 0xf8 | dst_green >> 5;
                    }
                }
                p += src_step;
                q += dst_step;
            }
            break;

	default:
	    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	    break;
    }
}

/****************************************************************************************/

VOID CBM__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    APTR src_pixels, dst_pixels;
    OOP_Object *dstpf;

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
            src_pixels = data->buffer + msg->y * data->bytesperrow
                + msg->x * data->bytesperpixel;
            dst_pixels = msg->pixels;
            dstpf = HIDD_Gfx_GetPixFmt(data->gfxhidd, msg->pixFmt);

            HIDD_BM_ConvertPixels(o, &src_pixels, BM_PIXFMT(o),
                data->bytesperrow, &dst_pixels, (HIDDT_PixelFormat *)dstpf,
                msg->modulo, msg->width, msg->height, NULL);
	    
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

VOID CBM__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);

    switch(data->bytesperpixel)
    {
	case 1:
	    HIDD_BM_PutMemTemplate8(o,
	    	    	    	    msg->gc,
				    msg->masktemplate,
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
				     msg->masktemplate,
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
				     msg->masktemplate,
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
				     msg->masktemplate,
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

VOID CBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    EnterFunc(bug("BitMap::Get() attrID: %i  storage: %p\n", msg->attrID, msg->storage));

    if (IS_CHUNKYBM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
    	case aoHidd_ChunkyBM_Buffer:
            *msg->storage = (IPTR)data->buffer;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/****************************************************************************************/

VOID CBM__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct chunkybm_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG idx;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_CHUNKYBM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_ChunkyBM_Buffer:
                    if (data->own_buffer)
                    {
    	                FreeVec(data->buffer);
                        data->own_buffer = FALSE;
                    }
                    data->buffer = (UBYTE *)tag->ti_Data;
                    D(bug("[CBM] New buffer now 0x%p\n", data->buffer));
                    break;
            }
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
