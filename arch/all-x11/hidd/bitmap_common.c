

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
		    /* Set X GC color */
LX11
		    XSetForeground(data->display, data->gc, data->hidd2x11cmap[tag->ti_Data]);
UX11
		    break;
		    
                case aoHidd_BitMap_Background :
LX11
		    XSetBackground(data->display, data->gc, data->hidd2x11cmap[tag->ti_Data]);
UX11		    
		    break;
		    
		case aoHidd_BitMap_DrawMode :		    

LX11
		    XSetFunction(data->display, data->gc, tag->ti_Data);
UX11		    
		    break;
            }
        }
    }
    
    /* Let supermethod take care of other attrs */
    DoSuperMethod(cl, o, (Msg)msg);
    
    return;
}


static BOOL MNAME(setcolors)(Class *cl, Object *o, struct pHidd_BitMap_SetColors *msg)
{
#warning Does not deallocate previously allocated colors
    
    
    struct bitmap_data *data = INST_DATA(cl, o);
    
    ULONG xc_i, col_i;
    
    XColor xc;
    
    
    EnterFunc(bug("X11Gfx.BitMap::SetColors(num=%d, first=%d)\n",
    		msg->numColors, msg->firstColor));
    
    for ( xc_i = msg->firstColor, col_i = 0;
    		col_i < msg ->numColors; 
		xc_i ++, col_i ++ )
    {
	xc.red   = msg->colors[col_i].red;
	xc.green = msg->colors[col_i].green;
	xc.blue	 = msg->colors[col_i].blue;
	
LX11	

	if (XAllocColor(data->display, data->colmap, &xc))
	{
/*	*((ULONG *)0) = 0;
*/		D(bug("Successfully allocated color (%x, %x, %x)\n",
			xc.red, xc.green, xc.blue));
			
	    /* Remember the color */
	    data->hidd2x11cmap[xc_i] = xc.pixel;
        			
	}


UX11	

/*	*((ULONG *)0) = 0;
*/    }
    
    
    ReturnBool("X11Gfx.BitMap::SetColors",  TRUE);

}

/*********  BitMap::PutPixel()  ***************************/

static VOID MNAME(putpixel)(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg)
{
     struct bitmap_data *data = INST_DATA(cl, o);
     ULONG old_fg;
     
     
     GetAttr(o, aHidd_BitMap_Foreground, &old_fg);
LX11     
     XSetForeground(data->display, data->gc, data->hidd2x11cmap[msg->val]);
     XDrawPoint(data->display, DRAWABLE(data), data->gc, msg->x, msg->y);
     
     /* Reset GC to old value */
     XSetForeground(data->display, data->gc, data->hidd2x11cmap[old_fg]);
     
     XFlush(data->display);
UX11     
     return;
}

/*********  BitMap::GetPixel()  *********************************/
static ULONG MNAME(getpixel)(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    ULONG pixel, i;
    struct bitmap_data *data = INST_DATA(cl, o);
    
    XImage *image;

LX11
    XSync(data->display, False);
    
    image = XGetImage(data->display
    	, DRAWABLE(data)
	, msg->x, msg->y
	, 1, 1
	, AllPlanes
	, ZPixmap);
UX11    
    if (!image)
    	return -1L;
	
    pixel = XGetPixel(image, 0, 0);
    
LX11    
    XDestroyImage(image);
UX11    
    /* Get pen number from colortab */

    for (i = 0; i < 256; i ++)
    {
        if (pixel == data->hidd2x11cmap[i])
    	    return i;
    }
    
    return -1L;
    
    
}

/*********  BitMap::DrawPixel() ************************************/
static ULONG MNAME(drawpixel)(Class *cl, Object *o, struct pHidd_BitMap_DrawPixel *msg)
{

    struct bitmap_data *data = INST_DATA(cl, o);
    

    /* Foreground pen allready set in X GC. Note, though, that a
       call to WritePixelDirect may owerwrite the GC's pen  */
LX11       
    XDrawPoint(data->display, DRAWABLE(data), data->gc, msg->x, msg->y);
/*    XFlush(data->display); */
UX11    
    return 0;

    
}


/*********  BitMap::FillRect()  *************************************/
static VOID MNAME(fillrect)(Class *cl, Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    ULONG mode;
    
    
    EnterFunc(bug("X11Gfx.BitMap::FillRect(%d,%d,%d,%d)\n",
    	msg->minX, msg->minY, msg->maxX, msg->maxY));
	
    GetAttr(o, aHidd_BitMap_DrawMode, &mode);
    
    D(bug("Drawmode: %d\n", mode));

// kprintf("fillrect, drmd %d\n", mode);

LX11  
    XFillRectangle(data->display
	, DRAWABLE(data)
	, data->gc
	, msg->minX
	, msg->minY
	, msg->maxX - msg->minX + 1
	, msg->maxY - msg->minY + 1
    );
UX11	

   
    D(bug("Flushing\n"));


LX11
    XFlush(data->display);
UX11    
    ReturnVoid("X11Gfx.BitMap::FillRect");
    

}

/*********  BitMap::GetImage()  *************************************/

static VOID MNAME(getimage)(Class *cl, Object *o, struct pHidd_BitMap_GetImage *msg)
{
    /* Read an X image from which we can faster get the pixels, since the
       X image resides in the client.
    */
    WORD x, y;
    ULONG *pixarray = msg->pixels;
    struct bitmap_data *data;
    XImage *image;
    
    EnterFunc(bug("X11Gfx.BitMap::GetImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
    data = INST_DATA(cl, o);
LX11
    image = XGetImage(data->display
    	, DRAWABLE(data)
	, msg->x, msg->y
	, msg->width, msg->height
	, AllPlanes
	, ZPixmap);
UX11	
    if (!image)
    	ReturnVoid("X11Gfx.BitMap::GetImage(couldn't get XImage)");
	
    for (y = 0; y < msg->height; y ++)
    {
	for (x = 0; x < msg->width; x ++)
	{
	    *pixarray ++ = map_x11_to_hidd(data->hidd2x11cmap, XGetPixel(image, x, y));
	}
	
    }
LX11    
    XDestroyImage(image);
UX11    
    
    ReturnVoid("X11Gfx.BitMap::GetImage");
    
}

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*********  BitMap::PutImage()  *************************************/

static VOID MNAME(putimage)(Class *cl, Object *o, struct pHidd_BitMap_PutImage *msg)
{
    ULONG mode, depth;
    WORD x, y;
    ULONG *pixarray = msg->pixels;
    struct bitmap_data *data;
    XImage *image;
    int bperline;
    void *imdata;
    

    EnterFunc(bug("X11Gfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
    data = INST_DATA(cl, o);
    GetAttr(o, aHidd_BitMap_DrawMode, &mode);
    GetAttr(o, aHidd_BitMap_Depth, &depth);


LX11	
    image = XCreateImage(data->display
	, DefaultVisual(data->display, data->screen)
	, depth
	, ZPixmap
	, 0
	, NULL
	, msg->width, msg->height
	, 32
	, 0
    );
UX11	
    if (!image)
    	ReturnVoid("X11Gfx.BitMap::PutImage(XCreateImage failed)");
	    
    bperline	= image->bytes_per_line;
	
    imdata = malloc((size_t)msg->height * bperline);
    if (!imdata)
    {
LX11	
	XFree(image);
UX11	    
    	ReturnVoid("X11Gfx.BitMap::PutImage(malloc(image data) failed)");
    }
	
    image->data = (char *)imdata;
	
    for (y = 0; y < msg->height; y ++)
    {
	for (x = 0; x < msg->width; x ++)
	{
	    if (depth == 1) {
		XPutPixel(image, x, y, *pixarray ++);
	    } else {
		XPutPixel(image, x, y, data->hidd2x11cmap[*pixarray ++]);
	    }
		
	}
	    
    }

LX11

   XSetFunction(data->display, data->gc, mode);
   XPutImage(data->display
    		, DRAWABLE(data)
		, data->gc
		, image
		, 0, 0
		, msg->x, msg->y
		, msg->width, msg->height
   );

    XFlush(data->display);
UX11    

    free(image->data);
    
LX11    
    XFree(image);
UX11   

   ReturnVoid("X11Gfx.BitMap::PutImage");

   
}

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*** BitMap::BlitColorExpansion() **********************************************/
static VOID MNAME(blitcolorexpansion)(Class *cl, Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    ULONG cemd;
    XImage *dest_im;
    struct bitmap_data *data = INST_DATA(cl, o);
    ULONG fg, bg, fg_pixel, bg_pixel;
    LONG x, y;
    
    Drawable d = 0;
    
    EnterFunc(bug("X11Gfx.BitMap::BlitColorExpansion(%p, %d, %d, %d, %d, %d, %d)\n"
    	, msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
    
    
    GetAttr(msg->srcBitMap, aHidd_X11BitMap_Drawable, (IPTR *)&d);
    
    if (0 == d)
    {
    	/* We know nothing about the source bitmap. Let the superclass handle this */
	DoSuperMethod(cl, o, (Msg)msg);
	return;
    }

    GetAttr(o, aHidd_BitMap_ColorExpansionMode, &cemd);
    GetAttr(o, aHidd_BitMap_Foreground, &fg);
    GetAttr(o, aHidd_BitMap_Background, &bg);
    
    D(bug("fg: %d\n", fg));
    D(bug("bg: %d\n", bg));
    fg_pixel = data->hidd2x11cmap[fg];
    bg_pixel = data->hidd2x11cmap[bg];
    
    D(bug("fg_pixel: %d\n", fg_pixel));
    D(bug("bg_pixel: %d\n", bg_pixel));
    
    if (0 != d)
    {

// kprintf("D");
LX11    
	XSetForeground(data->display, data->gc, fg_pixel);
UX11	
    	if (cemd & vHidd_GC_ColExp_Opaque)  
	{
//	    kprintf("XCP\n");
LX11
	    XSetBackground(data->display, data->gc, bg_pixel);
	    
	    XCopyPlane(data->display
	    	, d, DRAWABLE(data)
		, data->gc
		, msg->srcX, msg->srcY
		, msg->width, msg->height
		, msg->destX, msg->destY
		, 0x01
	    );
UX11	    
	} else {
	    /* Do transparent blit */
	    
	    XGCValues val;
	    val.stipple		= d;
	    val.ts_x_origin	= msg->destX - msg->srcX;
	    val.ts_y_origin	= msg->destY - msg->srcY;
	    val.fill_style	= FillStippled;
	    


//	    kprintf(" XSS\n");

LX11
	    XChangeGC(data->display
	    	, data->gc
		, GCStipple|GCTileStipXOrigin|GCTileStipYOrigin|GCFillStyle
		, &val
	    );
	    XFillRectangle(data->display
	    	, DRAWABLE(data)
		, data->gc
		, msg->destX, msg->destY
		, msg->width, msg->height
	    );
	    XSetFillStyle(data->display, data->gc, FillSolid);

UX11	
	}

    }
    else
    {
    	/* We know nothing about the format of the source bitmap
	   an must get single pixels
	*/

LX11    
	dest_im = XGetImage(data->display
		, DRAWABLE(data)
		, msg->destX, msg->destY
		, msg->width, msg->height
		, AllPlanes
		, ZPixmap);
    	
UX11    
	if (!dest_im)
    	    ReturnVoid("X11Gfx.BitMap::BlitColorExpansion()");


	D(bug("Src bm: %p\n", msg->srcBitMap));
	for (y = 0; y < msg->height; y ++)
	{
	    for (x = 0; x < msg->width; x ++)
	    {
		ULONG is_set;
	    
	    	is_set = HIDD_BM_GetPixel(msg->srcBitMap, x + msg->srcX, y + msg->srcY);
	    
	    	if (is_set)
	    	{
	    	    XPutPixel(dest_im, x, y, fg_pixel);
		
	    	}
	    	else
	    	{
		    if (cemd & vHidd_GC_ColExp_Opaque)
		    {
			XPutPixel(dest_im, x, y, bg_pixel);
		    }
		}
	    } /* for (each x) */
	    
    	} /* for (each y) */
    
	/* Put image back into display */
LX11    
	XPutImage(data->display
    		, DRAWABLE(data)
		, data->gc
		, dest_im
		, 0, 0
		, msg->destX, msg->destY
		, msg->width, msg->height
    	);

    	XDestroyImage(dest_im);
UX11
    }

#if 0    
LX11
    XFlush(data->display);
UX11 
#endif   
    ReturnVoid("X11Gfx.BitMap::BlitColorExpansion");
}

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*********  BitMap::CopyBox()  *************************************/
static VOID MNAME(copybox)(Class *cl, Object *o, struct pHidd_BitMap_CopyBox *msg)
{
    ULONG mode;
    Drawable dest;
    struct bitmap_data *data = INST_DATA(cl, o);
    

    GetAttr(msg->dest, aHidd_BitMap_DrawMode, &mode);
    
    EnterFunc(bug("X11Gfx.BitMap::CopyBox( %d,%d to %d,%d of dim %d,%d\n",
    	msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
	
    if (o != msg->dest)
    {

    	GetAttr(msg->dest, aHidd_X11BitMap_Drawable, (IPTR *)&dest);
	
	if (0 == dest)
	{
	    /* The destination object is no X11 bitmap, onscreen nor offscreen.
	       Let the superclass do the copying in a more general way
	    */
	    DoSuperMethod(cl, o, (Msg)msg);
	    return;
	}
	
    }
    else
    {
    	dest = DRAWABLE(data);
    }

LX11

    XSetFunction(data->display, data->gc, mode);

    XCopyArea(data->display
    	, DRAWABLE(data)	/* src	*/
	, dest			/* dest */
	, data->gc
	, msg->srcX
	, msg->srcY
	, msg->width
	, msg->height
	, msg->destX
	, msg->destY
    );
	
    XFlush(data->display);
UX11    
    ReturnVoid("X11Gfx.BitMap::CopyBox");
}

/*** BitMap::Get() *******************************************/

static VOID MNAME(get)(Class *cl, Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = INST_DATA(cl, o);
    ULONG idx;
    if (IS_X11BM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_X11BitMap_Drawable:
	    	*msg->storage = (IPTR)DRAWABLE(data);
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
