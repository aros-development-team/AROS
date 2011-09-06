#include <hidd/graphics.h>
#include <cybergraphx/cybergraphics.h>

#include "cybergraphics_intern.h"

/* These are the only bitmap internals on which we depend. */
#define IS_HIDD_BM(bitmap)  ((bitmap)->Flags & BMF_SPECIALFMT)
#define HIDD_BM_OBJ(bitmap) ((OOP_Object *)((bitmap)->Planes[0]))

extern BYTE hidd2cyber_pixfmt[];

void hidd2buf_fast(struct BitMap *hidd_bm, LONG x_src , LONG y_src, APTR dest_info,
    	    	   LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize, VOID (*putbuf_hook)(),
		   struct IntCGFXBase *CyberGfxBase);
