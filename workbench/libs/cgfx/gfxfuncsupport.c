#include <aros/debug.h>
#include <clib/macros.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <proto/graphics.h>

#include "gfxfuncsupport.h"

/****************************************************************************************/

void hidd2buf_fast(struct BitMap *hidd_bm, LONG x_src , LONG y_src, APTR dest_info,
    	    	   LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize, VOID (*putbuf_hook)(),
		   struct IntCGFXBase *CyberGfxBase)
{

    ULONG tocopy_w, tocopy_h;
    
    LONG pixels_left_to_process = xsize * ysize;
    ULONG current_x, current_y, next_x, next_y;

    OOP_Object *bm_obj;

    next_x = 0;
    next_y = 0;
    
    bm_obj = HIDD_BM_OBJ(hidd_bm);
	
    LOCK_PIXBUF    

    while (pixels_left_to_process)
    {
	
	current_x = next_x;
	current_y = next_y;
	
	if (NUMPIX < xsize)
	{
	   /* buffer cant hold a single horizontal line, and must 
	      divide each line into copies */
	    tocopy_w = xsize - current_x;
	    if (tocopy_w > NUMPIX)
	    {
	        /* Not quite finished with current horizontal pixel line */
	    	tocopy_w = NUMPIX;
		next_x += NUMPIX;
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
	    tocopy_h = MIN(NUMPIX / xsize, ysize - current_y);
	    tocopy_w = xsize;

	    next_x = 0;
	    next_y += tocopy_h;
	    
    	}
	
	
	/* Get some more pixels from the HIDD */
	HIDD_BM_GetImage(bm_obj
		, (UBYTE *)CyberGfxBase->pixel_buf
		, tocopy_w
		, x_src + current_x
		, y_src + current_y
		, tocopy_w, tocopy_h
		, vHidd_StdPixFmt_Native32);


	/*  Write pixels to the destination */
	putbuf_hook(dest_info
		, current_x + x_src
		, current_y + y_src
		, current_x + x_dest
		, current_y + y_dest
		, tocopy_w, tocopy_h
		, (HIDDT_Pixel *)CyberGfxBase->pixel_buf
		, bm_obj
	);
	
	pixels_left_to_process -= (tocopy_w * tocopy_h);

    }

    ULOCK_PIXBUF

    return;
    
}

/****************************************************************************************/

BYTE hidd2cyber_pixfmt[] = {
    -1,
    -1,
    -1,
    PIXFMT_RGB24,
    PIXFMT_BGR24,
    PIXFMT_RGB16,
    PIXFMT_RGB16PC,
    PIXFMT_BGR16,
    PIXFMT_BGR16PC,
    PIXFMT_RGB15,
    PIXFMT_RGB15PC,
    PIXFMT_BGR15,
    PIXFMT_BGR15PC,
    PIXFMT_ARGB32,
    PIXFMT_BGRA32,
    PIXFMT_RGBA32,
    -1,
    PIXFMT_ARGB32,
    PIXFMT_BGRA32,
    PIXFMT_RGBA32,
    -1,
    PIXFMT_LUT8,
    -1
};
