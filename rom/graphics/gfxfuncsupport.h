#ifndef GFXFUNCSUPPORT_H
#define GFXFUNCSUPPORT_H

/****************************************************************************************/

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <hidd/graphics.h>

#define PEN_BITS    8
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)

#define FIX_GFXCOORD(x) x = (WORD)x

/* !!!! ONLY USE THE BELOW MACROS IF YOU ARE 100% SURE 
   THAT IT IS A HIDD BITMAP AND NOT ONE THE USER
   HAS CREATED BY HAND !!!. You can use IS_HIDD_BM(bitmap) to test
   if it is a HIDD bitmap
*/

#define HIDD_BM_OBJ(bitmap)       (*(OOP_Object **)&((bitmap)->Planes[0])) 
#define HIDD_BM_COLMAP(bitmap)	  (*(OOP_Object **)&((bitmap)->Planes[2]))
#define HIDD_BM_COLMOD(bitmap)    (*(HIDDT_ColorModel *)&((bitmap)->Planes[3]))
#define HIDD_BM_PIXTAB(bitmap)	  (*(HIDDT_Pixel **)&((bitmap)->Planes[4]))
#define HIDD_BM_REALDEPTH(bitmap) (*(LONG *)&((bitmap)->Planes[5]))

#define OBTAIN_HIDD_BM(bitmap)	\
	( ( IS_HIDD_BM(bitmap))	\
		? HIDD_BM_OBJ(bitmap)	\
		: get_planarbm_object((bitmap), GfxBase) )

#define RELEASE_HIDD_BM(bm_obj, bitmap)                                        \
do                                                                             \
{                                                                              \
    if(! IS_HIDD_BM(bitmap))                                                   \
        release_cache_object(SDD(GfxBase)->planarbm_cache, (bm_obj), GfxBase); \
} while (0)


#define IS_HIDD_BM(bitmap) (((bitmap)->Flags & BMF_AROS_HIDD) == BMF_AROS_HIDD)

#if 0
#define BM_PIXEL(bitmap, pen) ((!IS_HIDD_BM(bitmap) || !HIDD_BM_PIXTAB(bitmap)) ? (pen) :  \
    	    	    	       HIDD_BM_PIXTAB(bitmap)[pen])
#else
#define BM_PIXEL(bitmap, pen) ((!IS_HIDD_BM(bitmap) || !HIDD_BM_COLMAP(bitmap)) ? (pen) :  \
    	    	    	       HIDD_CM_GetPixel(HIDD_BM_COLMAP(bitmap), pen))
#endif

/* Minterms and GC drawmodes are in opposite order */
#define MINTERM_TO_GCDRMD(minterm) 	\
((  	  ((minterm & 0x80) >> 3)	\
	| ((minterm & 0x40) >> 1)	\
	| ((minterm & 0x20) << 1)	\
	| ((minterm & 0x10) << 3) )  >> 4 )

#define TranslateRect(rect, dx, dy) \
do                                  \
{                                   \
    struct Rectangle *_rect = rect; \
    WORD _dx = dx;                  \
    WORD _dy = dy;                  \
    (_rect)->MinX += (_dx);         \
    (_rect)->MinY += (_dy);         \
    (_rect)->MaxX += (_dx);         \
    (_rect)->MaxY += (_dy);         \
} while(0)

/* Private Rastport flags */

/* Flags that tells whether or not the driver has been inited */
#define RPF_DRIVER_INITED   	(1L << 15)
#define RPF_SELF_CLEANUP    	(1L << 14)
/* Shall color fonts be automatically remapped? */
#define RPF_REMAP_COLORFONTS 	(1L << 13)

#define AROS_PALETTE_SIZE   	256
#define AROS_PALETTE_MEMSIZE 	(sizeof (HIDDT_Pixel) * AROS_PALETTE_SIZE)

/* private AROS fields in RastPort struct:

   driverdata:  longreserved[0],longreserved[1]
   backpointer: wordreserved[0],wordreserved[1],wordreserved[2],wordreserved[3]
   fgcolor:     wordreserved[4],wordreserved[5]
   bgcolor: 	wordreserved[6],reserved[0],reserved[1]
   patoriginx:  reserved[2], reserved[3]
   patoriginy:  reserved[4], reserved[5]
   
   Pointers above use 8 byte/64 bit, because that's what
   would be required on AROS 64 bit.
   
   Still unused fields:
   
   UBYTE reserved[6 .. 7]

*/   

#define RP_FGCOLOR(rp)	    (*(ULONG *)&((rp)->wordreserved[4]))
#define RP_BGCOLOR(rp)	    (*(ULONG *)&((rp)->wordreserved[6]))
#define RP_PATORIGINX(rp)   (*(WORD *)&((rp)->reserved[2]))
#define RP_PATORIGINY(rp)   (*(WORD *)&((rp)->reserved[4]))

#define RP_BACKPOINTER(rp)  (*(struct RastPort **)&((rp)->wordreserved[0]))
#define RP_DRIVERDATA(rp)   (*(struct gfx_driverdata **)&((rp)->longreserved[0]))
#define GetDriverData(rp)   RP_DRIVERDATA(rp)

/* SetDriverData() should only be used when cloning RastPorts           */
/* For other purposes just change the values directly in the struct.	*/
#define SetDriverData(rp,dd) RP_DRIVERDATA(rp) = dd

#define NUMPIX 50000
#define PIXELBUF_SIZE (NUMPIX * 4)

#define NUMLUTPIX (PIXELBUF_SIZE)

#define LOCK_PIXBUF ObtainSemaphore(&(PrivGBase(GfxBase)->pixbuf_sema));
#define ULOCK_PIXBUF ReleaseSemaphore(&(PrivGBase(GfxBase)->pixbuf_sema));

#define LOCK_BLIT ObtainSemaphore(&(PrivGBase(GfxBase)->blit_sema));
#define ULOCK_BLIT ReleaseSemaphore(&(PrivGBase(GfxBase)->blit_sema));

#define FLG_PALETTE		( 1L << vHidd_ColorModel_Palette	)
#define FLG_STATICPALETTE	( 1L << vHidd_ColorModel_StaticPalette	)
#define FLG_TRUECOLOR		( 1L << vHidd_ColorModel_TrueColor	)
#define FLG_HASCOLMAP		( 1L << num_Hidd_CM		)

#define GET_COLMOD_FLAGS(bm) (1L << HIDD_BM_COLMOD(bm))

/****************************************************************************************/

struct render_special_info
{
    struct BitMap *curbm;
    BOOL    	  onscreen;
    LONG    	  layer_rel_srcx;
    LONG    	  layer_rel_srcy;
};

#define RSI(x) ((struct render_special_info *)x)

/* A Pointer to this struct is stored in each RastPort->longreserved[0] */

struct gfx_driverdata
{
#if 0
    UWORD	    * dd_AreaPtrn;	/* Amiga current AreaPtrn	*/
    BYTE	      dd_AreaPtSz;	/* Amiga AreaPtSz		*/
    UWORD	      dd_LinePtrn;	/* Amiga current LinePtrn	*/
#endif
    struct MinNode    dd_Node;
    OOP_Object	    * dd_GC;
    struct RastPort * dd_RastPort;	/* This RastPort		*/
    struct Rectangle  dd_ClipRectangle;
    WORD    	      dd_LockCount;
    UBYTE   	      dd_ClipRectangleFlags;    
};

/****************************************************************************************/

OOP_Object *get_planarbm_object(struct BitMap *bitmap, struct GfxBase *GfxBase);

ULONG do_render_func(struct RastPort *rp, Point *src, struct Rectangle *rr,
	    	     ULONG (*render_func)(APTR, LONG, LONG, OOP_Object *, OOP_Object *, LONG, LONG, LONG, LONG, struct GfxBase *),
		     APTR funcdata, BOOL get_special_info, struct GfxBase *GfxBase);

ULONG do_pixel_func(struct RastPort *rp, LONG x, LONG y,
    	    	    LONG (*render_func)(APTR, OOP_Object *, OOP_Object *, LONG, LONG, struct GfxBase *),
		    APTR funcdata, struct GfxBase *GfxBase);

LONG fillrect_pendrmd(struct RastPort *tp, LONG x1, LONG y1, LONG x2, LONG y2,
    	    	      HIDDT_Pixel pix, ULONG drmd, struct GfxBase *GfxBase);

BOOL int_bltbitmap(struct BitMap *srcBitMap, OOP_Object *srcbm_obj, LONG xSrc, LONG ySrc,
	    	   struct BitMap *dstBitMap, OOP_Object *dstbm_obj, LONG xDest, LONG yDest,
		   LONG xSize, LONG ySize, ULONG minterm, OOP_Object *gc, struct GfxBase *GfxBase);


LONG write_pixels_8(struct RastPort *rp, UBYTE *array, ULONG modulo,
    	    	    LONG xstart, LONG ystart, LONG xstop, LONG ystop,
		    HIDDT_PixelLUT *pixlut, struct GfxBase *GfxBase);

LONG write_transp_pixels_8(struct RastPort *rp, UBYTE *array, ULONG modulo,
    	    	    	   LONG xstart, LONG ystart, LONG xstop, LONG ystop,
		    	   HIDDT_PixelLUT *pixlut, UBYTE transparent,
			   struct GfxBase *GfxBase);

void amiga2hidd_fast(APTR src_info, OOP_Object *hidd_gc, LONG x_src , LONG y_src,
    	    	     struct BitMap *hidd_bm, LONG x_dest, LONG y_dest,
		     ULONG xsize, ULONG ysize, VOID (*fillbuf_hook)(),
		     struct GfxBase * GfxBase);

void hidd2buf_fast(struct BitMap *hidd_bm, LONG x_src , LONG y_src, APTR dest_info,
    	    	   LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize, VOID (*putbuf_hook)(),
		   struct GfxBase * GfxBase);

HIDDT_StdPixFmt cyber2hidd_pixfmt(UWORD cpf, struct GfxBase *GfxBase);

UWORD hidd2cyber_pixfmt(HIDDT_StdPixFmt stdpf, struct GfxBase *GfxBase);

BOOL MoveRaster (struct RastPort * rp, LONG dx, LONG dy, LONG x1, LONG y1,
    	    	 LONG x2, LONG y2, BOOL UpdateDamageList, struct GfxBase * GfxBase);

BOOL GetRPClipRectangleForLayer(struct RastPort *rp, struct Layer *lay,
    	    	    	    	struct Rectangle *r, struct GfxBase *GfxBase);
BOOL GetRPClipRectangleForBitMap(struct RastPort *rp, struct BitMap *bm,
    	    	    	    	 struct Rectangle *r, struct GfxBase *GfxBase);


/****************************************************************************************/

#endif
