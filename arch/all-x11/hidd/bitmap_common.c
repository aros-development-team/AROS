/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/alerts.h>
#include <aros/macros.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

/* stegerg: maybe more safe, even if Unix malloc is used and not AROS malloc */
#define NO_MALLOC   	1

#define DO_ENDIAN_FIX 	1 /* fix if X11 server running on remote server with different endianess */

/****************************************************************************************/

#if DO_ENDIAN_FIX

/****************************************************************************************/

#if AROS_BIG_ENDIAN
#define NEEDS_ENDIAN_FIX(image) (((image)->bits_per_pixel >= 15) && ((image)->byte_order != MSBFirst))
#define SWAP16(x) AROS_WORD2LE(x)
#define SWAP32(x) AROS_LONG2LE(x)
#define AROS_BYTEORDER MSBFirst
#else
#define NEEDS_ENDIAN_FIX(image) (((image)->bits_per_pixel >= 15) && ((image)->byte_order != LSBFirst))
#define SWAP16(x) AROS_WORD2BE(x)
#define SWAP32(x) AROS_LONG2BE(x)
#define AROS_BYTEORDER LSBFirst
#endif

#if 0 /* stegerg: to test above stuff*/
#define NEEDS_ENDIAN_FIX(image) ((image)->bits_per_pixel >= 15)
#define AROS_BYTEORDER MSBFirst
#define SWAP16(x) AROS_WORD2BE(x)
#define SWAP32(x) AROS_LONG2BE(x)
#endif

/****************************************************************************************/

static void SwapImageEndianess(XImage *image)
{
    LONG  x, y, height, width, bpp;    
    UBYTE *imdata = (UBYTE *)image->data;

    width = image->width;
    height = image->height;
    bpp = (image->bits_per_pixel + 7) / 8;
    
    for (y = 0; y < height; y ++)
    {
    	switch(bpp)
	{
	    case 2:
    		for (x = 0; x < width; x++, imdata += 2)
    		{
    		    UWORD pix = *(UWORD *)imdata;
		    
		    pix = SWAP16(pix);
		    
		    *(UWORD *)imdata = pix;
    		}
	    	imdata += (image->bytes_per_line - width * 2);
		break;

    	    case 3:
    		for (x = 0; x < width; x++, imdata += 3)
    		{
    		    UBYTE pix1 = imdata[0];
    		    UBYTE pix3 = imdata[2];
		    
		    imdata[0] = pix3;
		    imdata[2] = pix1;
    		}
	    	imdata += (image->bytes_per_line - width * 3);
		break;
		
	    case 4:
    		for (x = 0; x < width; x++, imdata += 4)
    		{
    		    ULONG pix = *(ULONG *)imdata;
		    
		    pix = SWAP32(pix);
		    
		    *(ULONG *)imdata = pix;
    		}
	    	imdata += (image->bytes_per_line - width * 4);
		break;
		
	} /* switch(bpp) */
	
    } /* for (y = 0; y < height; y ++) */
    
    image->byte_order = AROS_BYTEORDER;    
}

/****************************************************************************************/

#endif /* DO_ENDIAN_FIX */

/****************************************************************************************/

static BOOL MNAME(setcolors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat 	*pf;    
    ULONG   	    	 xc_i, col_i;
        
    pf = BM_PIXFMT(o);
    
    if (vHidd_ColorModel_StaticPalette == HIDD_PF_COLMODEL(pf) ||
    	vHidd_ColorModel_TrueColor == HIDD_PF_COLMODEL(pf) )
    {	 
	 /* Superclass takes care of this case */
	 
	 return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }

    /* Ve have a vHidd_GT_Palette bitmap */    
	
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) return FALSE;
    
    if (data->flags & BMDF_COLORMAP_ALLOCED)
    {
    	LOCK_X11	

	for ( xc_i = msg->firstColor, col_i = 0;
    	      col_i < msg->numColors; 
	      xc_i ++, col_i ++ )
	{
            XColor xcol;

	    xcol.red   = msg->colors[col_i].red;
	    xcol.green = msg->colors[col_i].green;
	    xcol.blue  = msg->colors[col_i].blue;
	    xcol.pad   = 0;
	    xcol.pixel = xc_i;
	    xcol.flags = DoRed | DoGreen | DoBlue;
	    
	    XStoreColor(data->display, data->colmap, &xcol);

	}
	
    	UNLOCK_X11	
	
    } /* if (data->flags & BMDF_COLORMAP_ALLOCED) */
    

    return TRUE;
}

/****************************************************************************************/

static VOID MNAME(putpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
     
    LOCK_X11
     
    XSetForeground(data->display, data->gc, msg->pixel);
    XDrawPoint(data->display, DRAWABLE(data), data->gc, msg->x, msg->y);
    XFlush(data->display);

    UNLOCK_X11
}

/****************************************************************************************/

static HIDDT_Pixel MNAME(getpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel     	pixel = -1;    
    XImage  	       *image;

    LOCK_X11
    
    XSync(data->display, False);
    
    image = XGetImage(data->display, DRAWABLE(data), msg->x, msg->y,
    	    	      1, 1, AllPlanes, ZPixmap);
    
    if (image)
    {
    	pixel = XGetPixel(image, 0, 0);    
    	XDestroyImage(image);
    }
    
    UNLOCK_X11    
    
    return pixel;
       
}

/****************************************************************************************/

static ULONG MNAME(drawpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    XGCValues 	    	gcval;
    
    gcval.function = GC_DRMD(msg->gc);
    gcval.foreground = GC_FG(msg->gc);
    gcval.background = GC_BG(msg->gc);

    LOCK_X11
    XChangeGC(data->display, data->gc, GCFunction | GCForeground | GCBackground, &gcval);    
    XDrawPoint(data->display, DRAWABLE(data), data->gc, msg->x, msg->y);
    XFlush(data->display); /* stegerg: uncommented */
    UNLOCK_X11    
    
    return 0;    
}

/****************************************************************************************/

static VOID MNAME(fillrect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    XGCValues 	    	gcval;
        
    EnterFunc(bug("X11Gfx.BitMap::FillRect(%d,%d,%d,%d)\n",
    	          msg->minX, msg->minY, msg->maxX, msg->maxY));
	    
    D(bug("Drawmode: %d\n", mode));
    
    gcval.function = GC_DRMD(msg->gc);
    gcval.foreground = GC_FG(msg->gc);
    gcval.background = GC_BG(msg->gc);

    LOCK_X11
    XChangeGC(data->display, data->gc, GCFunction | GCForeground | GCBackground, &gcval);
    
    XFillRectangle(data->display, DRAWABLE(data), data->gc,
    	    	   msg->minX, msg->minY,
		   msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1);

    XFlush(data->display);
    UNLOCK_X11    
    
    ReturnVoid("X11Gfx.BitMap::FillRect");    
}

/****************************************************************************************/

static ULONG *ximage_to_buf(OOP_Class *cl, OOP_Object *bm, HIDDT_Pixel *buf,
    	    	    	    XImage *image, ULONG width, ULONG height, ULONG depth,
	    	    	    struct pHidd_BitMap_GetImage *msg)
{
    switch (msg->pixFmt)
    {
	case vHidd_StdPixFmt_Native:
	    {
        	UBYTE *imdata = image->data;
		LONG   y;

		for (y = 0; y < height; y ++)
		{
		    memcpy(buf, imdata, msg->width * image->bits_per_pixel / 8);

		    imdata += image->bytes_per_line;
		    buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		}
	    }
	    break;

	case vHidd_StdPixFmt_Native32:
    	    switch (image->bits_per_pixel)
	    {	
		case 8:
		{
		    UBYTE *imdata = (UBYTE *)image->data;
		    LONG   x, y;

		    for (y = 0; y < height; y ++)
		    {
			HIDDT_Pixel *p = buf;

			for (x = 0; x < width; x ++)
			{
			    *p++ = *imdata++;
			}
		        imdata += (image->bytes_per_line - width);
			buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }
		    break;
		}

		case 16:
		{
		    UWORD *imdata = (UWORD *)image->data;
		    LONG   x, y;

		    for (y = 0; y < height; y ++)
		    {
			HIDDT_Pixel *p = buf;

			for (x = 0; x < width; x ++)
			{
			    *p++ = *imdata++;
			}
			imdata += image->bytes_per_line/2 - width;
			buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }
		    break;
		}

		case 32:
		{
		    ULONG *imdata = (ULONG *)image->data;
		    LONG   x, y;

		    for (y = 0; y < height; y ++)
		    {
			HIDDT_Pixel *p = buf;

			for (x = 0; x < width; x ++)
			{
			    *p++ = *imdata++;
			}
			imdata += image->bytes_per_line/4 - width;
			buf     = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }
		    break;
		}

		default:
		{
		    LONG x, y;

    		    LOCK_X11		    		    	    
		    for (y = 0; y < height; y ++)
		    {
	    		HIDDT_Pixel *p;

	    		p = buf;
	    		for (x = 0; x < width; x ++)
			{
			    *p++ = XGetPixel(image, x, y);
			}
			buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }		    
    		    UNLOCK_X11
		    break;
		}

	    } /* switch (image->bits_per_pixel) */

	    break;

	 default:
	 {

	    OOP_Object *srcpf, *dstpf, *gfxhidd;
	    APTR    	srcPixels = image->data, dstBuf = buf;

	    //kprintf("DEFAULT PIXEL CONVERSION\n");

	    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
	    dstpf = HIDD_Gfx_GetPixFmt(gfxhidd, msg->pixFmt);

	    OOP_GetAttr(bm, aHidd_BitMap_PixFmt, (IPTR *)&srcpf);

	    //kprintf("CALLING ConvertPixels()\n");

     	    HIDD_BM_ConvertPixels(bm, &srcPixels, (HIDDT_PixelFormat *)srcpf, 
	    	    	    	  image->bytes_per_line, &dstBuf,
				  (HIDDT_PixelFormat *)dstpf, msg->modulo,
				  width, height, NULL /* We have no CLUT */
	    );

	    //kprintf("CONVERTPIXELS DONE\n");

	    buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo * height);
	    break;
	}

    } /* switch (msg->pixFmt) */

    return buf;
}

/****************************************************************************************/

#define ABS(a) ((a) < 0 ? -(a) : a)
	
/****************************************************************************************/

static inline UBYTE pix_to_lut(HIDDT_Pixel pixel, HIDDT_PixelLUT *plut,
    	    	    	       HIDDT_PixelFormat *pf)
{
    HIDDT_ColComp   red, green, blue;
    ULONG   	    i, best_match;
    ULONG   	    diff, lowest_diff = 0xFFFFFFFF;
    
    red   = RED_COMP(pixel, pf);
    green = GREEN_COMP(pixel, pf);
    blue  = BLUE_COMP(pixel, pf);
    
    for (i = 0; i < plut->entries; i ++)
    {
    	register HIDDT_Pixel cur_lut = plut->pixels[i];
	
    	if (pixel == cur_lut)
	    return i; /* Exact match found */
	
	/* How well does these pixels match ? */
	diff =  ABS(red   - RED_COMP(cur_lut, pf))   +
		ABS(green - GREEN_COMP(cur_lut, pf)) +
		ABS(blue  - BLUE_COMP(cur_lut, pf));
		
	if (diff < lowest_diff)
	{
    	    best_match = i;
    	    lowest_diff = diff;
	}
	
    }
    
    return best_match;
}

/****************************************************************************************/

static UBYTE *ximage_to_buf_lut(OOP_Class *cl, OOP_Object *bm, UBYTE *buf,
    	    	    	    	XImage *image, ULONG width, ULONG height,
				ULONG depth, struct pHidd_BitMap_GetImageLUT *msg)	
{
    /* This one is trickier, as we have to reverse-lookup the lut.
       This costs CPU ! Maybe one could do some kind of caching here ?
       Ie. one stores the most often used RGB combinations
       in a trie and looks up this first to see if whe can find an exact match
    */
    
    HIDDT_PixelFormat 	*pf = BM_PIXFMT(bm);    
    UBYTE   	    	*pixarray = msg->pixels;
    
    if (image->bits_per_pixel == 16)
    {
	UWORD *imdata = (UWORD *)image->data;
	LONG   x, y;
	
	for (y = 0; y < height; y ++)
	{
	    UBYTE *buf = pixarray;
	    
	    for (x = 0; x < width; x ++)
	    {
		*buf ++ = pix_to_lut((HIDDT_Pixel)*imdata, msg->pixlut, pf);
	    
		imdata ++;
	    }
	    imdata += ((image->bytes_per_line / 2) - width); /*sg*/
	    
	    pixarray += msg->modulo;
	    
	}
    }
    else
    {
    	LONG x, y;

    	LOCK_X11	
	for (y = 0; y < height; y ++)
	{
	    UBYTE *buf = pixarray;
	    for (x = 0; x < width; x ++)
	    {
		*buf ++ = pix_to_lut((HIDDT_Pixel)XGetPixel(image, x, y), msg->pixlut, pf);;
	    }
	    pixarray += msg->modulo;
	    
	}
    	UNLOCK_X11
    
    }
    
    return pixarray;
     
}    

/****************************************************************************************/

#if USE_XSHM

/****************************************************************************************/

static void getimage_xshm(OOP_Class *cl, OOP_Object *o, LONG x, LONG y,
    	    	    	  ULONG width, ULONG height, APTR pixarray,
			  APTR (*fromimage_func)(), APTR fromimage_data)
{     
    struct bitmap_data  *data;
    XImage  	    	*image;
    IPTR    	    	 depth;
    ULONG   	    	 bperline;    
    ULONG   	    	 lines_to_copy;
    LONG    	    	 ysize;
    LONG    	    	 current_y;
    LONG    	    	 maxlines;
    OOP_Object      	*pf;
	
    data = OOP_INST_DATA(cl, o);
    
    OOP_GetAttr(o,  aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth,  &depth);

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

    LOCK_X11
    image = create_xshm_ximage(data->display,
    	    	    	       DefaultVisual(data->display, data->screen),
			       depth,
			       ZPixmap,
			       width,
			       height,
			       XSD(cl)->xshm_info);
    UNLOCK_X11 
       
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

    	LOCK_X11	
	get_xshm_ximage(data->display, DRAWABLE(data), image,
	    	    	x, y + current_y);
    	UNLOCK_X11

	current_y += lines_to_copy;
	
	pixarray = fromimage_func(cl, o, pixarray, image, image->width,
	    	    	    	  lines_to_copy, depth, fromimage_data);
	
    } /* while (pixels left to copy) */
    
    ReleaseSemaphore(&XSD(cl)->shm_sema);

    LOCK_X11
    destroy_xshm_ximage(image);    
    UNLOCK_X11    

    return;

}

/****************************************************************************************/

#endif

/****************************************************************************************/

static void getimage_xlib(OOP_Class *cl, OOP_Object *o, LONG x, LONG y,
    	    	    	  ULONG width, ULONG height, APTR pixels,
			  APTR (*fromimage_func)(), APTR fromimage_data)
{
    struct bitmap_data  *data;
    XImage  	    	*image;
    ULONG   	    	*pixarray = (ULONG *)pixels;
    OOP_Object      	*pf; 
    IPTR    	    	 depth;
 
    data = OOP_INST_DATA(cl, o);

    OOP_GetAttr(o,  aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth,  &depth);

    LOCK_X11
    image = XGetImage(data->display, DRAWABLE(data), x, y,
    	    	      width, height, AllPlanes, ZPixmap);
    UNLOCK_X11
    	
    if (!image)
    	return;
	
#if DO_ENDIAN_FIX
    if (NEEDS_ENDIAN_FIX(image))
    {
    	SwapImageEndianess(image);
	
    	LOCK_X11
        XInitImage(image);
    	UNLOCK_X11
    }    
#endif
 	
    fromimage_func(cl, o, pixarray, image, width, height, depth, fromimage_data);
	
    LOCK_X11    
    XDestroyImage(image);
    UNLOCK_X11    
    
    return;
}    

/****************************************************************************************/

static VOID MNAME(getimage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{    
#if USE_XSHM
    if (XSD(cl)->use_xshm)
    {
	getimage_xshm(cl, o, msg->x, msg->y, msg->width, msg->height,
	    	      msg->pixels, (APTR (*)())ximage_to_buf, msg);
    }
    else
#endif
    {
	 getimage_xlib(cl, o, msg->x, msg->y, msg->width, msg->height,
	    	       msg->pixels, (APTR (*)())ximage_to_buf, msg);
    }
}

/****************************************************************************************/

static VOID MNAME(getimagelut)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
#if USE_XSHM
    if (XSD(cl)->use_xshm)
    {
	getimage_xshm(cl, o, msg->x, msg->y, msg->width, msg->height,
	    	      msg->pixels, (APTR (*)())ximage_to_buf_lut, msg);
    }
    else
#endif
    {
	getimage_xlib(cl, o, msg->x, msg->y, msg->width, msg->height,
	    	      msg->pixels, (APTR (*)())ximage_to_buf_lut, msg);
    }    	
}

/****************************************************************************************/


#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

static ULONG *buf_to_ximage(OOP_Class *cl, OOP_Object *bm, HIDDT_Pixel *buf,
    	    	    	    XImage *image, ULONG width, ULONG height, ULONG depth,
			    struct pHidd_BitMap_PutImage *msg)
{
    switch (msg->pixFmt)
    {
	case vHidd_StdPixFmt_Native:
	    {
        	UBYTE *imdata = image->data;
		LONG   y;

		for (y = 0; y < height; y ++)
		{
		    memcpy(imdata, buf, msg->width * image->bits_per_pixel / 8);

		    imdata += image->bytes_per_line;
		    buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		}
	    }
	    break;

	case vHidd_StdPixFmt_Native32:
    	    switch (image->bits_per_pixel)
	    {
    	    	case 8:
		{		    
		    UBYTE *imdata = (UBYTE *)image->data;
		    LONG   x, y;

		    for (y = 0; y < height; y ++)
		    {
			HIDDT_Pixel *p = buf;
			
			for (x = 0; x < width; x ++)
			{
			    *imdata ++ = (UBYTE)*p++;
			}
			imdata += image->bytes_per_line - width; 
		        buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }
		    break;
		}
		    
		case 16:
		{
		    UWORD *imdata = (UWORD *)image->data;
		    LONG   x, y;

		    for (y = 0; y < height; y ++)
		    {
			HIDDT_Pixel *p = buf;
			
			for (x = 0; x < width; x ++)
			{
			    *imdata ++ = (UWORD)*p ++;
			}
			imdata += image->bytes_per_line/2 - width; 
		        buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }
		    break;
		}


		case 24:
		{
		    HIDDT_PixelFormat 	*pf;
		    UBYTE   	    	*imdata = image->data;
		    LONG    	    	 x, y;

		    pf = BM_PIXFMT(bm);

		    for (y = 0; y < height; y ++)
		    {

	    		HIDDT_Pixel *p = buf;

	    		for (x = 0; x < width; x ++)
			{
			     register HIDDT_Pixel pix;

			     pix = *p ++;
    	    		#if (AROS_BIG_ENDIAN == 1)
	    		    *imdata ++ = pix >> 16;
			    *imdata ++ = (pix & pf->green_mask) >> 8;
			    *imdata ++ = (pix & pf->blue_mask);
    	    		#else
			    *imdata ++ = (pix & pf->blue_mask);
			    *imdata ++ = (pix & pf->green_mask) >> 8;
			    *imdata ++ = pix >> 16;
    	    		#endif
			}
			imdata += image->bytes_per_line - width * 3;		
		        buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }
		    break;
		}

		case 32:
		{
		    ULONG *imdata = (ULONG *)image->data;
		    LONG   x, y;

		    for (y = 0; y < height; y ++)
		    {
			HIDDT_Pixel *p = buf;
			
			for (x = 0; x < width; x ++)
			{
			    *imdata ++ = (ULONG)*p ++;
			}
			imdata += image->bytes_per_line/4 - width; 
		        buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }
		    break;
		}

		default:
		{
		    LONG x, y;

    		    LOCK_X11	    
		    for (y = 0; y < height; y ++) 
    	    	    {
	    		HIDDT_Pixel *p;

	    		p = buf;
	    		for (x = 0; x < width; x ++)
			{
			     XPutPixel(image, x, y, *p ++);
			}
		        buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo);
		    }
    		    UNLOCK_X11
		    break;
		}

	    } /* switch (image->bits_per_pixel) */

	    break;


	 default:
	 {    
	    OOP_Object *srcpf, *dstpf, *gfxhidd;
	    APTR    	srcPixels = buf, dstBuf = image->data;

	    //kprintf("DEFAULT PIXEL CONVERSION\n");

	    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
	    srcpf = HIDD_Gfx_GetPixFmt(gfxhidd, msg->pixFmt);

	    OOP_GetAttr(bm, aHidd_BitMap_PixFmt, (IPTR *)&dstpf);

	    //kprintf("CALLING ConvertPixels()\n");

     	    HIDD_BM_ConvertPixels(bm, &srcPixels, (HIDDT_PixelFormat *)srcpf,
	    	    	    	  msg->modulo, &dstBuf, (HIDDT_PixelFormat *)dstpf,
				  image->bytes_per_line, width, height, NULL); /* We have no CLUT */

	    //kprintf("CONVERTPIXELS DONE\n");

            buf = (HIDDT_Pixel *)((UBYTE *)buf + msg->modulo * height);
	    break;
	}

    } /* switch (msg->pixFmt) */
    
    return buf;
}


/****************************************************************************************/

static UBYTE *buf_to_ximage_lut(OOP_Class *cl, OOP_Object *bm, UBYTE *pixarray,
    	    	    	    	XImage *image, ULONG width, ULONG height, ULONG depth,
				struct pHidd_BitMap_PutImageLUT *msg)
{
    HIDDT_Pixel *lut = msg->pixlut->pixels;
    
    switch(image->bits_per_pixel)
    {
	case 8:
	{
	    UBYTE *imdata = (UBYTE *)image->data;
    	    LONG   x, y;

	    for (y = 0; y < height; y ++)
	    {
		UBYTE *buf = pixarray;
		
		for (x = 0; x < width; x ++)
		{
		    *imdata ++ = (UBYTE)lut[*buf ++];
		}
		pixarray += msg->modulo;
		imdata += (image->bytes_per_line  - width); /*sg*/
	    }
	    break;
    	}

	case 16:
	{
	    UWORD *imdata = (UWORD *)image->data;
    	    LONG   x, y;

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
	    break;
    	}

	case 32:
	{
	    ULONG *imdata = (ULONG *)image->data;
    	    LONG   x, y;

	    for (y = 0; y < height; y ++)
	    {
		UBYTE *buf = pixarray;
		
		for (x = 0; x < width; x ++)
		{
		    *imdata ++ = (ULONG)lut[*buf ++];

		}
		pixarray += msg->modulo;
		imdata += ((image->bytes_per_line / 4) - width); /*sg*/
	    }
	    break;
    	}
	
	default:
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
    	    break;
	    
	}
	
    } /* switch(image->bits_per_pixel) */
    
    return pixarray;
}

/****************************************************************************************/

#if USE_XSHM

/****************************************************************************************/

static void putimage_xshm(OOP_Class *cl, OOP_Object *o, OOP_Object *gc,
    	    	    	  LONG x, LONG y, ULONG width, ULONG height,
			  APTR pixarray, APTR (*toimage_func)(), APTR toimage_data)
{

    struct bitmap_data  *data;
    XImage  	    	*image;
    ULONG   	    	 bperline;
    IPTR    	    	 depth;    
    ULONG   	    	 lines_to_copy;
    LONG    	    	 ysize;
    LONG    	    	 current_y;
    LONG    	    	 maxlines;
    OOP_Object      	*pf;
	
    data = OOP_INST_DATA(cl, o);

    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

    LOCK_X11
    image = create_xshm_ximage(data->display,
    	    	    	       DefaultVisual(data->display, data->screen),
			       depth,
			       ZPixmap,
			       width,
			       height,
			       XSD(cl)->xshm_info);
    UNLOCK_X11 
       
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
        
    ObtainSemaphore(&XSD(cl)->shm_sema);

    while (ysize)
    {
	/* Get some more pixels from the HIDD */
	
        lines_to_copy = MIN(maxlines, ysize);
	
	ysize -= lines_to_copy;
	image->height = lines_to_copy;
	
	pixarray = toimage_func(cl, o, pixarray, image, image->width,
	    	    	    	lines_to_copy, depth, toimage_data);
	
    	LOCK_X11	
	XSetFunction(data->display, data->gc, GC_DRMD(gc));

	put_xshm_ximage(data->display,
	    	    	DRAWABLE(data),
			data->gc,
			image,
			0, 0,
			x, y + current_y,
			image->width, lines_to_copy,
			FALSE);

    	UNLOCK_X11
	
	current_y += lines_to_copy;
	
    } /* while (pixels left to copy) */
        
    ReleaseSemaphore(&XSD(cl)->shm_sema);

    LOCK_X11
    XFlush(data->display); /* stegerg: added */
    destroy_xshm_ximage(image);    
    UNLOCK_X11    

    return;

}

/****************************************************************************************/

#endif

/****************************************************************************************/

static void putimage_xlib(OOP_Class *cl, OOP_Object *o, OOP_Object *gc,
    	    	    	  LONG x, LONG y, ULONG width, ULONG height,
			  APTR pixarray, APTR (*toimage_func)(), APTR toimage_data)
{

    struct bitmap_data  *data;
    XImage  	    	*image;
    ULONG   	    	 bperline;
    IPTR    	    	 depth;
    OOP_Object      	*pf;
	
    data = OOP_INST_DATA(cl, o);
    
    OOP_GetAttr(o,  aHidd_BitMap_PixFmt, (IPTR *)&pf);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth,  &depth);

    LOCK_X11	
    image = XCreateImage(data->display,
    	    	    	 DefaultVisual(data->display, data->screen),
			 depth,
			 ZPixmap,
			 0,
			 NULL,
			 width,
			 height,
			 32,
			 0);
    UNLOCK_X11	
    
    if (!image)
    	ReturnVoid("X11Gfx.BitMap::PutImage(XCreateImage failed)");

#if DO_ENDIAN_FIX
    if (NEEDS_ENDIAN_FIX(image))
    {
    	image->byte_order = AROS_BYTEORDER;
	
    	LOCK_X11
	XInitImage(image);
    	UNLOCK_X11
    }
#endif
	    
    bperline	= image->bytes_per_line;

#if NO_MALLOC
    image->data = (char *)AllocVec((size_t)height * bperline, MEMF_PUBLIC);
#else	
    image->data = (char *)malloc((size_t)height * bperline);
#endif
    
    if (!image->data)
    {
    	LOCK_X11	
	XFree(image);
    	UNLOCK_X11
		    
    	ReturnVoid("X11Gfx.BitMap::PutImage(malloc(image data) failed)");
    }
    
    toimage_func(cl, o, pixarray, image, width, height, depth, toimage_data);
	
    LOCK_X11
    XSetFunction(data->display, data->gc, GC_DRMD(gc));
    XPutImage(data->display, DRAWABLE(data), data->gc, image,
    	      0, 0, x, y, width, height);
    XFlush(data->display);
    UNLOCK_X11 
    
#if NO_MALLOC
    FreeVec(image->data);
#else   
    free(image->data);
#endif
    
    LOCK_X11    
    XFree(image);
    UNLOCK_X11   

}

/****************************************************************************************/
	
static VOID MNAME(putimage)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    EnterFunc(bug("X11Gfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	    	  msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
#if USE_XSHM
    if (XSD(cl)->use_xshm)
    {
	putimage_xshm(cl, o, msg->gc, msg->x, msg->y,
	    	      msg->width, msg->height, msg->pixels,
		      (APTR (*)()) buf_to_ximage, msg);
    }
    else
#endif
    {
	putimage_xlib(cl, o, msg->gc, msg->x, msg->y,
	    	      msg->width, msg->height, msg->pixels,
		      (APTR (*)()) buf_to_ximage, msg);
    }

    ReturnVoid("X11Gfx.BitMap::PutImage");
}

/****************************************************************************************/

static VOID MNAME(putimagelut)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    EnterFunc(bug("X11Gfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
    	    	  msg->pixels, msg->x, msg->y, msg->width, msg->height));
	
#if USE_XSHM
    if (XSD(cl)->use_xshm)
    {
	putimage_xshm(cl, o, msg->gc, msg->x, msg->y,
	    	      msg->width, msg->height, msg->pixels,
		      (APTR (*)())buf_to_ximage_lut, msg);
    }
    else
#endif
    {
	putimage_xlib(cl, o, msg->gc, msg->x, msg->y,
	    	      msg->width, msg->height, msg->pixels,
		      (APTR (*)())buf_to_ximage_lut, msg);
    }

    ReturnVoid("X11Gfx.BitMap::PutImageLUT");
}

/****************************************************************************************/

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

static VOID MNAME(blitcolorexpansion)(OOP_Class *cl, OOP_Object *o,
    	    	    	    	      struct pHidd_BitMap_BlitColorExpansion *msg)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    XImage  	    	*dest_im;
    HIDDT_Pixel     	 fg, bg;
    ULONG   	    	 cemd;
    LONG    	    	 x, y;    
    Drawable 	    	 d = 0;
    
    EnterFunc(bug("X11Gfx.BitMap::BlitColorExpansion(%p, %d, %d, %d, %d, %d, %d)\n",
    	    	  msg->srcBitMap, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
    
    
    OOP_GetAttr(msg->srcBitMap, aHidd_X11BitMap_Drawable, (IPTR *)&d);
    
    if (0 == d)
    {
    	/* We know nothing about the source bitmap. Let the superclass handle this */
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	return;
    }
    
    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    cemd = GC_COLEXP(msg->gc);

    if (0 != d)
    {
    	LOCK_X11    

	XSetForeground(data->display, data->gc, fg);
		
    	if (cemd & vHidd_GC_ColExp_Opaque)  
	{
	    XSetBackground(data->display, data->gc, bg);
	    XSetFunction(data->display, data->gc, GXcopy);
	    
	    XCopyPlane(data->display, d, DRAWABLE(data), data->gc,
	    	       msg->srcX, msg->srcY, msg->width, msg->height,
		       msg->destX, msg->destY, 0x01);
	}
	else
	{
	    /* Do transparent blit */
	    
	    XGCValues val;
	    
	    val.stipple		= d;
	    val.ts_x_origin	= msg->destX - msg->srcX;
	    val.ts_y_origin	= msg->destY - msg->srcY;
	    val.fill_style	= FillStippled;

	    XSetFunction(data->display, data->gc, GC_DRMD(msg->gc));
	    
	    XChangeGC(data->display, data->gc,
	    	      GCStipple|GCTileStipXOrigin|GCTileStipYOrigin|GCFillStyle,
		      &val);
		      
	    XFillRectangle(data->display, DRAWABLE(data), data->gc,
	    	    	   msg->destX, msg->destY, msg->width, msg->height);
	    
	    XSetFillStyle(data->display, data->gc, FillSolid);

	}
	
    	UNLOCK_X11	

    }
    else
    {
    	/* We know nothing about the format of the source bitmap
	   an must get single pixels
	*/

    	LOCK_X11    
	dest_im = XGetImage(data->display, DRAWABLE(data),
	    	    	    msg->destX, msg->destY, msg->width, msg->height,
			    AllPlanes, ZPixmap);    	
    	UNLOCK_X11   
	 
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

    	LOCK_X11    
	XSetFunction(data->display, data->gc, GC_DRMD(msg->gc));
	XPutImage(data->display, DRAWABLE(data), data->gc, dest_im,
	    	  0, 0, msg->destX, msg->destY, msg->width, msg->height);
    	XDestroyImage(dest_im);
    	UNLOCK_X11
    }

    LOCK_X11
    XFlush(data->display);
    UNLOCK_X11 
    
    ReturnVoid("X11Gfx.BitMap::BlitColorExpansion");
}

/****************************************************************************************/

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

static VOID MNAME(get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG   	    	idx;
    
    if (IS_X11BM_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_X11BitMap_Drawable:
	    	*msg->storage = (IPTR)DRAWABLE(data);
		break;

	    case aoHidd_X11BitMap_MasterWindow:
	        *msg->storage = (IPTR)data->masterxwindow;
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
    
}

/****************************************************************************************/

VOID MNAME(drawline)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    OOP_Object      	*gc = msg->gc;
    
    if (GC_LINEPAT(gc) != (UWORD)~0)
    {
    	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	
	return;
    }
    
    LOCK_X11
    
    if (GC_DOCLIP(gc))
    {
    	XRectangle cr;
	
	cr.x = GC_CLIPX1(gc);
	cr.y = GC_CLIPY1(gc);
	cr.width  = GC_CLIPX2(gc) - cr.x + 1;
	cr.height = GC_CLIPY2(gc) - cr.y + 1;
    
    	XSetClipRectangles(data->display, data->gc,
	    	    	   0, 0, &cr, 1, Unsorted);
    }
    
    XSetForeground(data->display, data->gc, GC_FG(gc));
    XSetFunction(data->display, data->gc, GC_DRMD(gc));
    
    XDrawLine(data->display, DRAWABLE(data), data->gc,
    	      msg->x1, msg->y1, msg->x2, msg->y2);
	
    if (GC_DOCLIP(gc))
    {
    	XSetClipMask(data->display, data->gc, None);
    }	
    
    XFlush(data->display);
    
    UNLOCK_X11
}

/****************************************************************************************/

VOID MNAME(drawellipse)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    struct bitmap_data  *data = OOP_INST_DATA(cl, o);
    OOP_Object      	*gc = msg->gc;
    
    LOCK_X11
    
    if (GC_DOCLIP(gc))
    {
    	XRectangle cr;
	
    	/* kprintf("X11::Drawllipse: clip %d %d %d %d\n"
	    	    , GC_CLIPX1(gc), GC_CLIPY1(gc), GC_CLIPX2(gc), GC_CLIPY2(gc));
    	*/
		
	cr.x = GC_CLIPX1(gc);
	cr.y = GC_CLIPY1(gc);
	cr.width  = GC_CLIPX2(gc) - cr.x + 1;
	cr.height = GC_CLIPY2(gc) - cr.y + 1;
    
    	XSetClipRectangles(data->display, data->gc,
	    	    	   0, 0, &cr, 1, Unsorted);
    }
    
    XSetForeground(data->display, data->gc, GC_FG(gc));
    XSetFunction(data->display, data->gc, GC_DRMD(gc));
    
    /* kprintf("X11::Drawllipse: coord %d %d %d %d\n"
	    	, msg->x, msg->y, msg->rx, msg->ry);
   
    */	
    
    XDrawArc(data->display, DRAWABLE(data), data->gc,
    	     msg->x - msg->rx, msg->y - msg->ry,
	     msg->rx * 2, msg->ry * 2, 0, 360 * 64);
	
    if (GC_DOCLIP(gc))
    {
    	XSetClipMask(data->display, data->gc, None);
    }	
    
    XFlush(data->display);
    
    UNLOCK_X11
}

/****************************************************************************************/
