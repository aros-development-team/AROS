/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

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
    
    UBYTE description[32];
};


/* This is the pixfmts DB. */

#warning Find a way to optimize searching in the pixfmt database

/* Organize the pf db in some other way that makes it quicker to find a certain PF */

struct pfnode {
    struct MinNode node;
    OOP_Object *pixfmt;
    ULONG   refcount;
};

/* This is used as an alias for both pfnode and ModeNode */
struct objectnode {
   struct MinNode node;
   OOP_Object *object;
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

#if 0	
	/* Software cursor stuff */
	OOP_Object *curs_bm;
	BOOL curs_on;
	ULONG curs_x;
	ULONG curs_y;
	OOP_Object *curs_backup;
#endif	
	OOP_Object *framebuffer;
	
	OOP_Object *shownbm;
	
	/* gc used for stuff like rendering cursor */
	OOP_Object *gc;
	
	/* The mode currently used */
	HIDDT_ModeID curmode;
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
    struct ExecBase      * sysbase;
    struct Library       * utilitybase;
    struct Library       * oopbase;
 
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
    
    OOP_Object		 *std_pixfmts[num_Hidd_StdPixFmt];
    
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

OOP_Class *init_gfxhiddclass(struct class_static_data *csd);
void   free_gfxhiddclass(struct class_static_data *csd);

OOP_Class *init_bitmapclass(struct class_static_data *csd);
void   free_bitmapclass(struct class_static_data *csd);

OOP_Class *init_gcclass(struct class_static_data *csd);
void   free_gcclass(struct class_static_data *csd);

OOP_Class *init_gfxmodeclass(struct class_static_data *csd);
void   free_gfxmodeclass(struct class_static_data *csd);

OOP_Class *init_pixfmtclass(struct class_static_data *csd);
void   free_pixfmtclass(struct class_static_data *csd);

OOP_Class *init_syncclass(struct class_static_data *csd);
void   free_syncclass(struct class_static_data *csd);

OOP_Class *init_colormapclass(struct class_static_data *csd);
void free_colormapclass(struct class_static_data *csd);


VOID  bitmap_putpixel(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_PutPixel *msg);
ULONG bitmap_getpixel(OOP_Class *cl, OOP_Object *obj, struct pHidd_BitMap_GetPixel *msg);
VOID bitmap_convertpixels(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_ConvertPixels *msg);
VOID bitmap_fillmemrect8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_FillMemRect8 *msg);
VOID bitmap_fillmemrect16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_FillMemRect16 *msg);
VOID bitmap_fillmemrect24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_FillMemRect24 *msg);
VOID bitmap_fillmemrect32(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_FillMemRect32 *msg);
VOID bitmap_invertmemrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_InvertMemRect *msg);
VOID bitmap_copymembox8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyMemBox8 *msg);
VOID bitmap_copymembox16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyMemBox16 *msg);
VOID bitmap_copymembox24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyMemBox24 *msg);
VOID bitmap_copymembox32(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyMemBox32 *msg);
VOID bitmap_copylutmembox16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyLUTMemBox16 *msg);
VOID bitmap_copylutmembox24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyLUTMemBox24 *msg);
VOID bitmap_copylutmembox32(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_CopyLUTMemBox32 *msg);
VOID bitmap_putmem32image8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMem32Image8 *msg);
VOID bitmap_putmem32image16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMem32Image16 *msg);
VOID bitmap_putmem32image24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutMem32Image24 *msg);
VOID bitmap_getmem32image8(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetMem32Image8 *msg);
VOID bitmap_getmem32image16(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetMem32Image16 *msg);
VOID bitmap_getmem32image24(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetMem32Image24 *msg);

OOP_Class *init_planarbmclass(struct class_static_data *csd);
void   free_planarbmclass(struct class_static_data *csd);

OOP_Class *init_chunkybmclass(struct class_static_data *csd);
void   free_chunkybmclass(struct class_static_data *csd);

inline HIDDT_Pixel int_map_truecolor(HIDDT_Color *color, HIDDT_PixelFormat *pf);
BOOL parse_pixfmt_tags(struct TagItem *tags, HIDDT_PixelFormat *pf, ULONG attrcheck, struct class_static_data *csd);
BOOL parse_sync_tags(struct TagItem *tags, struct sync_data *data, ULONG attrcheck, struct class_static_data *csd);



#endif /* GRAPHICS_HIDD_INTERN_H */
