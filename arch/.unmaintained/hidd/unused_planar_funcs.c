
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
