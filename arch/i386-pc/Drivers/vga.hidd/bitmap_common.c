#include <exec/alerts.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*********  BitMap::Clear()  *************************************/
static VOID MNAME(clear)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    ULONG width, height;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};
    
    /* Get width & height from bitmap superclass */

    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);

    box.x2 = width - 1;
    box.y2 = height - 1;

    memset(data->VideoData, GC_BG(msg->gc), width*height);

#ifdef OnBitmap
    ObtainSemaphore(&XSD(cl)->HW_acc);
    vgaRefreshArea(data, 1, &box);
    ReleaseSemaphore(&XSD(cl)->HW_acc);
    draw_mouse(XSD(cl));
#endif /* OnBitmap */
    
    return;
}

static HIDDT_Pixel MNAME(mapcolor)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_MapColor *msg)
{
    int i,f;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    HIDDT_Pixel red	= msg->color->red >> 8;
    HIDDT_Pixel green	= msg->color->green >> 8;
    HIDDT_Pixel blue	= msg->color->blue >> 8;

    HIDDT_Pixel color = red | (green << 8) | (blue << 16);
    
    i = 0;
    f = 1;
    
    do
    {
	if (data->cmap[i] < 0x01000000)	/* Is empty? */
	{
	    f = 0;			/* Got color */
	    data->cmap[i] = 0x01000000 | red | (green << 8) | (blue << 16);
	    data->Regs->DAC[i*3] = red >> 2;
	    data->Regs->DAC[i*3+1] = green >> 2;
	    data->Regs->DAC[i*3+2] = blue >> 2;
#ifdef OnBitmap
	    ObtainSemaphore(&XSD(cl)->HW_acc);
	    vgaRestore(data->Regs);
	    ReleaseSemaphore(&XSD(cl)->HW_acc);
#endif /* OnBitmap */
	}
	else if ((data->cmap[i] & 0xffffff) == color)
	{
	    if ((data->cmap[i] & 0xff000000) != 0xff000000)
	    {
		data->cmap[i] += 0x01000000;
	    }
	    f=0;
	}
	else i++;	    
    } while (f && (i<16));

    return i;
}

static VOID MNAME(unmappixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UnmapPixel *msg)
{
    int i,f;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    HIDDT_Pixel red	= msg->color->red >> 8;
    HIDDT_Pixel green	= msg->color->green >> 8;
    HIDDT_Pixel blue	= msg->color->blue >> 8;

    HIDDT_Pixel color = red | (green << 8) | (blue << 16);
    
    i = 0;
    f = 1;

    do
    {
	if ((data->cmap[i] & 0xffffff) == color)	/* Find color */
	{
	    f = 0;					/* Got color */
	    if (data->cmap[i] & 0xff000000)		/* Dealloc it if used */
	    {
		data->cmap[i] -= 0x01000000;
	    }
	}
	else i++;	    
    } while (f && (i<16));
}

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
	data->Regs->DAC[xc_i*3] = red >> 2;
	data->Regs->DAC[xc_i*3+1] = green >> 2;
	data->Regs->DAC[xc_i*3+2] = blue >> 2;
	
	msg->colors[col_i].pixval = xc_i;
    }

    /* Restore palette if OnBitmap */
#ifdef OnBitmap
    ObtainSemaphore(&XSD(cl)->HW_acc);
    vgaRestore(data->Regs);
    ReleaseSemaphore(&XSD(cl)->HW_acc);
#endif /* OnBitmap */

    return TRUE;
}

/*********  BitMap::PutPixel()  ***************************/

static VOID MNAME(putpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel fg;
    unsigned char *ptr;

#ifdef OnBitmap
    int pix;
    int i;
    unsigned char *ptr2;
#endif /* OnBitmap */

    fg = msg->pixel;
    ptr = (char *)(data->VideoData + msg->x + (msg->y * data->width));
    
    *ptr = (char) fg;

#ifdef OnBitmap
    ptr2 = (char *)(0xa0000 + (msg->x + (msg->y * data->width)) / 8);
    pix = 0x8000 >> (msg->x % 8);
    ObtainSemaphore(&XSD(cl)->HW_acc);

    outw(0x3c4,0x0f02);
    outw(0x3ce,pix | 8);
    outw(0x3ce,0x0005);
    outw(0x3ce,0x0003);
    outw(0x3ce,(fg << 8));
    outw(0x3ce,0x0f01);

    *ptr2 |= 1;		// This or'ed value isn't important

    ReleaseSemaphore(&XSD(cl)->HW_acc);

    if (((msg->x >= XSD(cl)->mouseX) && (msg->x < (XSD(cl)->mouseX + XSD(cl)->mouseW))) ||
	((msg->y >= XSD(cl)->mouseY) && (msg->y < (XSD(cl)->mouseY + XSD(cl)->mouseH))))
	draw_mouse(XSD(cl));

#endif /* OnBitmap */

    return;
}

/*********  BitMap::GetPixel()  *********************************/
static HIDDT_Pixel MNAME(getpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel=0;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    unsigned char *ptr;

    ptr = (char *)(data->VideoData + msg->x + (msg->y * data->width));

    pixel = *(char*)ptr;

    /* Get pen number from colortab */
    return pixel;
}

#if 0

/*********  BitMap::DrawPixel()  ***************************/

static VOID MNAME(drawpixel)(OOP_Class *cl,OOP_ Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel fg;
    unsigned char *ptr;

#ifdef OnBitmap
    int pix;
    int i;
    unsigned char *ptr2;
#endif /* OnBitmap */

    fg = GC_FG(msg->gc);

    ptr = (char *)(data->VideoData + msg->x + (msg->y * data->width));
    *ptr = (char) fg;

#ifdef OnBitmap
    ptr2 = (char *)(0xa0000 + (msg->x + (msg->y * data->width)) / 8);
    pix = 0x8000 >> (msg->x % 8);
    ObtainSemaphore(&XSD(cl)->HW_acc);

    outw(0x3c4,0x0f02);
    outw(0x3ce,pix | 8);
    outw(0x3ce,0x0005);
    outw(0x3ce,0x0003);
    outw(0x3ce,(fg << 8));
    outw(0x3ce,0x0f01);

    *ptr2 |= 1;		// This or'ed value isn't important

    ReleaseSemaphore(&XSD(cl)->HW_acc);

    if (((msg->x >= XSD(cl)->mouseX) && (msg->x < (XSD(cl)->mouseX + XSD(cl)->mouseW))) ||
	((msg->y >= XSD(cl)->mouseY) && (msg->y < (XSD(cl)->mouseY + XSD(cl)->mouseH))))
	draw_mouse(XSD(cl));

#endif /* OnBitmap */

    return;
}

#endif

/*********  BitMap::PutImage()  ***************************/

static VOID MNAME(putimage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};

    int i;

    // start of Source data
    unsigned char *buff = data->VideoData +
                                 msg->x + (msg->y * data->width);
    // adder for each line
    ULONG add = data->width - msg->width;
    ULONG cnt = msg->height;

    unsigned long *s_start = (unsigned long *)msg->pixels;

    EnterFunc(bug("VGAGfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));

    while (cnt > 0)
    {
        i = msg->width;
        while (i)
        {
            *buff++ = (unsigned char)*s_start++;
            i--;
        }
        buff += add;
	s_start += (msg->modulo - msg->width);
        cnt--;
    }
    if (data->disp)
    {
        box.x1 = msg->x;
        box.y1 = msg->y;
        box.x2 = box.x1 + msg->width - 1;
        box.y2 = box.y1 + msg->height - 1;
        ObtainSemaphore(&XSD(cl)->HW_acc);
        vgaRefreshArea(data, 1, &box);
        ReleaseSemaphore(&XSD(cl)->HW_acc);
	
	if ( (	(XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= box.x1) &&
		(XSD(cl)->mouseX <= box.x2) ) ||
	    (	(XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= box.y1) && 
		(XSD(cl)->mouseY <= box.y2) ) )
	    draw_mouse(XSD(cl));
    }
    ReturnVoid("VGAGfx.BitMap::PutImage");
}

/*********  BitMap::GetImage()  ***************************/

static VOID MNAME(getimage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    int i;

    // start of Source data
    unsigned char *buff = data->VideoData +
                                 msg->x + (msg->y * data->width);
    // adder for each line
    ULONG add = data->width - msg->width;
    ULONG cnt = msg->height;

    unsigned long *s_start = (unsigned long *)msg->pixels;

    while (cnt > 0)
    {
        i = msg->width;
        while (i)
        {
	    *s_start++ = (unsigned long)*buff++;
            i--;
        }
        buff += add;
	s_start += (msg->modulo - msg->width);
        cnt--;
    }
}

/*********  BitMap::PutImageLUT()  ***************************/

static VOID MNAME(putimagelut)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};

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
    if (data->disp)
    {
        box.x1 = msg->x;
        box.y1 = msg->y;
        box.x2 = box.x1 + msg->width - 1;
        box.y2 = box.y1 + msg->height - 1;
        ObtainSemaphore(&XSD(cl)->HW_acc);
        vgaRefreshArea(data, 1, &box);
        ReleaseSemaphore(&XSD(cl)->HW_acc);

        if ( (  (XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= box.x1) &&
                (XSD(cl)->mouseX <= box.x2) ) ||
            (   (XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= box.y1) &&
                (XSD(cl)->mouseY <= box.y2) ) )
            draw_mouse(XSD(cl));
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
    struct Box box = {0, 0, 0, 0};
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
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

    
    if (data->disp)
    {
        box.x1 = msg->minX;
        box.y1 = msg->minY;
        box.x2 = msg->maxX;
        box.y2 = msg->maxY;
        ObtainSemaphore(&XSD(cl)->HW_acc);
        vgaRefreshArea(data, 1, &box);
        ReleaseSemaphore(&XSD(cl)->HW_acc);

        if ( (  (XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= box.x1) &&
                (XSD(cl)->mouseX <= box.x2) ) ||
            (   (XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= box.y1) &&
                (XSD(cl)->mouseY <= box.y2) ) )
            draw_mouse(XSD(cl));
    }
    ReturnVoid("VGAGfx.BitMap::FillRect");
}

static VOID PutPixel(OOP_Class *cl, struct bitmap_data *data, int x, int y, unsigned long fg)
{
    unsigned char *ptr;

#ifdef OnBitmap
    int pix;
    int i;
    unsigned char *ptr2;
#endif /* OnBitmap */

    ptr = (char *)(data->VideoData + x + (y * data->width));
    *ptr = (char) fg;

#ifdef OnBitmap
    ptr2 = (char *)(0xa0000 + (x + (y * data->width)) / 8);
    pix = 0x8000 >> (x % 8);
    ObtainSemaphore(&XSD(cl)->HW_acc);

    outw(0x3c4,0x0f02);
    outw(0x3ce,pix | 8);
    outw(0x3ce,0x0005);
    outw(0x3ce,0x0003);
    outw(0x3ce,(fg << 8));
    outw(0x3ce,0x0f01);

    *ptr2 |= 1;		// This or'ed value isn't important

    ReleaseSemaphore(&XSD(cl)->HW_acc);

    if (((x >= XSD(cl)->mouseX) && (x < (XSD(cl)->mouseX + XSD(cl)->mouseW))) ||
	((y >= XSD(cl)->mouseY) && (y < (XSD(cl)->mouseY + XSD(cl)->mouseH))))
	draw_mouse(XSD(cl));

#endif /* OnBitmap */
    return;
}

/*** BitMap::BlitColorExpansion() **********************************************/
static VOID MNAME(blitcolorexpansion)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    ULONG cemd;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct Box box;
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

    if (data->disp)
    {
        box.x1 = msg->destX;
        box.y1 = msg->destY;
        box.x2 = box.x1 + msg->width - 1;
        box.y2 = box.y1 + msg->height - 1;
        ObtainSemaphore(&XSD(cl)->HW_acc);
        vgaRefreshArea(data, 1, &box);
        ReleaseSemaphore(&XSD(cl)->HW_acc);

        if ( (  (XSD(cl)->mouseX + XSD(cl)->mouseW - 1 >= box.x1) &&
                (XSD(cl)->mouseX <= box.x2) ) ||
            (   (XSD(cl)->mouseY + XSD(cl)->mouseH - 1 >= box.y1) &&
                (XSD(cl)->mouseY <= box.y2) ) )
            draw_mouse(XSD(cl));
    }    
    ReturnVoid("VGAGfx.BitMap::BlitColorExpansion");
}

/*** BitMap::Get() *******************************************/

static VOID MNAME(get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;
    if (IS_VGABM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_VGABitMap_Drawable:
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

