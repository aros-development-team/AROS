/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English.
*/

#include <exec/alerts.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*********  BitMap::Clear()  *************************************/
static VOID MNAME(clear)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    ULONG width, height;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
	int multi = 1;

    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);

//	if (data == NSD(cl)->visible)
//	{
//		riva_rectfill(NSD(cl), 0, 0, width, height, GC_BG(msg->gc), vHidd_GC_DrawMode_Copy);
//	}
//	else
	{
		if (data->bpp > 16) multi = 4;
		else if (data->bpp > 8) multi = 2;
    
	    memset(data->VideoData, GC_BG(msg->gc), width*height*multi);
 	}
    return;
}

/*********  BitMap::SetColors()  *************************************/

static BOOL MNAME(setcolors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *pf;
    
    ULONG xc_i, col_i;
    
    HIDDT_Pixel	red, green, blue;
    
    pf = BM_PIXFMT(o);

    if (    vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf)
    	 || vHidd_ColorModel_TrueColor	   == HIDD_PF_COLMODEL(pf) ) {
	 
		/* Superclass takes care of this case */
	 
		return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    /* We have a vHidd_GT_Palette bitmap */    
    
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) return FALSE;
    
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
#ifdef OnBitmap
		riva_wclut(&NSD(cl)->riva, xc_i, red, green, blue);
#endif
	
		msg->colors[col_i].pixval = xc_i;
    }

    return TRUE;
}

/*********  BitMap::PutPixel()  ***************************/

static VOID MNAME(putpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

	*((UBYTE*)(data->VideoData + msg->x + (msg->y * data->width))) = msg->pixel;

	if (data == NSD(cl)->visible)
	{
		riva_setup_pat(NSD(cl), -1, -1, -1, -1);
		riva_setup_clip(NSD(cl), 0, 0, data->width, data->height);
		riva_rectfill(NSD(cl), 10, 20, 
			20,
			10,
			1,
			3);
	}

    return;
}

/*********  BitMap::GetPixel()  *********************************/
static HIDDT_Pixel MNAME(getpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel=0;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
	pixel = *((UBYTE*)(data->VideoData + msg->x + (msg->y * data->width)));

    /* Get pen number from colortab */
    return pixel;
}

/*********  BitMap::PutImage()  ***************************/

static VOID MNAME(putimage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    UBYTE   	       *buff = data->VideoData + msg->x + (msg->y * data->width);
    ULONG   	    	add = data->width - msg->width;
    ULONG   	    	cnt = msg->height;
    UBYTE   	       *s_start = (UBYTE *)msg->pixels;
    BOOL    	    	done_by_superclass = FALSE;
    int     	    	i;
   
    EnterFunc(bug("VGAGfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
   		msg->pixels, msg->x, msg->y, msg->width, msg->height));

    switch(msg->pixFmt)
    {
   		case vHidd_StdPixFmt_Native:
		    while (cnt > 0)
		    {
	    		UBYTE *p = s_start;

	        	i = msg->width;
	        	while (i)
	        	{
	        	    *buff++ = *p++;
    	    	    i--;
	        	}
	        	buff += add;
				s_start += msg->modulo;
	        	cnt--;
		    }
		    break;
	    
    	case vHidd_StdPixFmt_Native32:
		    while (cnt > 0)
		    {
    			HIDDT_Pixel *p = (HIDDT_Pixel *)s_start;

	        	i = msg->width;
	        	while (i)
   		    	{
	        	    *buff++ = (UBYTE)*p++;
	        	    i--;
	        	}
	        	buff += add;
				s_start += msg->modulo;
	        	cnt--;
		    }
		    break;
    
		default:
		    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		    done_by_superclass = TRUE;
		    break;	    
    }

    ReturnVoid("VGAGfx.BitMap::PutImage");
}

/*********  BitMap::GetImage()  ***************************/

static VOID MNAME(getimage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);

    UBYTE   	    	*buff = data->VideoData + msg->x + (msg->y * data->width);
    ULONG   	    	 add = data->width - msg->width;
    ULONG   	    	 cnt = msg->height;
    UBYTE      	    	*s_start = (UBYTE *)msg->pixels;
    int     	    	 i;

    switch(msg->pixFmt)
    {
    	case vHidd_StdPixFmt_Native:
		    while (cnt > 0)
		    {
		    	UBYTE *p = s_start;
		
	        	i = msg->width;
	        	while (i)
	        	{
				    *p++ = *buff++;
	        	    i--;
	        	}
	        	buff += add;
				s_start += msg->modulo;
	        	cnt--;
		    }
		    break;
	    
    	case vHidd_StdPixFmt_Native32:
		    while (cnt > 0)
		    {
		    	HIDDT_Pixel *p = (HIDDT_Pixel *)s_start;

	        	i = msg->width;
	        	while (i)
	        	{
				    *p++ = (HIDDT_Pixel)*buff++;
	        	    i--;
	        	}
	        	buff += add;
				s_start += msg->modulo;
	        	cnt--;
		    }
		    break;
	    
   		default:
		    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		    break;
	    
    } /* switch(msg->pixFmt) */
}

/*********  BitMap::PutImageLUT()  ***************************/

static VOID MNAME(putimagelut)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    int i;

    // start of Source data
    unsigned char *buff = data->VideoData +
   		msg->x + (msg->y * data->width);
    // adder for each line
    ULONG add = data->width - msg->width;
    ULONG cnt = msg->height;

    unsigned char *s_start = msg->pixels;

    EnterFunc(bug("VGAGfx.BitMap::PutImageLUT(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
   		msg->pixels, msg->x, msg->y, msg->width, msg->height));

    while (cnt > 0)
    {
        i = msg->width;
        while (i)
        {
            *buff++ = *s_start++;
            i--;
        }
        buff += add;
		s_start += (msg->modulo - msg->width);
        cnt--;
    }
    ReturnVoid("VGAGfx.BitMap::PutImageLUT");
}

/*********  BitMap::GetImageLUT()  ***************************/

static VOID MNAME(getimagelut)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
	
    int i;

    // start of Source data
    unsigned char *buff = data->VideoData +
                                msg->x + (msg->y * data->width);
    // adder for each line
    ULONG add = data->width - msg->width;
    ULONG cnt = msg->height;

    unsigned char *s_start = msg->pixels;

    while (cnt > 0)
    {
   	    i = msg->width;
        while (i)
        {
		    *s_start++ = *buff++;
            i--;
        }
        buff += add;
        cnt--;
    }
}

/*********  BitMap::FillRect()  ***************************/

static VOID MNAME(fillrect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data =OOP_INST_DATA(cl, o);

    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);

//	if (data == NSD(cl)->visible)
//	{
//		riva_setup_pat(NSD(cl), -1, -1, -1, -1);
//		riva_setup_clip(NSD(cl), 0, 0, data->width, data->height);
//		riva_rectfill(NSD(cl), msg->minX, msg->minY, 
//			msg->maxX - msg->minX + 1,
//			msg->maxY - msg->minY + 1,
//			fg,
//			mode);
//	}
//	else
	{
	    int i, phase, j;

	    ULONG width = msg->maxX - msg->minX + 1;

	    // start of video data
	    unsigned char *s_start = data->VideoData +
                                msg->minX + (msg->minY * data->width);
				     
	    // adder for each line
	    ULONG s_add = data->width - width;
	    ULONG cnt = msg->maxY - msg->minY + 1;

	    EnterFunc(bug("VGAGfx.BitMap::FillRect(%d,%d,%d,%d)\n",
	   		msg->minX, msg->minY, msg->maxX, msg->maxY));

	    if ((phase = (long)s_start & 3L))
	    {
	        phase = 4 - phase;
	        if (phase > width) phase = width;
	        width -= phase;
	    }

	    switch(mode)
	    {
	        case vHidd_GC_DrawMode_Copy:
				
			    fg |= ((char)fg) << 8;
			    fg |= ((short)fg) << 16;

			    while (cnt--)
			    {
		        	i = width;
		        	j = phase;
					while (j--)
		        	{		    
		        	    *(unsigned char*)s_start++ = (char)fg;
		        	}
					while (i >= 4)
					{
					    *((unsigned long*)s_start) = fg;
					    s_start += 4;
					    i -= 4;
		        	}
					while (i--)
		        	{
		        	    *(unsigned char*)s_start++ = (char)fg;
		        	}
		        	s_start += s_add;
			    }
			    break;
		    
			case vHidd_GC_DrawMode_Invert:

			    while (cnt--)
			    {
			        unsigned char bg;
					unsigned long bglong;
	
		        	i = width;
		        	j = phase;
					while (j--)
		        	{
					    bg = *s_start;
		        	    *(unsigned char*)s_start++ = ~bg;
		        	}
					while (i >= 4)
					{
					    bglong = *(unsigned long *)s_start;
					    *((unsigned long*)s_start) = ~bglong;
					    s_start += 4;
					    i -= 4;
		        	}
					while (i--)
		        	{
					    bg = *s_start;
		        	    *(unsigned char*)s_start++ = ~bg;
		        	}
		        	s_start += s_add;
			    }
			    break;
		    
			default:
			    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
			    break;
	    
	    } /* switch(mode) */
	}

//	if (data == NSD(cl)->visible)
//	{
//		riva_setup_pat(NSD(cl), -1, -1, -1, -1);
//		riva_setup_clip(NSD(cl), 0, 0, data->width, data->height);
//		riva_rectfill(NSD(cl), 10, 20, 
//			20,
//			10,
//			1,
//			3);
//	}

    ReturnVoid("VGAGfx.BitMap::FillRect");
}

/*** BitMap::BlitColorExpansion() **********************************************/
static VOID MNAME(blitcolorexpansion)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    ULONG cemd;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel fg, bg;
    LONG x, y;

    EnterFunc(bug("VGAGfx.BitMap::BlitColorExpansion(%p, %d, %d, %d, %d, %d, %d)\n"
    	, msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
    
    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    cemd = GC_COLEXP(msg->gc);

    if (cemd & vHidd_GC_ColExp_Opaque)
    {
		for (y = 0; y < msg->height; y ++)
		{
            for (x = 0; x < msg->width; x ++)
            {
				ULONG is_set;

				is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);

	   	    	*(data->VideoData + x + msg->destX + ((y + msg->destY) * data->width)) = is_set ? fg : bg;

		    } /* for (each x) */
		} /* for (each y) */
    }
    else
    {
		for (y = 0; y < msg->height; y ++)
		{
            for (x = 0; x < msg->width; x ++)
            {
				ULONG is_set;

				is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);

    	    	if (is_set)
	   	    	    *(data->VideoData + x + msg->destX + ((y + msg->destY) * data->width)) = fg;

		    } /* for (each x) */
		} /* for (each y) */
    }
    ReturnVoid("VGAGfx.BitMap::BlitColorExpansion");
}

/*** BitMap::Get() *******************************************/

static VOID MNAME(get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;
    if (IS_NVBM_ATTR(msg->attrID, idx))
    {
		switch (idx)
		{
		    case aoHidd_NVBitMap_Drawable:
		    	*msg->storage = (ULONG)data->VideoData;
				break;
		
		    default:
		    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
				break;
		}
    }
    else
    {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    return;
}
