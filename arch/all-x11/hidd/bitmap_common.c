#include <exec/alerts.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

static VOID set_pixelformat(Object *bm, struct x11_staticdata *xsd)
{
    HIDDT_PixelFormat *pf = BM_PIXFMT(bm);
    
    pf->red_mask	= xsd->vi.red_mask;
    pf->green_mask	= xsd->vi.green_mask;
    pf->blue_mask	= xsd->vi.blue_mask;
    
    pf->red_shift	= xsd->red_shift;
    pf->green_shift	= xsd->green_shift;
    pf->blue_shift	= xsd->blue_shift;
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
		    /* Set X GC color */
LX11
		    XSetForeground(data->display, data->gc, tag->ti_Data);
UX11
		    break;
		    
                case aoHidd_BitMap_Background :
LX11
		    XSetBackground(data->display, data->gc, tag->ti_Data);
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

static HIDDT_Pixel MNAME(mapcolor)(Class *cl, Object *o, struct pHidd_BitMap_MapColor *msg)
{

    HIDDT_PixelFormat *pf = BM_PIXFMT(o);
    
    HIDDT_Pixel red	= msg->color->red;
    HIDDT_Pixel green	= msg->color->green;
    HIDDT_Pixel blue	= msg->color->blue;
    
    

    /* This code assumes that sizeof (HIDDT_Pixel is a multimple of sizeof(col->#?)
       which should be true for most (all ?) systems. (I have never heard
       of any system with for example 3 byte types.
    */
    
#if 0    
    red   = ((HIDDT_Pixel)red  ) << (( sizeof (HIDDT_Pixel) - sizeof (col->red  ) ) * 8);
    green = ((HIDDT_Pixel)green) << (( sizeof (HIDDT_Pixel) - sizeof (col->green) ) * 8);
    blue  = ((HIDDT_Pixel)blue ) << (( sizeof (HIDDT_Pixel) - sizeof (col->blue ) ) * 8);
    
    pixel  = (red   >> XSD(cl)->red_shift   ) & XSD(cl)->vi.red_mask;
    pixel |= (green >> XSD(cl)->green_shift ) & XSD(cl)->vi.green_mask;
    pixel |= (blue  >> XSD(cl)->blue_shift  ) & XSD(cl)->vi.blue_mask;
#endif    

    return MAP_RGB(red, green, blue, pf);
}

static VOID MNAME(unmappixel)(Class *cl, Object *o, struct pHidd_BitMap_UnmapPixel *msg)
{

    HIDDT_PixelFormat *pf = BM_PIXFMT(pf);

#if 0
    HIDDT_Pixel red;
    HIDDT_Pixel green;
    HIDDT_Pixel blue;
    

    red   = (msg->pixel & XSD(cl)->vi.red_mask  ) << XSD(cl)->red_shift;
    green = (msg->pixel & XSD(cl)->vi.green_mask) << XSD(cl)->green_shift;
    blue  = (msg->pixel & XSD(cl)->vi.blue_mask ) << XSD(cl)->blue_shift;

    msg->color->red   = red   >> ((sizeof (HIDDT_Pixel) - sizeof (msg->color->red) * 8);
    msg->color->green = green >> ((sizeof (HIDDT_Pixel) - sizeof (msg->color->red) * 8);
    msg->color->blue  = blue  >> ((sizeof (HIDDT_Pixel) - sizeof (msg->color->red) * 8);
#endif

    msg->color->red	= RED_COMP( msg->pixel, pf);
    msg->color->green	= GREEN_COMP( msg->pixel, pf);
    msg->color->blue	= BLUE_COMP( msg->pixel, pf);
}

static BOOL MNAME(setcolors)(Class *cl, Object *o, struct pHidd_BitMap_SetColors *msg)
{
#warning Does not deallocate previously allocated colors
    
    
    struct bitmap_data *data = INST_DATA(cl, o);
    
    ULONG xc_i, col_i;
    
    XColor xc;
    
    /* We assume TruColor display */
    return TRUE;
    
    
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
     HIDDT_Pixel old_fg;
     
     
     GetAttr(o, aHidd_BitMap_Foreground, &old_fg);
LX11     
     XSetForeground(data->display, data->gc, msg->pixel);
     XDrawPoint(data->display, DRAWABLE(data), data->gc, msg->x, msg->y);
     
     /* Reset GC to old value */
     XSetForeground(data->display, data->gc, old_fg);
     
     XFlush(data->display);
UX11     
     return;
}

/*********  BitMap::GetPixel()  *********************************/
static HIDDT_Pixel MNAME(getpixel)(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    HIDDT_Pixel pixel;
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
    
    return pixel;
    
    
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

static ULONG *ximage_to_buf(Object *bm
	, HIDDT_Pixel *buf, XImage *image
	, ULONG width, ULONG height, ULONG depth
	, APTR dummy)
{
    if (image->bits_per_pixel == 16)
    {
    	int i;
	UWORD *imdata = (UWORD *)image->data;
	
	i = width * height;
	while (i --)
	{
	    *buf ++ = (ULONG)*imdata ++;
	}
    }
    else
    {
    	LONG x, y;
	
	for (y = 0; y < height; y ++)
	{
	    for (x = 0; x < width; x ++)
	    {
		*buf ++ = XGetPixel(image, x, y);
	    }
	    
	}
    
    }
    
    return buf;
}


#define ABS(a) ((a) < 0 ? -(a) : a)
	

static inline UBYTE pix_to_lut(HIDDT_Pixel pixel, HIDDT_PixelLUT *plut, HIDDT_PixelFormat *pf)
{
    ULONG i, best_match;
    ULONG diff, lowest_diff = 0xFFFFFFFF;
    HIDDT_ColComp red, green, blue;
    
    red   = RED_COMP(pixel, pf);
    green = GREEN_COMP(pixel, pf);
    blue  = BLUE_COMP(pixel, pf);
   
    
    for (i = 0; i < plut->entries; i ++) {
    	register HIDDT_Pixel cur_lut = plut->pixels[i];
	
    	if (pixel == cur_lut)
		return i; /* Exact match found */
	
	/* How well does these pixels match ? */
	diff =  ABS(red   - RED_COMP(cur_lut, pf))   +
		ABS(green - GREEN_COMP(cur_lut, pf)) +
		ABS(blue  - BLUE_COMP(cur_lut, pf));
		
	if (diff < lowest_diff) {
		best_match = i;
		lowest_diff = diff;
	}
	
    }
    return best_match;
}


static UBYTE *ximage_to_buf_lut(Object *bm
	, UBYTE *buf, XImage *image
	, ULONG width, ULONG height, ULONG depth
	, struct pHidd_BitMap_GetImageLUT *msg)
	
{
    /* This one is trickier, as we have to reverse-lookup the lut.
       This costs CPU ! Maybe one could do some kind of caching here ?
       Ie. one stores the most often used RGB combinations
       in a trie and looks up this first to see if whe can find an exact match
    */
    
    HIDDT_PixelFormat *pf = BM_PIXFMT(bm);
    
    UBYTE *pixarray = msg->pixels;
    
    if (image->bits_per_pixel == 16)
    {
	UWORD *imdata = (UWORD *)image->data;
	LONG x, y;
	
	for (y = 0; y < height; y ++)
	{
	    UBYTE *buf = pixarray;
	    
	    for (x = 0; x < width; x ++)
	    {
		*buf ++ = pix_to_lut((HIDDT_Pixel)*imdata, msg->pixlut, pf);
	    
		imdata ++;
	    }
	    
	    pixarray += msg->modulo;
	    
	}
    }
    else
    {
    	LONG x, y;
	
	for (y = 0; y < height; y ++)
	{
	    UBYTE *buf = pixarray;
	    for (x = 0; x < width; x ++)
	    {
		*buf ++ = pix_to_lut((HIDDT_Pixel)XGetPixel(image, x, y), msg->pixlut, pf);;
	    }
	    pixarray += msg->modulo;
	    
	}
    
    }
    
    return pixarray;
     
}    

#if USE_XSHM

static void getimage_xshm(Class *cl, Object *o
	, LONG x, LONG y
	, ULONG width, ULONG height
	, APTR pixarray
	, APTR (*fromimage_func)()
	, APTR fromimage_data)
{
     

    ULONG depth;
    struct bitmap_data *data;
    XImage *image;
    ULONG  bperline;
    
    ULONG lines_to_copy;
    LONG ysize;
    LONG current_y;
    LONG maxlines;

	
    data = INST_DATA(cl, o);
    GetAttr(o, aHidd_BitMap_Depth, &depth);

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

LX11
    image = create_xshm_ximage(data->display
    	, DefaultVisual(data->display, data->screen)
	, depth
	, ZPixmap
	, width
	, height
	, XSD(cl)->xshm_info
    );
UX11    
    if (!image)
    	ReturnVoid("X11Gfx.BitMap::PutImage(XShmCreateImage failed)");
    bperline = image->bytes_per_line;
    
    /* Calculate how many scanline can be stored in the buffer */
    maxlines = XSHM_MEMSIZE / image->bytes_per_line; 
    
    if (0 == maxlines)
    {
    	kprintf("ALERT !!! NOT ENOUGH MEMORY TO READ A COMPLETE SCANLINE\n");
    	kprintf("THROUGH XSHM IN X11GF X HIDD !!!\n");
	Alert(AT_DeadEnd);
    }
    
    current_y = 0;
    ysize = image->height;
    
    
    ObtainSemaphore(&XSD(cl)->shm_sema);

    while (ysize)
    {
	/* Get some more pixels from the Ximage */
	
        lines_to_copy = MIN(maxlines, ysize);
	
	ysize -= lines_to_copy;
	image->height = lines_to_copy;
LX11	
	get_xshm_ximage(data->display
		, DRAWABLE(data)
		, image
		, x
		, y + current_y
	);
	current_y += lines_to_copy;
UX11
	pixarray = fromimage_func(o, pixarray, image, image->width, lines_to_copy, depth, fromimage_data);
	
    } /* while (pixels left to copy) */
    
    ReleaseSemaphore(&XSD(cl)->shm_sema);

LX11
	destroy_xshm_ximage(image);    
UX11    

    return;

}

#else
static void getimage_xlib(Class *cl, Object *o
	, LONG x, LONG y
	, ULONG width, ULONG height
	, APTR pixels
	, APTR (*fromimage_func)()
	, APTR fromimage_data)
{
    ULONG *pixarray = (ULONG *)pixels;
    struct bitmap_data *data;
    XImage *image;
    ULONG depth;
    
 
    data = INST_DATA(cl, o);

    GetAttr(o, aHidd_BitMap_Depth, &depth);
LX11
    image = XGetImage(data->display
    	, DRAWABLE(data)
	, x, y
	, width, height
	, AllPlanes
	, ZPixmap);
UX11	
    if (!image)
    	return;
	
	
    fromimage_func(o, pixarray, image
    	, width, height
	, depth, fromimage_data);
	
LX11    
    XDestroyImage(image);
UX11    
    
    return;
}    

#endif

static VOID MNAME(getimage)(Class *cl, Object *o, struct pHidd_BitMap_GetImage *msg)
{

    #if USE_XSHM
    getimage_xshm(cl, o
    	, msg->x, msg->y
	, msg->width, msg->height
	, msg->pixels
	, (APTR (*)())ximage_to_buf
	, NULL
    );
	
    
    #else
    

    getimage_xlib(cl, o
    	, msg->x, msg->y
	, msg->width, msg->height
	, msg->pixels
	, (APTR (*)())ximage_to_buf
	, NULL
    );
    
    #endif
        
	
    return;
}


static VOID MNAME(getimagelut)(Class *cl, Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{

#if USE_XSHM
    getimage_xshm(cl, o
    	, msg->x, msg->y
	, msg->width, msg->height
	, msg->pixels
	, (APTR (*)())ximage_to_buf_lut
	, msg
    );
	
    
#else
    

    getimage_xlib(cl, o
    	, msg->x, msg->y
	, msg->width, msg->height
	, msg->pixels
	, (APTR (*)())ximage_to_buf_lut
	, msg
    );
    
#endif
	
    return;
}


#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*********  BitMap::PutImage()  *************************************/


static ULONG *buf_to_ximage(Object *bm
	, HIDDT_Pixel *buf, XImage *image
	, ULONG width, ULONG height, ULONG depth
	, APTR dummy
)
{
    if (image->bits_per_pixel == 16)
    {
#if 1
	/* sg */
	
    	LONG x, y;
	UWORD *imdata = (UWORD *)image->data;
	
	for (y = 0; y < height; y ++)
	{
	    for (x = 0; x < width; x ++)
	    {
		*imdata ++ = (UWORD)*buf ++;
		
	    }
	    imdata += ((image->bytes_per_line / 2) - width); /*sg*/
	}

	
	
#else	
    	int i;
	UWORD *imdata = (UWORD *)image->data;
	i = width * height;
	while (i --)
	{
	    *imdata ++ = (UWORD)*buf ++;
	}
#endif
    }
    else
    {
    	LONG x, y;
	
	for (y = 0; y < height; y ++)
	{
	    for (x = 0; x < width; x ++)
	    {
		XPutPixel(image, x, y, *buf ++);
	    }
	    
	}
    
    }
    
    return buf;
}



static UBYTE *buf_to_ximage_lut(Object *bm
	, UBYTE *pixarray, XImage *image
	, ULONG width, ULONG height, ULONG depth
	, struct pHidd_BitMap_PutImageLUT *msg
)
{
    HIDDT_Pixel *lut = msg->pixlut->pixels;
    
    if (image->bits_per_pixel == 16)
    {
    	LONG x, y;
	UWORD *imdata = (UWORD *)image->data;
	
	for (y = 0; y < height; y ++)
	{
	    UBYTE *buf = pixarray;
	    for (x = 0; x < width; x ++)
	    {
		*imdata ++ = (UWORD)lut[*buf ++];
		
	    }
	    pixarray += msg->modulo;
	    imdata += ((image->bytes_per_line / 2) - width); /*sg*/
	}
    }
    else
    {
    	LONG x, y;
	
	for (y = 0; y < height; y ++)
	{
	    UBYTE *buf = pixarray;
	    for (x = 0; x < width; x ++)
	    {
		XPutPixel(image, x, y, lut[*buf ++]);
	    }
	    
	    pixarray += msg->modulo;
	    
	}
    
    }
    
    return pixarray;
}


#if USE_XSHM
static void putimage_xshm(Class *cl, Object *o
	, LONG x, LONG y
	, ULONG width, ULONG height
	, APTR pixarray
	, APTR (*toimage_func)()
	, APTR toimage_data)
{

    ULONG mode, depth;
    struct bitmap_data *data;
    XImage *image;
    ULONG  bperline;
    
    ULONG lines_to_copy;
    LONG ysize;
    LONG current_y;
    LONG maxlines;

	
    data = INST_DATA(cl, o);
    GetAttr(o, aHidd_BitMap_DrawMode, &mode);
    GetAttr(o, aHidd_BitMap_Depth, &depth);


#define MIN(a, b) (((a) < (b)) ? (a) : (b))


LX11
    image = create_xshm_ximage(data->display
    	, DefaultVisual(data->display, data->screen)
	, depth
	, ZPixmap
	, width
	, height
	, XSD(cl)->xshm_info
    );
UX11    
    if (!image)
    	ReturnVoid("X11Gfx.BitMap::PutImage(XShmCreateImage failed)");
    bperline = image->bytes_per_line;
    
    /* Calculate how many scanline can be stored in the buffer */
    maxlines = XSHM_MEMSIZE / image->bytes_per_line; 
    
    
    if (0 == maxlines)
    {
    	kprintf("ALERT !!! NOT ENOUGH MEMORY TO WRITE A COMPLETE SCANLINE\n");
    	kprintf("THROUGH XSHM IN X11GF X HIDD !!!\n");
	Alert(AT_DeadEnd);
    }
    
    current_y = 0;
    ysize = image->height;
    
//    kprintf("Max lines: %d, ysize: %d\n", maxlines, ysize);
    
    ObtainSemaphore(&XSD(cl)->shm_sema);

    while (ysize)
    {
	/* Get some more pixels from the HIDD */
	
        lines_to_copy = MIN(maxlines, ysize);
	
	ysize -= lines_to_copy;
	image->height = lines_to_copy;
	
	pixarray = toimage_func(o, pixarray, image, image->width, lines_to_copy, depth, toimage_data);
	
LX11	

	put_xshm_ximage(data->display
		, DRAWABLE(data)
		, data->gc
		, image
		, 0, 0
		, x
		, y + current_y
		, image->width, lines_to_copy
		, FALSE
	);
	current_y += lines_to_copy;

UX11
	
    } /* while (pixels left to copy) */
    
// kprintf("Finished copying\n");    
    
    ReleaseSemaphore(&XSD(cl)->shm_sema);

LX11
	destroy_xshm_ximage(image);    
UX11    

    return;

}

#else
static void putimage_xlib(Class *cl, Object *o
	, LONG x, LONG y
	, ULONG width, ULONG height
	, APTR pixarray
	, APTR (*toimage_func)()
	, APTR toimage_data)
{

    ULONG mode, depth;
    struct bitmap_data *data;
    XImage *image;
    ULONG  bperline;

	
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
	, width, height
	, 32
	, 0
    );
UX11	
    if (!image)
    	ReturnVoid("X11Gfx.BitMap::PutImage(XCreateImage failed)");
	    
    bperline	= image->bytes_per_line;
	
    image->data = (char *)malloc((size_t)height * bperline);
    if (!image->data)
    {
LX11	
	XFree(image);
UX11	    
    	ReturnVoid("X11Gfx.BitMap::PutImage(malloc(image data) failed)");
    }
    
    toimage_func(o, pixarray, image, width, height, depth, toimage_data);
	
LX11
   XSetFunction(data->display, data->gc, mode);
   XPutImage(data->display
    		, DRAWABLE(data)
		, data->gc
		, image
		, 0, 0
		, x, y
		, width, height
   );
    XFlush(data->display);
UX11    
    free(image->data);
    
LX11    
    XFree(image);
UX11   

    return;
}
	

#endif

static VOID MNAME(putimage)(Class *cl, Object *o, struct pHidd_BitMap_PutImage *msg)
{
    EnterFunc(bug("X11Gfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
#if USE_XSHM
    putimage_xshm(cl, o
    	, msg->x, msg->y
	, msg->width, msg->height
	, msg->pixels
	, (APTR (*)()) buf_to_ximage
	, NULL
    );

#else

    putimage_xlib(cl, o
    	, msg->x, msg->y
	, msg->width, msg->height
	, msg->pixels
	, (APTR (*)()) buf_to_ximage
	, NULL
    );

#endif

    ReturnVoid("X11Gfx.BitMap::PutImage");
}

/*********  BitMap::PutImageLUT()  *************************************/


static VOID MNAME(putimagelut)(Class *cl, Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    EnterFunc(bug("X11Gfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
#if USE_XSHM
    putimage_xshm(cl, o
    	, msg->x, msg->y
	, msg->width, msg->height
	, msg->pixels
	, (APTR (*)())buf_to_ximage_lut
	, msg
    );

#else

    putimage_xlib(cl, o
    	, msg->x, msg->y
	, msg->width, msg->height
	, msg->pixels
	, (APTR (*)())buf_to_ximage_lut
	, msg
    );

#endif

    ReturnVoid("X11Gfx.BitMap::PutImageLUT");
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
    HIDDT_Pixel fg, bg;
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
    
    if (0 != d)
    {

// kprintf("D");
LX11    
	XSetForeground(data->display, data->gc, fg);
UX11	
    	if (cemd & vHidd_GC_ColExp_Opaque)  
	{
//	    kprintf("XCP\n");
LX11
	    XSetBackground(data->display, data->gc, bg);
	    
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
	    	    XPutPixel(dest_im, x, y, fg);
		
	    	}
	    	else
	    	{
		    if (cemd & vHidd_GC_ColExp_Opaque)
		    {
			XPutPixel(dest_im, x, y, bg);
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

LX11
    XFlush(data->display);
UX11 
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



