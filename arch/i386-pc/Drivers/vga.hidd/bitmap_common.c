#include <exec/alerts.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

static VOID set_pixelformat(Object *bm, struct vga_staticdata *xsd)
{
    HIDDT_PixelFormat *pf = BM_PIXFMT(bm);
    
    pf->red_mask	= 0x000000ff;
    pf->green_mask	= 0x0000ff00;
    pf->blue_mask	= 0x00ff0000;
    
    pf->red_shift	= 24;
    pf->green_shift	= 16;
    pf->blue_shift	= 8;
}

/*********  BitMap::Clear()  *************************************/
static VOID MNAME(clear)(Class *cl, Object *o, struct pHidd_BitMap_Clear *msg)
{
    ULONG width, height, bg;
    struct bitmap_data *data = INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};
    
    GetAttr(o, aHidd_BitMap_Background, &bg);

    /* Get width & height from bitmap superclass */

    GetAttr(o, aHidd_BitMap_Width,  &width);
    GetAttr(o, aHidd_BitMap_Height, &height);

    box.x2 = width - 1;
    box.y2 = height - 1;

    memset(data->VideoData, bg, width*height);

#ifdef OnBitmap
    ObtainSemaphore(&XSD(cl)->HW_acc);
    vgaRefreshArea(data, 1, &box);
    ReleaseSemaphore(&XSD(cl)->HW_acc);
#endif /* OnBitmap */
    
    return;
}

/**************  BitMap::Set()  *********************************/
static VOID MNAME(set)(Class *cl, Object *o, struct pRoot_Set *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    struct TagItem *tag, *tstate;
    ULONG idx;
    
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_BitMap_Foreground :
		    data->fg = tag->ti_Data;
		    break;
		    
                case aoHidd_BitMap_Background :
		    data->bg = tag->ti_Data;
		    break;
		    
		case aoHidd_BitMap_DrawMode :		    
		    data->drmd = tag->ti_Data;
		    break;
            }
        }
    }
    
    /* Let supermethod take care of other attrs */
    DoSuperMethod(cl, o, (Msg)msg);
    
    return;
}

static HIDDT_Pixel MNAME(mapcolor)(Class *cl, Object *o, struct pHidd_BitMap_MapColor *msg)
{
    int i,f;
    struct bitmap_data *data = INST_DATA(cl, o);

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

static VOID MNAME(unmappixel)(Class *cl, Object *o, struct pHidd_BitMap_UnmapPixel *msg)
{
    int i,f;
    struct bitmap_data *data = INST_DATA(cl, o);

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

/*********  BitMap::PutPixel()  ***************************/

static VOID MNAME(putpixel)(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
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
#endif /* OnBitmap */
    return;
}

/*********  BitMap::GetPixel()  *********************************/
static HIDDT_Pixel MNAME(getpixel)(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel=0;
    struct bitmap_data *data = INST_DATA(cl, o);
    
    unsigned char *ptr;

    ptr = (char *)(data->VideoData + msg->x + (msg->y * data->width));

    pixel = *(char*)ptr;

    /* Get pen number from colortab */
    return pixel;
}

/*********  BitMap::DrawPixel()  ***************************/

static VOID MNAME(drawpixel)(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    HIDDT_Pixel fg;
    unsigned char *ptr;

#ifdef OnBitmap
    int pix;
    int i;
    unsigned char *ptr2;
#endif /* OnBitmap */

    GetAttr(o, aHidd_BitMap_Foreground, &fg);
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
#endif /* OnBitmap */
    return;
}

/*********  BitMap::CopyBox()  ***************************/

static VOID MNAME(copybox)(Class *cl, Object *o, struct pHidd_BitMap_CopyBox *msg)
{
    ULONG mode;
    unsigned char *dest;
    struct bitmap_data *data = INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};

    GetAttr(msg->dest, aHidd_BitMap_DrawMode, &mode);

    if (o != msg->dest)
    {

    	GetAttr(msg->dest, aHidd_VGABitMap_Drawable, (IPTR *)&dest);
	
	if (0 == dest)
	{
	    /* The destination object is no VGA bitmap, onscreen nor offscreen.
	       Let the superclass do the copying in a more general way
	    */
	    DoSuperMethod(cl, o, (Msg)msg);
	    return;
	}
	
    }
    else
    {
    	dest = data->VideoData;
    }

    {
        struct bitmap_data *ddata = INST_DATA(cl, msg->dest);
        int i, width, phase, j;

        // start of Source data
        unsigned char *s_start = data->VideoData +
                                 msg->srcX + (msg->srcY * data->width);
        // adder for each line
        ULONG s_add = data->width - msg->width;
        ULONG cnt = msg->height;

        unsigned char *d_start = ddata->VideoData +
                                 msg->destX + (msg->destY * ddata->width);
        ULONG d_add = ddata->width - msg->width;

	width = msg->width;

	if ((phase = (long)s_start & 3L))
	{
	    phase = 4 - phase;
	    if (phase > width) phase = width;
	    width -= phase;
	}

        while (cnt--)
        {
	    i = width;
	    j = phase;
            while (j--)
            {
                *(unsigned char*)d_start++ = *(unsigned char*)s_start++;
            }
	    while (i >= 4)
	    {
		*((unsigned long*)d_start) = *((unsigned long*)s_start);
		d_start += 4;
		s_start += 4;
		i -= 4;
	    }
	    while (i--)
            {
                *(unsigned char*)d_start++ = *(unsigned char*)s_start++;
            }
            d_start += d_add;
            s_start += s_add;
        }
	if (ddata->disp)
	{
    	    box.x1 = msg->destX;
    	    box.y1 = msg->destY;
    	    box.x2 = box.x1 + msg->width;
    	    box.y2 = box.y1 + msg->height;
    	    vgaRefreshArea(ddata, 1, &box);
	}
    }
}

/*********  BitMap::PutImage()  ***************************/

unsigned char MNAME(best_color)(struct bitmap_data *data, unsigned long color)
{
    int i;
    unsigned pixel = -1;
    
    i=0;
    
    do
    {
	if ((data->cmap[i] & 0xffffff) == color)	/* Find color */
	    pixel = i;
	i++;
    } while ((pixel == -1) && (i<16));
    
    return pixel;
}
    

static VOID MNAME(putimage)(Class *cl, Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};

    int i;

    // start of Source data
    unsigned char *buff = data->VideoData +
                                 msg->x + (msg->y * data->width);
    // adder for each line
    ULONG add = data->width - msg->width;
    ULONG cnt = msg->height;

    unsigned long *s_start = msg->pixels;

    while (cnt > 0)
    {
        i = msg->width;
        while (i)
        {
//            *buff++ = MNAME(best_color)(data, *s_start++);
            *buff++ = (unsigned char)*s_start++;
            i--;
        }
        buff += add;
        cnt--;
    }
    if (data->disp)
    {
        box.x1 = msg->x;
        box.y1 = msg->y;
        box.x2 = box.x1 + msg->width;
        box.y2 = box.y1 + msg->height;
        vgaRefreshArea(data, 1, &box);
    }
}

/*********  BitMap::FillRect()  ***************************/

static VOID MNAME(fillrect)(Class *cl, Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    struct Box box = {0, 0, 0, 0};
    HIDDT_Pixel fg;
    int i, phase, j;

    ULONG width = msg->maxX - msg->minX + 1;

    // start of video data
    unsigned char *s_start = data->VideoData +
                                 msg->minX + (msg->minY * data->width);
    // adder for each line
    ULONG s_add = data->width - width;
    ULONG cnt = msg->maxY - msg->minY + 1;

    GetAttr(o, aHidd_BitMap_Foreground, &fg);

    fg |= ((char)fg) << 8;
    fg |= ((short)fg) << 16;

    if ((phase = (long)s_start & 3L))
    {
        phase = 4 - phase;
        if (phase > width) phase = width;
        width -= phase;
    }

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
    if (data->disp)
    {
        box.x1 = msg->minX;
        box.y1 = msg->minY;
        box.x2 = msg->maxX;
        box.y2 = msg->maxY;
        vgaRefreshArea(data, 1, &box);
    }
}

/*** BitMap::Get() *******************************************/

static VOID MNAME(get)(Class *cl, Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    ULONG idx;
    if (IS_VGABM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_VGABitMap_Drawable:
	    	*msg->storage = (ULONG)data->VideoData;
		break;
		
	    default:
	    	DoSuperMethod(cl, o, (Msg)msg);
		break;
	}
    }
    else
    {
    	DoSuperMethod(cl, o, (Msg)msg);
    }

    return;
}
