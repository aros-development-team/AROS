#include <hidd/graphics.h>
#include <cybergraphx/cybergraphics.h>

#include "cybergraphics_intern.h"

#define IS_HIDD_BM(bitmap) (((bitmap)->Flags & BMF_AROS_HIDD) == BMF_AROS_HIDD)
#define HIDD_BM_OBJ(bitmap)       (*(OOP_Object **)&((bitmap)->Planes[0]))
#define HIDD_BM_PIXTAB(bitmap)	  (*(HIDDT_Pixel **)&((bitmap)->Planes[4]))

UWORD hidd2cyber_pixfmt(HIDDT_StdPixFmt stdpf);

void hidd2buf_fast(struct BitMap *hidd_bm, LONG x_src , LONG y_src, APTR dest_info,
    	    	   LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize, VOID (*putbuf_hook)(),
		   struct IntCGFXBase *CyberGfxBase);
