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
		, HIDD_BM_PIXTAB(hidd_bm)
	);
	
	pixels_left_to_process -= (tocopy_w * tocopy_h);

    }

    ULOCK_PIXBUF

    return;
    
}

/****************************************************************************************/

UWORD hidd2cyber_pixfmt(HIDDT_StdPixFmt stdpf)
{
     UWORD cpf = (UWORD)-1;

     D(bug("hidd2cyber stdpf = %d [%d]\n", stdpf, vHidd_StdPixFmt_BGR032));

     switch (stdpf)
     {
	case vHidd_StdPixFmt_RGB15:
	    cpf = PIXFMT_RGB15;
	    break;

	case vHidd_StdPixFmt_RGB15_LE:
	    cpf = PIXFMT_RGB15PC;
	    break;

	case vHidd_StdPixFmt_BGR15:
	    cpf = PIXFMT_BGR15;
	    break;

	case vHidd_StdPixFmt_BGR15_LE:
	    cpf = PIXFMT_BGR15PC;
	    break;
	
	case vHidd_StdPixFmt_RGB16:
	    cpf = PIXFMT_RGB16;
	    break;

	case vHidd_StdPixFmt_RGB16_LE:
	    cpf = PIXFMT_RGB16PC;
	    break;

	case vHidd_StdPixFmt_BGR16:
	    cpf = PIXFMT_BGR16;
	    break;

	case vHidd_StdPixFmt_BGR16_LE:
	    cpf = PIXFMT_BGR16PC;
	    break;
	
	case vHidd_StdPixFmt_RGB24:
	    cpf = PIXFMT_RGB24;
	    break;

	case vHidd_StdPixFmt_BGR24:
	    cpf = PIXFMT_BGR24;
	    break;
	
	case vHidd_StdPixFmt_0RGB32:
            cpf = PIXFMT_0RGB32;
            break;

	case vHidd_StdPixFmt_RGB032:
            cpf = PIXFMT_RGB032;
            break;

	case vHidd_StdPixFmt_BGR032:
            cpf = PIXFMT_BGR032;
            break;

	case vHidd_StdPixFmt_0BGR32:
            cpf = PIXFMT_0BGR32;
    	    break;
	    
	case vHidd_StdPixFmt_ARGB32:
	    cpf = PIXFMT_ARGB32;
	    break;
	
	case vHidd_StdPixFmt_RGBA32:
	    cpf = PIXFMT_RGBA32;
	    break;
	
	case vHidd_StdPixFmt_BGRA32:
	    cpf = PIXFMT_BGRA32;
	    break;
	    
	case vHidd_StdPixFmt_ABGR32:
	    cpf = PIXFMT_ABGR32;
	    break;
	
	case vHidd_StdPixFmt_LUT8:
	    cpf = PIXFMT_LUT8;
	    break;

	default:
	    D(bug("UNKNOWN CYBERGRAPHICS PIXFMT IN cyber2hidd_pixfmt\n"));
	    break;
     
    }

    return cpf;     
     
}
