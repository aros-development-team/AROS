/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GRAPHICS_HIDD_INTERN_H
#define GRAPHICS_HIDD_INTERN_H

/* Include files */

#include <aros/debug.h>
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif
#include <dos/dos.h>
#include <graphics/gfxbase.h>
#include <graphics/monitor.h>


#define USE_FAST_PUTPIXEL		1
#define OPTIMIZE_DRAWPIXEL_FOR_COPY	1
#define USE_FAST_DRAWPIXEL		1
#define USE_FAST_GETPIXEL		1
#define COPYBOX_CHECK_FOR_ALIKE_PIXFMT	1

#define HBM(x) ((struct HIDDBitMapData *)x)

#define PUTPIXEL(o, msg)	\
    HBM(o)->putpixel(OOP_OCLASS(o), o, msg)

#define GETPIXEL(o, msg)	\
    HBM(o)->getpixel(OOP_OCLASS(o), o, msg)

#define DRAWPIXEL(o, msg)	\
    HBM(o)->drawpixel(OOP_OCLASS(o), o, msg)
    
#define GOT_PF_ATTR(code)	GOT_ATTR(code, aoHidd_PixFmt, pixfmt)
#define FOUND_PF_ATTR(code)	FOUND_ATTR(code, aoHidd_PixFmt, pixfmt);

#define GOT_SYNC_ATTR(code)	GOT_ATTR(code, aoHidd_Sync, sync)
#define FOUND_SYNC_ATTR(code)	FOUND_ATTR(code, aoHidd_Sync, sync);

#define GOT_BM_ATTR(code)	GOT_ATTR(code, aoHidd_BitMap, bitmap)
#define FOUND_BM_ATTR(code)	FOUND_ATTR(code, aoHidd_BitMap, bitmap);

#define SWAPBYTES_WORD(x) ((((x) >> 8) & 0x00FF) | (((x) & 0x00FF) << 8))

struct colormap_data
{
    HIDDT_ColorLUT clut;
};

struct pixfmt_data
{
     HIDDT_PixelFormat pf; /* Public portion in the beginning    */

     struct MinNode node;  /* Node for linking into the database */
     ULONG refcount;	   /* Reference count			 */
};

/* Use this macro in order to transform node pointer to pixfmt pointer */
#define PIXFMT_OBJ(n) ((HIDDT_PixelFormat *)((char *)(n) - offsetof(struct pixfmt_data, node)))

struct planarbm_data
{
    UBYTE   **planes;
    ULONG   planebuf_size;
    ULONG   bytesperrow;
    ULONG   rows;
    UBYTE   depth;
    BOOL    planes_alloced;
};

struct chunkybm_data
{
    UBYTE *buffer;
    ULONG bytesperrow;
    ULONG bytesperpixel;
};

struct sync_data {
    struct MonitorSpec *mspc;	/* Associated MonitorSpec */

    ULONG pixelclock;		/* pixel time in Hz */

    ULONG hdisp;		/* Data missing from MonitorSpec */
    ULONG htotal;
    ULONG vdisp;

    ULONG flags;		/* Flags */

    UBYTE description[32];
    
    ULONG hmin;			/* Minimum and maximum allowed bitmap size */
    ULONG hmax;
    ULONG vmin;
    ULONG vmax;

    OOP_Object *gfxhidd;	/* Graphics driver that owns this sync */
    ULONG InternalFlags;	/* Internal flags, see below */
};

/* Sync internal flags */
#define SYNC_FREE_MONITORSPEC    0x0001 /* Allocated own MonitorSpec 				*/
#define SYNC_FREE_SPECIALMONITOR 0x0002 /* Allocated own SpecialMonitor				*/
#define SYNC_VARIABLE		 0x0004 /* Signal timings can be changed			*/

struct mode_bm {
    UBYTE *bm;
    UWORD bpr;
};
struct mode_db {
    /* Array of all available gfxmode PixFmts that are part of 
       gfxmodes
     */
    struct SignalSemaphore sema;
    OOP_Object **pixfmts;
    /* Number of pixfmts in the above array */
    ULONG num_pixfmts;
    
    /* All the sync times that are part of any gfxmode */
    OOP_Object **syncs;
    /* Number of syncs in the above array */
    ULONG num_syncs;
    
    /* A bitmap of size (num_pixfmts * num_syncs), that tells if the
       mode is displayable or not. If a particular (x, y) coordinate
       of the bitmap is 1, it means that the pixfmt and sync objects
       you get by indexing pixfmts[x] and syncs[y] are a valid mode.
       If not, the mode is considered invalid
    */
    
    struct mode_bm orig_mode_bm;	/* Original as supplied by subclass */
    struct mode_bm checked_mode_bm;	/* After applying monitor refresh rate checks etc. */
    
};

struct HIDDGraphicsData
{
	/* Gfx mode "database" */
	struct mode_db mdb;

	/* Framebuffer control stuff */
	OOP_Object *framebuffer;
	OOP_Object *shownbm;
	
	/* gc used for stuff like rendering cursor */
	OOP_Object *gc;
	
	/* The mode currently used (obsolete ?)
	HIDDT_ModeID curmode; */
};

/* Private gfxhidd methods */
enum {
    moHidd_Gfx_RegisterPixFmt = num_Hidd_Gfx_Methods,
    moHidd_Gfx_ReleasePixFmt
};

struct pHidd_Gfx_RegisterPixFmt {
    OOP_MethodID mID;
    struct TagItem *pixFmtTags;
    
};

struct pHidd_Gfx_ReleasePixFmt {
    OOP_MethodID mID;
    OOP_Object *pixFmt;
};


OOP_Object *HIDD_Gfx_RegisterPixFmt(OOP_Object *o, struct TagItem *pixFmtTags);
VOID HIDD_Gfx_ReleasePixFmt(OOP_Object *o, OOP_Object *pixFmt);

/* Private bitmap methods */
enum {
    moHidd_BitMap_SetBitMapTags = num_Hidd_BitMap_Methods
};

struct pHidd_BitMap_SetBitMapTags {
    OOP_MethodID mID;
    struct TagItem *bitMapTags;
};

BOOL HIDD_BitMap_SetBitMapTags(OOP_Object *o, struct TagItem *bitMapTags);


struct HIDDBitMapData
{
    struct _hidd_bitmap_protected prot;
    
    ULONG width;         /* width of the bitmap in pixel  */
    ULONG height;        /* height of the bitmap in pixel */
    ULONG reqdepth;	 /* Depth as requested by user */
    BOOL  displayable;   /* bitmap displayable?           */
    BOOL  pf_registered;
    ULONG flags;         /* see hidd/graphic.h 'flags for */
#if 0
    ULONG format;        /* planar or chunky              */
    ULONG bytesPerRow;   /* bytes per row                 */
    ULONG bytesPerPixel; /* bytes per pixel               */
    OOP_Object *bitMap;
#endif
    /* WARNING: structure could be extented in the future                */
    
    OOP_Object *friend;	/* Friend bitmap */
    
    OOP_Object *gfxhidd;
    OOP_Class  *classptr; /* Class pointer specified during creation */
    
    OOP_Object *colmap;
    
    HIDDT_ModeID modeid;
    

    /* Optimize these two method calls */
#if USE_FAST_PUTPIXEL    
    IPTR (*putpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg);
#endif
#if USE_FAST_GETPIXEL    
    IPTR (*getpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg);
#endif

#if USE_FAST_DRAWPIXEL    
    IPTR (*drawpixel)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPixel *msg);
#endif



};

/* Private bitmap attrs */

enum {
    aoHidd_BitMap_Dummy = num_Hidd_BitMap_Attrs,
    
    num_Total_BitMap_Attrs
    
};



#if 0
struct HIDDGCData
{
#if 0
    APTR bitMap;     /* bitmap to which this gc is connected             */
#endif
    APTR  userData;  /* pointer to own data                              */
    ULONG fg;        /* foreground color                                 */
    ULONG bg;        /* background color                                 */
    ULONG drMode;    /* drawmode                                         */
    /* WARNING: type of font could be change */
    APTR  font;      /* current fonts                                    */
    ULONG colMask;   /* ColorMask prevents some color bits from changing */
    UWORD linePat;   /* LinePattern                                      */
    APTR  planeMask; /* Pointer to a shape bitMap                        */
    ULONG colExp;
    
    /* WARNING: structure could be extented in the future                */
};
#endif    


struct class_static_data
{
    struct GfxBase	 *GfxBase;
    struct SignalSemaphore sema;

    OOP_AttrBase    	 hiddPixFmtAttrBase;
    OOP_AttrBase    	 hiddBitMapAttrBase;
    OOP_AttrBase    	 hiddGfxAttrBase;
    OOP_AttrBase    	 hiddSyncAttrBase;
    OOP_AttrBase    	 hiddGCAttrBase;
    OOP_AttrBase    	 hiddColorMapAttrBase;
    OOP_AttrBase    	 hiddPlanarBMAttrBase;
    
    OOP_Class            *gfxhiddclass; /* graphics hidd class    */
    OOP_Class            *bitmapclass;  /* bitmap class           */
    OOP_Class            *gcclass;      /* graphics context class */
    OOP_Class		 *colormapclass; /* colormap class	  */
    
    OOP_Class		 *pixfmtclass;	/* descring bitmap pixel formats */
    OOP_Class		 *syncclass;	/* describing gfxmode sync times */
    
    
    OOP_Class		 *planarbmclass;
    OOP_Class		 *chunkybmclass;

    /*
       Pixel format "database". This is a list
       of all pixelformats currently used bu some bitmap.
       The point of having this as a central db in the gfx hidd is
       that if several bitmaps are of the same pixel format
       they may point to the same PixFmt object instead
       of allocating their own instance. Thus we are saving mem
     */
    struct SignalSemaphore pfsema;
    struct MinList pflist;
    /* Index of standard pixelformats for quick access */
    HIDDT_PixelFormat	 *std_pixfmts[num_Hidd_StdPixFmt];

    /* Thes calls are optimized by calling the method functions directly	*/
#if USE_FAST_PUTPIXEL
    OOP_MethodID	 putpixel_mid;
#endif
#if USE_FAST_GETPIXEL
    OOP_MethodID	 getpixel_mid;
#endif
#if USE_FAST_DRAWPIXEL
    OOP_MethodID	 drawpixel_mid;
#endif
    HIDDT_RGBConversionFunction rgbconvertfuncs[NUM_RGB_STDPIXFMT][NUM_RGB_STDPIXFMT];
    struct SignalSemaphore rgbconvertfuncs_sem;
};

#define __IHidd_PixFmt      (csd->hiddPixFmtAttrBase)
#define __IHidd_BitMap	    (csd->hiddBitMapAttrBase)
#define __IHidd_Gfx 	    (csd->hiddGfxAttrBase)
#define __IHidd_Sync	    (csd->hiddSyncAttrBase)
#define __IHidd_GC  	    (csd->hiddGCAttrBase)
#define __IHidd_ColorMap    (csd->hiddColorMapAttrBase)
#define __IHidd_PlanarBM    (csd->hiddPlanarBMAttrBase)

/* Library base */

struct IntHIDDGraphicsBase
{
    struct Library            hdg_LibNode;

    struct class_static_data  hdg_csd;
};


/* pre declarations */

BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *csd);

static inline ULONG color_distance(UWORD a1, UWORD r1, UWORD g1, UWORD b1, UWORD a2, UWORD r2, UWORD g2, UWORD b2)
{
    LONG da = (a1 >> 8) - (a2 >> 8);
    LONG dr = (r1 >> 8) - (r2 >> 8);
    LONG dg = (g1 >> 8) - (g2 >> 8);
    LONG db = (b1 >> 8) - (b2 >> 8);

    DB2(bug("[color_distance] a1 = 0x%04X a2 = 0x%04X da = %d\n", a1, a2, da));
    DB2(bug("[color_distance] r1 = 0x%04X r2 = 0x%04X dr = %d\n", r1, r2, dr));
    DB2(bug("[color_distance] g1 = 0x%04X g2 = 0x%04X dg = %d\n", g1, g2, dg));
    DB2(bug("[color_distance] b1 = 0x%04X b2 = 0x%04X db = %d\n", b1, b2, db));

    /* '4' here is a result of trial and error. The idea behind this is to increase
       the weight of alpha difference in order to make the function prefer colors with
       the same alpha value. This is important for correct mouse pointer remapping. */
    return da*da*4 + dr*dr + dg*dg + db*db;
}

#define CSD(x) (&((struct IntHIDDGraphicsBase *)x->UserData)->hdg_csd)
#define csd CSD(cl)

#endif /* GRAPHICS_HIDD_INTERN_H */
