/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

static VOID blttemplate_amiga(PLANEPTR source, LONG x_src, LONG modulo, struct BitMap *dest
	, LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize, struct RastPort *rp, struct GfxBase *GfxBase)
{
    UBYTE *srcptr;
    UBYTE dest_depth = GetBitMapAttr(dest, BMA_DEPTH);
    UWORD drmd = GetDrMd(rp);
    UBYTE apen = GetAPen(rp);
    UBYTE bpen = GetBPen(rp);
    LONG x, y;

    /* Find the exact startbyte. x_src is max 15 */
    srcptr = ((UBYTE *)source) + XCOORD_TO_BYTEIDX(x_src);
    
    /* Find the exact startbit */
    x_src &= 0x07;
/*    
kprintf("DRMD: %d, APEN: %d, BPEN: %d\n", drmd, apen, bpen);
*/
    for (y = 0; y < ysize; y ++)
    {
	UBYTE *byteptr = srcptr;
    	for (x = 0; x < xsize; x ++)
	{
	    UBYTE pen;
	    UBYTE mask = XCOORD_TO_MASK( x + x_src );
	    
	    BOOL is_set = ((*byteptr & mask) ? TRUE : FALSE);
	    BOOL set_pixel = FALSE;

/*if (is_set)
kprintf("X");		    
else	    
kprintf("0");
*/
	    if (drmd & INVERSVID)
	    {
	    	is_set = ((is_set == TRUE) ? FALSE : TRUE);
	    }
	    
	    if (drmd & JAM2)
	    {
	    	/* Use apen if pixel is et, bpen otherwise */
		if (is_set)
		    pen = apen;
		else
		    pen = bpen;
		    
		set_pixel = TRUE;
		
	    }
	    else if (drmd & COMPLEMENT)
	    {
		
	    	pen = getbitmappixel(dest
			, x + x_dest
			, y + y_dest
			, dest_depth
			, 0xFF
		);
		
		pen = ~pen;

		
	    }
	    else /* JAM 1 */
	    {
	    	/* Only use apen if pixel is set */
		if (is_set)
		{
		    pen = apen;
		    set_pixel = TRUE;
		}
		    
	    }
	    if (set_pixel)
	    {
/* kprintf("X");		    
*/		setbitmappixel(dest
			, x + x_dest
			, y + y_dest
			, pen
			, dest_depth, 0xFF
		);
	    }
/* else
kprintf("0");
*/	
	    /* Last pixel in this byte ? */
	    if (((x + x_src) & 0x07) == 0x07)
	    	byteptr ++;
		
	}
/* kprintf("\n");	
*/	srcptr += modulo;
    }
    return;
}	




static VOID bltpattern_amiga(struct pattern_info *pi
		, struct BitMap *dest_bm
		, LONG x_src, LONG y_src	/* offset into layer */
		, LONG x_dest, LONG y_dest	/* offset into bitmap */
		, ULONG xsize, LONG ysize
		, struct GfxBase *GfxBase
)
{

    /* x_src, y_src is the coordinates int the layer. */
    LONG y;
    struct RastPort *rp = pi->rp;
    ULONG apen = GetAPen(rp);
    ULONG bpen = GetBPen(rp);
    
    UBYTE *apt = (UBYTE *)rp->AreaPtrn;
    
    UBYTE dest_depth = GetBitMapAttr(dest_bm, BMA_DEPTH);
    
    for (y = 0; y < ysize; y ++)
    {
        LONG x;
	
	for (x = 0; x < xsize; x ++)
	{
	    ULONG set_pixel;
	    ULONG pixval;
	    
	    
	    /* Mask supplied ? */
	    if (pi->mask)
	    {
		ULONG idx, mask;
		idx = COORD_TO_BYTEIDX(x + pi->mask_xmin, y + pi->mask_ymin, pi->mask_bpr);
		mask = XCOORD_TO_MASK(x + pi->mask_xmin);
		 
		set_pixel = pi->mask[idx] & mask;
		 
	    }
	    else
	        set_pixel = 1UL;
		
		
	    if (set_pixel)
	    {
	   
	
		if (apt)
		{

		   set_pixel = pattern_pen(rp, x + x_src, y + y_src, apen, bpen, &pixval, GfxBase);
		   if (set_pixel)
		   {
		    	setbitmappixel(dest_bm, x + x_dest, y + y_dest, pixval, dest_depth, 0xFF);
		   }
		   
		}
	    
	    } /* if (pixel should be set */
	    
	    
	} /* for (each column) */
	
    } /* for (each row) */
    
    return;

}    
static VOID bltmask_amiga(struct bltmask_info *bmi
	, LONG x_src, LONG y_src
	, struct BitMap *destbm
	, LONG x_dest, LONG y_dest
	, ULONG xsize, ULONG ysize
	, ULONG minterm )
{
    /* x_src, y_src is the coordinates int the layer. */
    LONG y;
    UBYTE src_depth, dest_depth;
    

    EnterFunc(bug("bltmask_amiga(%p, %d, %d, %d, %d, %d, %d, %p)\n"
    			, bmi, x_src, y_src, x_dest, y_dest, xsize, ysize));

    src_depth  = GetBitMapAttr(bmi->srcbm, BMA_DEPTH);
    dest_depth = GetBitMapAttr(destbm,     BMA_DEPTH);
    
    
    for (y = 0; y < ysize; y ++)
    {
        LONG x;
	
	for (x = 0; x < xsize; x ++)
	{
	    ULONG set_pixel;
	    
	    ULONG idx, mask;
	    idx = COORD_TO_BYTEIDX(x + bmi->mask_xmin, y + bmi->mask_ymin, bmi->mask_bpr);
	    mask = XCOORD_TO_MASK(x + bmi->mask_xmin);
		 
	    set_pixel = bmi->mask[idx] & mask;
		
	    if (set_pixel)
	    {
	        ULONG srcpen, destpen, pixval;
		srcpen = getbitmappixel(bmi->srcbm
		  	, x + x_src
			, y + y_src
			, src_depth
			, 0xFF
		);
		
/* Could optimize plain copy (0x00C0) here. (does not nead to get dest)
 and clear (0x0000) (needs neither src or dest)*/
		
		destpen = getbitmappixel(destbm
		  	, x + x_dest
			, y + y_dest
			, dest_depth
			, 0xFF
		);
		
		APPLY_MINTERM(pixval, srcpen, destpen, minterm);
		setbitmappixel(destbm
		  	, x + x_dest
			, y + y_dest
			, pixval
			, dest_depth
			, 0xFF
		);
		
	    }
	    
	    
	} /* for (each column) */
	
    } /* for (each row) */
    
    ReturnVoid("bltmask_amiga");
    
}

static VOID hidd2amiga_fast(struct BitMap *hidd_bm
	, LONG x_src , LONG y_src
	, APTR dest_info
	, LONG x_dest, LONG y_dest
	, ULONG xsize, ULONG ysize
	, VOID (*putbuf_hook)()
)
{

    ULONG tocopy_w, tocopy_h;
    
    LONG pixels_left_to_process = xsize * ysize;
    ULONG current_x, current_y, next_x, next_y;
    
#warning Src bitmap migh be user initialized so we should not use HIDD_BM_PIXTAB() below
    HIDDT_PixelLUT pixlut = { AROS_PALETTE_SIZE, HIDD_BM_PIXTAB(hidd_bm) };
    
    Object *bm_obj;
    
    next_x = 0;
    next_y = 0;
    
    bm_obj = OBTAIN_HIDD_BM(hidd_bm);
    if (NULL == bm_obj)
    	return;
	
LOCK_PIXBUF    

    while (pixels_left_to_process)
    {
	
	current_x = next_x;
	current_y = next_y;
	
	if (NUMLUTPIX < xsize)
	{
	   /* buffer cant hold a single horizontal line, and must 
	      divide each line into copies */
	    tocopy_w = xsize - current_x;
	    if (tocopy_w > NUMLUTPIX)
	    {
	        /* Not quite finished with current horizontal pixel line */
	    	tocopy_w = NUMLUTPIX;
		next_x += NUMLUTPIX;
	    }
	    else
	    {	/* Start at a new line */
	    
	    	next_x = 0;
		next_y ++;
	    }
	    tocopy_h = 1;
	    
    	}
    	else
    	{
	    tocopy_h = MIN(NUMLUTPIX / xsize, ysize - current_y);
	    tocopy_w = xsize;

	    next_x = 0;
	    next_y += tocopy_h;
	    
    	}
	
	
	/* Get some more pixels from the HIDD */
	HIDD_BM_GetImageLUT(bm_obj
		, (UBYTE *)pixel_buf
		, tocopy_w
		, x_src + current_x
		, y_src + current_y
		, tocopy_w, tocopy_h
		, &pixlut);


	/*  Write pixels to the destination */
	putbuf_hook(dest_info
		, current_x + x_src
		, current_y + y_src
		, current_x + x_dest
		, current_y + y_dest
		, tocopy_w, tocopy_h
		, (UBYTE *)pixel_buf
		, bm_obj
		, IS_HIDD_BM(hidd_bm) ? HIDD_BM_PIXTAB(hidd_bm) : NULL
	);
	
	pixels_left_to_process -= (tocopy_w * tocopy_h);

    }
    
ULOCK_PIXBUF

    RELEASE_HIDD_BM(bm_obj, hidd_bm);
    
    return;
    
}


struct blit_info
{
    struct BitMap *bitmap;
    ULONG minterm;
    ULONG planemask;
    UBYTE bmdepth;
    ULONG bmwidth;
    
};

#define BI(x) ((struct blit_info *)x)
static VOID bitmap_to_buf(APTR src_info
	, LONG x_src, LONG y_src
	, LONG x_dest, LONG y_dest
	, LONG width, LONG height
	, ULONG *bufptr
	, Object *dest_bm
	, HIDDT_Pixel *coltab
) /* destination HIDD bitmap */
{

    LONG y;
    
    /* Fill buffer with pixels from bitmap */
    for (y = 0; y < height; y ++)
    {
	LONG x;
	    
	for (x = 0; x < width; x ++)
	{
	    UBYTE pen;
	    
	    pen = getbitmappixel(BI(src_info)->bitmap
		, x + x_src
		, y + y_src
		, BI(src_info)->bmdepth
		, BI(src_info)->planemask);
		
		
		
	    *bufptr ++ = (coltab != NULL) ? coltab[pen] : pen;
//	    kprintf("(%d, %d) pen=%d buf=%d\n", x, y, pen, coltab[pen]);
			

	}
	
    }

}


static VOID buf_to_bitmap(APTR dest_info
	, LONG x_src, LONG y_src
	, LONG x_dest, LONG y_dest
	, ULONG width, ULONG height
	, UBYTE *bufptr
	, Object *src_bm
	, HIDDT_Pixel *coltab
)
{
	
    if (BI(dest_info)->minterm == 0x00C0)
    {
	LONG y;
	for (y = 0; y < height; y ++)
	{
	    LONG x;
	    for (x = 0; x < width; x ++)
	    {
		setbitmappixel(BI(dest_info)->bitmap
		    	, x + x_dest
			, y + y_dest
			, *bufptr ++, BI(dest_info)->bmdepth, BI(dest_info)->planemask
		);


	    }
		
	}

    }
    else
    {
	LONG y;
	    
	for (y = 0; y < height; y ++)
	{
	    LONG x;
		
	    for (x = 0; x < width; x ++)
	    {
		ULONG src = *bufptr ++ , dest = 0;
		ULONG minterm = BI(dest_info)->minterm;

		/* Set the pixel using correct minterm */

		dest = getbitmappixel(BI(dest_info)->bitmap
			, x + x_dest
			, y + y_dest
			, BI(dest_info)->bmdepth
			, BI(dest_info)->planemask
		);

#warning Do reverse coltab lookup	    	
		if (minterm & 0x0010) dest  = ~src & ~dest;
		if (minterm & 0x0020) dest |= ~src & dest;
		if (minterm & 0x0040) dest |=  src & ~dest;
		if (minterm & 0x0080) dest |= src & dest;
		    
		setbitmappixel(BI(dest_info)->bitmap
			, x + x_dest
			, y + y_dest
			, dest, BI(dest_info)->bmdepth
			, BI(dest_info)->planemask
		);

	    }
		
	}
	    
    }
    return;

}


static VOID setbitmapfast(struct BitMap *bm, LONG x_start, LONG y_start, LONG xsize, LONG ysize, ULONG pen);



static VOID clipagainstbitmap(struct BitMap *bm, LONG *x1, LONG *y1, LONG *x2, LONG *y2, struct GfxBase *GfxBase)
{
    ULONG width  = GetBitMapAttr(bm, BMA_WIDTH);
    ULONG height = GetBitMapAttr(bm, BMA_HEIGHT);
    
    /* Clip against bitmap bounds  */
	    
    if (*x1 < 0)  *x1 = 0;
    if (*y1 < 0)  *y1 = 0;

    if (*x2 >= width)  *x2 = width  - 1;
    if (*y2 >= height) *y2 = height - 1; 
    
    return;
}


static VOID setbitmappixel(struct BitMap *bm
	, LONG x, LONG y
	, ULONG pen
	, UBYTE depth
	, UBYTE plane_mask)
{
    UBYTE i;
    ULONG idx;
    UBYTE mask, clr_mask;
    ULONG penmask;

    idx = COORD_TO_BYTEIDX(x, y, bm->BytesPerRow);

    mask = XCOORD_TO_MASK( x );
    clr_mask = ~mask;
    
    penmask = 1;
    for (i = 0; i < depth; i ++)
    {

	if ((1L << i) & plane_mask)
	{
            UBYTE *plane = bm->Planes[i];
	
	    if ((plane != NULL) && (plane != (PLANEPTR)-1))
	    {
		if ((penmask & pen) != 0)
		    plane[idx] |=  mask;
		else
		    plane[idx] &=  clr_mask;
            }

	}
	penmask <<= 1;
	
    }
    return;
}


enum { SB_SINGLEMASK, SB_PREPOSTMASK, SB_FULL };
static VOID setbitmapfast(struct BitMap *bm, LONG x_start, LONG y_start, LONG xsize, LONG ysize, ULONG pen)
{
    LONG num_whole;
    UBYTE sb_type;
    
    UBYTE plane;
    UBYTE pre_pixels_to_set,
    	  post_pixels_to_set,
	  pre_and_post; /* number pixels to clear in pre and post byte */
 
    UBYTE prebyte_mask, postbyte_mask;
    
/*    kprintf("x_start: %d, y_start: %d, xsize: %d, ysize: %d, pen: %d\n",
    	x_start, y_start, xsize, ysize, pen);
*/	

    pre_pixels_to_set  = (7 - (x_start & 0x07)) + 1;
    post_pixels_to_set = ((x_start + xsize - 1) & 0x07) + 1;
    

    pre_and_post = pre_pixels_to_set + post_pixels_to_set;
    
    if (pre_and_post > xsize)
    {
	UBYTE start_bit, stop_bit;
	/* Check whether the pixels are kept within a byte */
	sb_type = SB_SINGLEMASK;
    	pre_pixels_to_set  = MIN(pre_pixels_to_set,  xsize);
	
	/* Mask out the needed bits */
	start_bit =  7 - (x_start & 0x07) + 1;
	stop_bit = 7 - ((x_start + xsize - 1) & 0x07);

/* kprintf("start_bit: %d, stop_bit: %d\n", start_bit, stop_bit);
*/	
	prebyte_mask = ((1L << start_bit) - 1) - ((1L << stop_bit) - 1) ;
/* kprintf("prebyte_mask: %d\n", prebyte_mask);

kprintf("SB_SINGLE\n");
*/
    }
    else if (pre_and_post == xsize)
    {
    	/* We have bytes within to neighbour pixels */
	sb_type = SB_PREPOSTMASK;
	prebyte_mask  = 0xFF >> (8 - pre_pixels_to_set);
	postbyte_mask = 0xFF << (8 - post_pixels_to_set);
    
/* kprintf("SB_PREPOSTMASK\n");
*/
    }
    else
    {

	/* Say we want to clear two pixels in last byte. We want the mask
	MSB 00000011 LSB
	*/
	sb_type = SB_FULL;
	prebyte_mask = 0xFF >> (8 - pre_pixels_to_set);
    
	/* Say we want to set two pixels in last byte. We want the mask
	MSB 11000000 LSB
	*/
	postbyte_mask = 0xFF << (8 - post_pixels_to_set);
	
        	/* We have at least one whole byte of pixels */
	num_whole = xsize - pre_pixels_to_set - post_pixels_to_set;
	num_whole >>= 3; /* number of bytes */
	
/* kprintf("SB_FULL\n");
*/
    }
	
/*
kprintf("pre_pixels_to_set: %d, post_pixels_to_set: %d, numwhole: %d\n"
	, pre_pixels_to_set, post_pixels_to_set, num_whole);
    
kprintf("prebyte_mask: %d, postbyte_mask: %d, numwhole: %d\n", prebyte_mask, postbyte_mask, num_whole);
*/    
    for (plane = 0; plane < GetBitMapAttr(bm, BMA_DEPTH); plane ++)
    {
    
        LONG y;
	UBYTE pixvals;
	UBYTE prepixels_set, prepixels_clr;
	UBYTE postpixels_set, postpixels_clr;
    	UBYTE *curbyte = ((UBYTE *)bm->Planes[plane]) + COORD_TO_BYTEIDX(x_start, y_start, bm->BytesPerRow);
	
	
	/* Set or clear current bit of pixval ? */
	if (pen & (1L << plane))
	    pixvals = 0xFF;
	else
	    pixvals = 0x00;
	
	/* Set the pre and postpixels */
	switch (sb_type)
	{
	    case SB_FULL:
		prepixels_set  = (pixvals & prebyte_mask);
		postpixels_set = (pixvals & postbyte_mask);
	

		prepixels_clr  = (pixvals & prebyte_mask)  | (~prebyte_mask);
		postpixels_clr = (pixvals & postbyte_mask) | (~postbyte_mask);

		for (y = 0; y < ysize; y ++)
		{
		    LONG x;
		    UBYTE *ptr = curbyte;
	    
		    *ptr |= prepixels_set;
		    *ptr ++ &= prepixels_clr;
	    
		    for (x = 0; x < num_whole; x ++)
		    {
			*ptr ++ = pixvals;
		    }
		    /* Clear the last nonwhole byte */
		    *ptr |= postpixels_set;
		    *ptr ++ &= postpixels_clr;
	    
		    /* Go to next line */
		    curbyte += bm->BytesPerRow;
		}
		break;
		
	    case SB_PREPOSTMASK:
	
		prepixels_set  = (pixvals & prebyte_mask);
		postpixels_set = (pixvals & postbyte_mask);
	

		prepixels_clr  = (pixvals & prebyte_mask)  | (~prebyte_mask);
		postpixels_clr = (pixvals & postbyte_mask) | (~postbyte_mask);

		for (y = 0; y < ysize; y ++)
		{
		    UBYTE *ptr = curbyte;
	    
		    *ptr |= prepixels_set;
		    *ptr ++ &= prepixels_clr;
	    
		    /* Clear the last nonwhole byte */
		    *ptr |= postpixels_set;
		    *ptr ++ &= postpixels_clr;
	    
		    /* Go to next line */
		    curbyte += bm->BytesPerRow;
		}
		break;
		
	    case SB_SINGLEMASK:
	
		prepixels_set  = (pixvals & prebyte_mask);
		prepixels_clr  = (pixvals & prebyte_mask) | (~prebyte_mask);

		for (y = 0; y < ysize; y ++)
		{
		    UBYTE *ptr = curbyte;
	    
		    *ptr |= prepixels_set;
		    *ptr ++ &= prepixels_clr;
	    
		    /* Go to next line */
		    curbyte += bm->BytesPerRow;
		}
		break;
		
	} /* switch() */
    }
    return;
    
}



