#ifndef GRAPHICS_HIDD_INTERN_H
#define GRAPHICS_HIDD_INTERN_H

/* Include files */

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


#define USE_FAST_PUTPIXEL		1
#define OPTIMIZE_DRAWPIXEL_FOR_COPY	1
#define USE_FAST_DRAWPIXEL		1
#define USE_FAST_GETPIXEL		1
#define COPYBOX_CHECK_FOR_ALIKE_PIXFMT	1

    
#define GOT_PF_ATTR(code)	GOT_ATTR(code, aoHidd_PixFmt, pixfmt)
#define FOUND_PF_ATTR(code)	FOUND_ATTR(code, aoHidd_PixFmt, pixfmt);

#define GOT_SYNC_ATTR(code)	GOT_ATTR(code, aoHidd_Sync, sync)
#define FOUND_SYNC_ATTR(code)	FOUND_ATTR(code, aoHidd_Sync, sync);

#define GOT_BM_ATTR(code)	GOT_ATTR(code, aoHidd_BitMap, bitmap)
#define FOUND_BM_ATTR(code)	FOUND_ATTR(code, aoHidd_BitMap, bitmap);


struct sync_data {
    ULONG pixtime; /* pixel time in pico seconds */
    
    ULONG hdisp;
    ULONG left_margin;
    ULONG right_margin;
    ULONG hsync_length;
    
    ULONG vdisp;
    ULONG upper_margin;
    ULONG lower_margin;
    ULONG vsync_length;
    
};


/* This is the pixfmts DB. */

#warning Find a way to optimize searching in the pixfmt database

/* Organize the pf db in some other way that makes it quicker to find a certain PF */

struct pfnode {
    struct MinNode node;
    Object *pixfmt;
    ULONG   refcount;
};

/* This is used as an alias for both pfnode and ModeNode */
struct objectnode {
   struct MinNode node;
   Object *object;
   ULONG refcount;
};


struct mode_bm {
    UBYTE *bm;
    UWORD bpr;
};
struct mode_db {
    /* Array of all available gfxmode PixFmts that are part of 
       gfxmodes
     */
    struct SignalSemaphore sema;
    Object **pixfmts;
    /* Number of pixfmts in the above array */
    ULONG num_pixfmts;
    
    /* All the sync times that are part of any gfxmode */
    Object **syncs;
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
	
	/* Software cursor stuff */
	Object *curs_bm;
	BOOL curs_on;
	ULONG curs_x;
	ULONG curs_y;
	Object *curs_backup;
	
	Object *framebuffer;
	
	Object *shownbm;
	
	/* gc used for stuff like rendering cursor */
	Object *gc;
	
	/* The mode currently used */
	HIDDT_ModeID curmode;
};

/* Private gfxhidd methods */
enum {
    moHidd_Gfx_RegisterPixFmt = num_Hidd_Gfx_Methods,
    moHidd_Gfx_ReleasePixFmt
};

struct pHidd_Gfx_RegisterPixFmt {
    MethodID mID;
    struct TagItem *pixFmtTags;
    
};

struct pHidd_Gfx_ReleasePixFmt {
    MethodID mID;
    Object *pixFmt;
};


Object *HIDD_Gfx_RegisterPixFmt(Object *o, struct TagItem *pixFmtTags);
VOID HIDD_Gfx_ReleasePixFmt(Object *o, Object *pixFmt);

/* Private bitmap methods */
enum {
    moHidd_BitMap_SetBitMapTags = num_Hidd_BitMap_Methods
};

struct pHidd_BitMap_SetBitMapTags {
    MethodID mID;
    struct TagItem *bitMapTags;
};

BOOL HIDD_BitMap_SetBitMapTags(Object *o, struct TagItem *bitMapTags);


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
    Object *bitMap;
#endif
    /* WARNING: structure could be extented in the future                */
    
    Object *friend;	/* Friend bitmap */
    
    Object *gfxhidd;
    
    Object *colmap;
    
    HIDDT_ModeID modeid;
    

    /* Optimize these two method calls */
#if USE_FAST_PUTPIXEL    
    IPTR (*putpixel)(Class *cl, Object *o, struct pHidd_BitMap_PutPixel *msg);
#endif
#if USE_FAST_GETPIXEL    
    IPTR (*getpixel)(Class *cl, Object *o, struct pHidd_BitMap_GetPixel *msg);
#endif

#if USE_FAST_DRAWPIXEL    
    IPTR (*drawpixel)(Class *cl, Object *o, struct pHidd_BitMap_DrawPixel *msg);
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
    struct ExecBase      * sysbase;
    struct Library       * utilitybase;
    struct Library       * oopbase;

    Class                *gfxhiddclass; /* graphics hidd class    */
    Class                *bitmapclass;  /* bitmap class           */
    Class                *gcclass;      /* graphics context class */
    Class		 *colormapclass; /* colormap class	  */
    
    Class		 *pixfmtclass;	/* descring bitmap pixel formats */
    Class		 *syncclass;	/* describing gfxmode sync times */
    
    
    Class		 *planarbmclass;
    Class		 *chunkybmclass;
    
    Object		*std_pixfmts[num_Hidd_StdPixFmt];
    
    /* Thes calls are optimized by calling the method functions directly	*/
#if USE_FAST_PUTPIXEL
    MethodID		putpixel_mid;
#endif
#if USE_FAST_GETPIXEL
    MethodID		getpixel_mid;
#endif
#if USE_FAST_DRAWPIXEL
    MethodID		drawpixel_mid;
#endif
    
};


/* Library base */

struct IntHIDDGraphicsBase
{
    struct Library            hdg_LibNode;
    BPTR                      hdg_SegList;
    struct ExecBase          *hdg_SysBase;
    struct Library           *hdg_UtilityBase;

    struct class_static_data *hdg_csd;
};


#define CSD(x) ((struct class_static_data *)x->UserData)

#undef SysBase
#define SysBase (CSD(cl)->sysbase)

#undef UtilityBase
#define UtilityBase (CSD(cl)->utilitybase)

#undef OOPBase
#define OOPBase (CSD(cl)->oopbase)


/* pre declarations */

Class *init_gfxhiddclass(struct class_static_data *csd);
void   free_gfxhiddclass(struct class_static_data *csd);

Class *init_bitmapclass(struct class_static_data *csd);
void   free_bitmapclass(struct class_static_data *csd);

Class *init_gcclass(struct class_static_data *csd);
void   free_gcclass(struct class_static_data *csd);

Class *init_gfxmodeclass(struct class_static_data *csd);
void   free_gfxmodeclass(struct class_static_data *csd);

Class *init_pixfmtclass(struct class_static_data *csd);
void   free_pixfmtclass(struct class_static_data *csd);

Class *init_syncclass(struct class_static_data *csd);
void   free_syncclass(struct class_static_data *csd);

Class *init_colormapclass(struct class_static_data *csd);
void free_colormapclass(struct class_static_data *csd);


VOID  bitmap_putpixel(Class *cl, Object *obj, struct pHidd_BitMap_PutPixel *msg);
ULONG bitmap_getpixel(Class *cl, Object *obj, struct pHidd_BitMap_GetPixel *msg);
VOID bitmap_convertpixels(Class *cl, Object *o, struct pHidd_BitMap_ConvertPixels *msg);


Class *init_planarbmclass(struct class_static_data *csd);
void   free_planarbmclass(struct class_static_data *csd);

Class *init_chunkybmclass(struct class_static_data *csd);
void   free_chunkybmclass(struct class_static_data *csd);

inline HIDDT_Pixel int_map_truecolor(HIDDT_Color *color, HIDDT_PixelFormat *pf);
BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *csd);
BOOL parse_sync_tags(struct TagItem *tags, struct sync_data *data, ULONG attrcheck, struct class_static_data *csd);



#endif /* GRAPHICS_HIDD_INTERN_H */
