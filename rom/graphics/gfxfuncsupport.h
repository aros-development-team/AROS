#ifndef GFXFUNCSUPPORT_H
#define GFXFUNCSUPPORT_H

/****************************************************************************************/

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <graphics/clip.h>
#include <graphics/gfxbase.h>
#include <hidd/graphics.h>

#define PEN_BITS    8
#define NUM_COLORS  (1L << PEN_BITS)
#define PEN_MASK    (NUM_COLORS - 1)

#define FIX_GFXCOORD(x) x = (WORD)x

/* Our own usage of some private fields in ViewPortExtra */
#define VPE_DATA(vpe)   ((struct HIDD_ViewPortData *)(vpe)->DriverData[0])
#define VPE_DRIVER(vpe) ((struct monitor_driverdata *)(vpe)->DriverData[1])

/* !!!! ONLY USE THE BELOW MACROS IF YOU ARE 100% SURE 
   THAT IT IS A HIDD BITMAP AND NOT ONE THE USER
   HAS CREATED BY HAND !!!. You can use IS_HIDD_BM(bitmap) to test
   if it is a HIDD bitmap
*/

#define OBTAIN_HIDD_BM(bitmap)	\
	( ( IS_HIDD_BM(bitmap))	\
		? HIDD_BM_OBJ(bitmap)	\
		: get_planarbm_object((bitmap), GfxBase) )

#define RELEASE_HIDD_BM(bm_obj, bitmap)                                        \
do                                                                             \
{                                                                              \
    if(! IS_HIDD_BM(bitmap))                                                   \
        release_cache_object(CDD(GfxBase)->planarbm_cache, (bm_obj), GfxBase); \
} while (0)


#define GET_BM_DEPTH(bitmap) \
	(IS_HIDD_BM(bitmap) ? HIDD_BM_REALDEPTH(bitmap) : bitmap->Depth)

#define GET_BM_DRIVERDATA(bitmap) \
	((IS_HIDD_BM(bitmap)) \
		? HIDD_BM_DRVDATA(bitmap) \
		: (struct monitor_driverdata *)CDD(GfxBase))

#define GET_BM_MODEID(bitmap) \
	(HIDD_BM_DRVDATA(bitmap)->id | HIDD_BM_HIDDMODE(bitmap))

/* An idea for future Amiga(tm) chipset driver: it should be implemented in
   architecture-specific part of graphics.hidd. In this case many things will
   start working automatically */
#define GET_VP_DRIVERDATA(vp) \
	((vp->ColorMap && vp->ColorMap->NormalDisplayInfo) \
		? DIH(vp->ColorMap->NormalDisplayInfo)->drv \
		: GET_BM_DRIVERDATA(vp->RasInfo->BitMap))

#if 0
#define BM_PIXEL(bitmap, pen) ((!IS_HIDD_BM(bitmap) || !HIDD_BM_PIXTAB(bitmap)) ? (pen) :  \
    	    	    	       HIDD_BM_PIXTAB(bitmap)[pen])
#else
#define BM_PIXEL(bitmap, pen) ((!IS_HIDD_BM(bitmap) || !HIDD_BM_COLMAP(bitmap)) ? (pen) :  \
    	    	    	       HIDD_CM_GetPixel(HIDD_BM_COLMAP(bitmap), pen))
#endif

/* HIDD BM Flags */

#define HIDD_BMF_SHARED_PIXTAB    	1
#define HIDD_BMF_SCREEN_BITMAP	      512

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

/****************************************************************************************/

/* Private Rastport flags */
#define RPF_NO_PENS	    	(1L << 14)	/* Are pens disabled?				*/
#define RPF_REMAP_COLORFONTS 	(1L << 13)	/* Shall color fonts be automatically remapped? */

#define AROS_PALETTE_SIZE   	256
#define AROS_PALETTE_MEMSIZE 	(sizeof (HIDDT_Pixel) * AROS_PALETTE_SIZE)

/*
 * private AROS fields in RastPort struct:
 * longreserved[0] - pointer to GC class (see oop's _OOP_OBJECT() macro)
 * longreserved[1] - Embedded hidd.graphics.gc object starts here
 */

#define RP_GC(rp)	    ((APTR)&((rp)->longreserved[1]))
#define RP_FGCOLOR(rp)	    GC_FG(RP_GC(rp))
#define RP_BGCOLOR(rp)	    GC_BG(RP_GC(rp))

/****************************************************************************************/

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
};

#define RSI(x) ((struct render_special_info *)x)

/* A Pointer to this struct is stored in each RastPort->longreserved[0] */

struct gfx_driverdata
{
    struct RastPort * dd_RastPort;	/* This RastPort		*/
    struct Rectangle  dd_ClipRectangle;
    UBYTE   	      dd_ClipRectangleFlags;    
};

static inline struct gfx_driverdata *ObtainDriverData(struct RastPort *rp)
{
    struct gfx_driverdata *dd = rp->RP_Extra;

    if (dd && dd->dd_RastPort != rp)
    {
    	/* We have detected a cloned rastport. Detach extra data from it. */
    	dd = NULL;
    }

    return dd;
}

typedef ULONG (*RENDERFUNC)(APTR, WORD, WORD, OOP_Object *, OOP_Object *, struct Rectangle *, struct GfxBase *);
typedef LONG (*PIXELFUNC)(APTR, OOP_Object *, OOP_Object *, WORD, WORD, struct GfxBase *);

/****************************************************************************************/

OOP_Object *get_planarbm_object(struct BitMap *bitmap, struct GfxBase *GfxBase);

ULONG do_render_func(struct RastPort *rp, Point *src, struct Rectangle *rr,
	    	     RENDERFUNC render_func, APTR funcdata,
	    	     BOOL do_update, BOOL get_special_info, struct GfxBase *GfxBase);

ULONG do_render_with_gc(struct RastPort *rp, Point *src, struct Rectangle *rr,
			RENDERFUNC render_func, APTR funcdata, OOP_Object *gc,
			BOOL do_update, BOOL get_special_info, struct GfxBase *GfxBase);

ULONG do_pixel_func(struct RastPort *rp, WORD x, WORD y,
    	    	    LONG (*render_func)(APTR, OOP_Object *, OOP_Object *, WORD, WORD, struct GfxBase *),
		    APTR funcdata, BOOL do_update, struct GfxBase *GfxBase);

ULONG fillrect_render(APTR funcdata, WORD srcx, WORD srcy,
    	    	      OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	      struct Rectangle *rect, struct GfxBase *GfxBase);

LONG fillrect_pendrmd(struct RastPort *tp, WORD x1, WORD y1, WORD x2, WORD y2,
    	    	      HIDDT_Pixel pix, HIDDT_DrawMode drmd, BOOL do_update, struct GfxBase *GfxBase);

BOOL int_bltbitmap(struct BitMap *srcBitMap, OOP_Object *srcbm_obj, WORD xSrc, WORD ySrc,
	    	   struct BitMap *dstBitMap, OOP_Object *dstbm_obj, WORD xDest, WORD yDest,
		   WORD xSize, WORD ySize, ULONG minterm, OOP_Object *gfxhidd, OOP_Object *gc,
		   struct GfxBase *GfxBase);


LONG write_pixels_8(struct RastPort *rp, UBYTE *array, ULONG modulo,
    	    	    WORD xstart, WORD ystart, WORD xstop, WORD ystop,
		    HIDDT_PixelLUT *pixlut, BOOL do_update, struct GfxBase *GfxBase);

LONG write_transp_pixels_8(struct RastPort *rp, UBYTE *array, ULONG modulo,
    	    	    	   WORD xstart, WORD ystart, WORD xstop, WORD ystop,
		    	   HIDDT_PixelLUT *pixlut, UBYTE transparent,
			   BOOL do_update, struct GfxBase *GfxBase);

void amiga2hidd_fast(APTR src_info, OOP_Object *hidd_gc, WORD x_src , WORD y_src,
    	    	     struct BitMap *hidd_bm, WORD x_dest, WORD y_dest,
		     ULONG xsize, ULONG ysize, VOID (*fillbuf_hook)(),
		     struct GfxBase * GfxBase);

BOOL MoveRaster (struct RastPort * rp, WORD dx, WORD dy, WORD x1, WORD y1,
    	    	 WORD x2, WORD y2, BOOL UpdateDamageList, struct GfxBase * GfxBase);

BOOL GetRPClipRectangleForRect(struct RastPort *rp, struct Rectangle *rect, struct Rectangle *r);
BOOL GetRPClipRectangleForBitMap(struct RastPort *rp, struct BitMap *bm,
    	    	    	    	 struct Rectangle *r, struct GfxBase *GfxBase);

void update_bitmap(struct BitMap *bitmap, OOP_Object *bm, UWORD x, UWORD y, UWORD width, UWORD height, struct GfxBase *GfxBase);

void BltRastPortBitMap(struct RastPort *srcRastPort, WORD xSrc, WORD ySrc, 
		       struct BitMap *destBitMap, WORD xDest, WORD yDest,
		       WORD xSize, WORD ySize, ULONG minterm,
		       struct GfxBase *GfxBase);

/****************************************************************************************/

static inline BOOL GetRPClipRectangleForLayer(struct RastPort *rp, struct Layer *lay, struct Rectangle *r, struct GfxBase *GfxBase)
{
    return GetRPClipRectangleForRect(rp, &lay->bounds, r);
}

#endif
