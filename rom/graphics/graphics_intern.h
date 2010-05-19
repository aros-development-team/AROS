#ifndef GRAPHICS_INTERN_H
#define GRAPHICS_INTERN_H
/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header file for graphics.library
    Lang: english
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_SEMAPHORE_H
#   include <exec/semaphores.h>
#endif
#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef GRAPHICS_TEXT_H
#   include <graphics/text.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef GRAPHICS_REGIONS_H
#   include <graphics/regions.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif
#ifndef GRAPHICS_VIEW_H
#   include <graphics/view.h>
#endif
#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif

#include <exec/memory.h>
#include <proto/exec.h>
#include <graphics/scale.h>
#include "fontsupport.h"
#include "objcache.h"

#define BITMAP_CLIPPING	    	1

#define REGIONS_USE_MEMPOOL 	1

/* Setting BMDEPTH_COMPATIBILITY to 1 will cause bitmap->Depth
   to be never bigger than 8. The same seems to be the case under
   AmigaOS with CyberGraphX/Picasso96. GetBitMapAttr() OTOH will
   actually return the real depth. */
   
#define BMDEPTH_COMPATIBILITY	1

/* New driverdata code (driverdata is gfx driver stuff connected
   to rastports, ie. a GC object) which is prepared for garbage
   collection. Really need this, as relying on all rastports
   to be un-initialized with AROS specific DeinitRastPort() just
   will not work. Best example: 68k binaries, if we should ever
   have some kind of 68k emu */
   
#define NEW_DRIVERDATA_CODE 	1

#if NEW_DRIVERDATA_CODE
#define OBTAIN_DRIVERDATA(rp,libbase)   ObtainDriverData(rp, libbase)
#define RELEASE_DRIVERDATA(rp,libbase)  ReleaseDriverData(rp, libbase)
#define KILL_DRIVERDATA(rp,libbase) 	KillDriverData(rp, libbase)
#else
#define OBTAIN_DRIVERDATA(rp,libbase)   obsolete_CorrectDriverData(rp, libbase)
#define RELEASE_DRIVERDATA(rp,libbase)  
#define KILL_DRIVERDATA(rp,libbase) 	
#endif

#define SIZERECTBUF 128

struct RegionRectangleExt
{
    struct RegionRectangle     RR;
    IPTR                       Counter;
};

struct RegionRectangleExtChunk
{
     struct RegionRectangleExt       Rects[SIZERECTBUF];
     struct RegionRectangleExtChunk *FirstChunk;
};

#define RRE(x)     ((struct RegionRectangleExt *)(x))
#define Counter(x) (RRE(x)->Counter)
#define Chunk(x)   ((x) ? ((struct RegionRectangleExtChunk *)&RRE(x)[-Counter(x)]) : NULL)

/* PaletteExtra internals */

typedef WORD PalExtra_RefCnt_Type;
typedef WORD PalExtra_AllocList_Type;

#define PALEXTRA_REFCNT(pe,n) 	    (((PalExtra_RefCnt_Type *)(pe)->pe_RefCnt)[(n)])
#define PALEXTRA_ALLOCLIST(pe,n)    (((PalExtra_AllocList_Type *)(pe)->pe_AllocList)[(n)])

struct class_static_data
{
    struct GfxBase  *gfxbase;
    
    OOP_AttrBase    hiddFakeFBAttrBase;
    
    OOP_Class 	    *fakegfxclass;
    OOP_Class 	    *fakefbclass;
    ULONG	    fakefb_attr;
};

/* Monitor driver data */
struct monitor_driverdata
{
    OOP_Object      	     *gfxhidd;		/* Graphics driver to use (can be fakegfx object) */
    ObjectCache     	     *gc_cache;		/* GC cache					  */

    /* FakeGfx-related */
    OOP_Object      	     *gfxhidd_orig;	/* Real graphics driver object			  */
    BOOL    	    	     fakegfx_inited;	/* fakegfx HIDD is in use 			  */

    /* Framebuffer stuff */
    struct BitMap   	     *frontbm;		/* Currently shown bitmap			  */
    OOP_Object	    	     *framebuffer;	/* Framebuffer bitmap object			  */
    OOP_Object	    	     *bm_bak;		/* Original shown bitmap object			  */
    OOP_Object	    	     *colmap_bak;	/* Original colormap object of shown bitmap	  */
    HIDDT_ColorModel 	     colmod_bak;	/* Original colormodel of shown bitmap		  */

    /* Sync modes */
    struct MonitorSpec	     **specs;		/* MonitorSpecs array				  */
    ULONG		     numspecs;		/* Number of MonitorSpecs			  */
};

#define SDD(base)   	    (PrivGBase(base)->current_display)

/* Common driver data data to all monitors */
struct common_driverdata
{
    /* The order of these two must match struct monitor_driverdata */
    OOP_Object		    *memorygfx;		/* Memory graphics driver */
    ObjectCache     	    *gc_cache;		/* GC cache		  */

    ObjectCache     	    *planarbm_cache;	/* Planar bitmaps cache	  */

    /* Attribute bases */
    OOP_AttrBase    	     hiddBitMapAttrBase;
    OOP_AttrBase    	     hiddGCAttrBase;
    OOP_AttrBase    	     hiddSyncAttrBase;
    OOP_AttrBase    	     hiddPixFmtAttrBase;
    OOP_AttrBase    	     hiddPlanarBMAttrBase;
    OOP_AttrBase    	     hiddGfxAttrBase;
    OOP_AttrBase    	     hiddFakeGfxHiddAttrBase;
    OOP_MethodID    	     hiddGfxShowImminentReset_MethodID;
};

#define CDD(base)   	    ((struct common_driverdata *)&PrivGBase(base)->shared_driverdata)

#define __IHidd_BitMap      CDD(GfxBase)->hiddBitMapAttrBase
#define __IHidd_GC          CDD(GfxBase)->hiddGCAttrBase
#define __IHidd_Sync        CDD(GfxBase)->hiddSyncAttrBase
#define __IHidd_PixFmt      CDD(GfxBase)->hiddPixFmtAttrBase
#define __IHidd_PlanarBM    CDD(GfxBase)->hiddPlanarBMAttrBase
#define __IHidd_Gfx         CDD(GfxBase)->hiddGfxAttrBase
#define __IHidd_FakeGfxHidd CDD(GfxBase)->hiddFakeGfxHiddAttrBase

/* Hashtable sizes. Must be powers of two */
#define GFXASSOCIATE_HASHSIZE   8
#define TFE_HASHTABSIZE   	16
#define DRIVERDATALIST_HASHSIZE 256

/* Internal GFXBase struct */
struct GfxBase_intern
{
    struct GfxBase 	 	gfxbase;

    struct class_static_data    *fakegfx_staticdata; /* FakeGFX HIDD static data				 */
    struct monitor_driverdata   *current_display;    /* Current display driver, temporary hack			 */
    ULONG			displays;	     /* Number of display drivers installed in the system	 */
    struct common_driverdata	shared_driverdata;   /* Driver data shared between all monitors (allocated once) */
    struct SignalSemaphore	monitors_sema;	     /* Monitor list semaphore					 */

    struct SignalSemaphore	hashtab_sema;	     /* hash_table arbitration semaphore			 */

    /* TextFontExtension pool */
    struct tfe_hashnode   	* tfe_hashtab[TFE_HASHTABSIZE];
    struct SignalSemaphore  	tfe_hashtab_sema;
    struct SignalSemaphore  	fontsem;

#if REGIONS_USE_MEMPOOL
    /* Regions pool */
    struct SignalSemaphore  	regionsem;
    APTR    	    	    	regionpool;
    struct MinList              ChunkPoolList;
#endif

    /* GC driverdata pool */
    struct SignalSemaphore  	driverdatasem;
    APTR    	    	    	driverdatapool;
    struct MinList  	    	driverdatalist[DRIVERDATALIST_HASHSIZE];

    /* Pixelbuffer, needed for some operations */
    ULONG                      *pixel_buf;
    struct SignalSemaphore      pixbuf_sema;
    struct SignalSemaphore      blit_sema;
};


/* Macros */

#define PrivGBase(x)   	    	((struct GfxBase_intern *)x)

#define WIDTH_TO_BYTES(width) 	((( (width) + 15) & ~15) >> 3)
#define WIDTH_TO_WORDS(width) 	((( (width) + 15) & ~15) >> 4)

#define XCOORD_TO_BYTEIDX( x ) 	(( x ) >> 3)
#define XCOORD_TO_WORDIDX( x ) 	(( x ) >> 4)

#define COORD_TO_BYTEIDX(x, y, bytes_per_row)	\
				( ( ((LONG)(y)) * (bytes_per_row)) + XCOORD_TO_BYTEIDX(x))

#define CHUNKY8_COORD_TO_BYTEIDX(x, y, bytes_per_row)	\
				( ( ((LONG)(y)) * (bytes_per_row)) + (x) )

#define XCOORD_TO_MASK(x)   	(128L >> ((x) & 0x07))

/* For vsprite sorting */

#define JOIN_XY_COORDS(x,y)	(LONG)( ( ((UWORD)(y)) << 16) + ( ( ((UWORD)(x)) + 0x8000 ) & 0xFFFF ) )

#define TFE(tfe) 		(*(struct TextFontExtension**)&tfe)

#define TFE_MATCHWORD	    	0xDFE7 /* randomly invented */

/* Defines for flags in areainfo->FlagPtr */

#define AREAINFOFLAG_MOVE   	0x00
#define AREAINFOFLAG_DRAW   	0x01
#define AREAINFOFLAG_CLOSEDRAW	0x02
#define AREAINFOFLAG_ELLIPSE	0x03

/* Forward declaration */
struct ViewPort;

/* Hash index calculation */
extern ULONG CalcHashIndex(IPTR n, UWORD size);

BOOL obsolete_CorrectDriverData (struct RastPort * rp, struct GfxBase * GfxBase);
#if NEW_DRIVERDATA_CODE
BOOL ObtainDriverData (struct RastPort * rp, struct GfxBase * GfxBase);
void ReleaseDriverData (struct RastPort * rp, struct GfxBase * GfxBase);
void KillDriverData (struct RastPort * rp, struct GfxBase * GfxBase);
#endif

/* a function needed by ClipBlit */
void internal_ClipBlit(struct RastPort * srcRP,
                       LONG xSrc,
                       LONG ySrc,
                       struct RastPort * destRP,
                       LONG xDest,
                       LONG yDest,
                       LONG xSize,
                       LONG ySize,
                       UBYTE minterm,
                       struct GfxBase * GfxBase);

/* Driver prototypes */
extern void driver_Text (struct RastPort *, CONST_STRPTR, LONG, struct GfxBase *);

extern void driver_LoadView(struct View *view, struct GfxBase *);

extern struct monitor_driverdata *driver_Setup(OOP_Object *gfxhidd, struct GfxBase *GfxBase);

extern void driver_Expunge(struct monitor_driverdata *mdd, struct GfxBase *GfxBase);

/* functions in support.c */
extern BOOL pattern_pen(struct RastPort *rp
	, LONG x, LONG y
	, ULONG apen, ULONG bpen
	, ULONG *pixval_ptr
	, struct GfxBase *GfxBase);

/* function for area opeartions */
BOOL areafillpolygon(struct RastPort  * rp,
                     struct Rectangle * bounds,
                     UWORD              first_idx,
                     UWORD              last_idx,
                     ULONG              bytesperrow,
                     struct GfxBase   * GfxBase);

void areafillellipse(struct RastPort   * rp,
		     struct Rectangle  * bounds,
                     UWORD             * CurVctr,
                     ULONG               BytesPerRow,
                     struct GfxBase    * GfxBase);

void areaclosepolygon(struct AreaInfo *areainfo);

/* functions in color_support */
ULONG color_distance(struct ColorMap * cm,
                     ULONG r,
                     ULONG g,
                     ULONG b,
                     ULONG index);

BOOL color_equal(struct ColorMap * cm,
                 ULONG r,
                 ULONG g,
                 ULONG b,
                 ULONG index);

VOID color_get(struct ColorMap *cm,
		ULONG *r,
		ULONG *g,
		ULONG *b,
		ULONG index);

void _DisposeRegionRectangleList
(
    struct RegionRectangle *RR,
    struct GfxBase         *GfxBase
);

struct RegionRectangle *_NewRegionRectangle
(
    struct RegionRectangle **LastRectPtr,
    struct GfxBase *GfxBase
);

BOOL _LinkRegionRectangleList
(
    struct RegionRectangle  *src,
    struct RegionRectangle **dstptr,
    struct GfxBase          *GfxBase
);

#if REGIONS_USE_POOL
#   define GFX_ALLOC(Size)                                \
    ({                                                    \
        APTR Mem;                                         \
                                                          \
        ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);  \
                                                          \
        Mem = AllocPooled                                 \
        (                                                 \
            PrivGBase(GfxBase)->regionpool,               \
            Size                                          \
        );                                                \
                                                          \
        ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem); \
                                                          \
        Mem;                                              \
    })

#    define GFX_FREE(Mem, Size)                           \
     {                                                    \
        APTR Mem;                                         \
                                                          \
        ObtainSemaphore(&PrivGBase(GfxBase)->regionsem);  \
                                                          \
        FreePooled                                        \
        (                                                 \
            PrivGBase(GfxBase)->regionpool,               \
            Mem,                                          \
            size                                          \
        );                                                \
                                                          \
        ReleaseSemaphore(&PrivGBase(GfxBase)->regionsem); \
     }
#else
#    define GFX_ALLOC(Size)     AllocMem(Size, MEMF_ANY)
#    define GFX_FREE(Mem, Size) FreeMem(Mem, Size)
#endif

#define _NewRegionRectangleExtChunk() \
    __NewRegionRectangleExtChunk(GfxBase)

#define _DisposeRegionRectangleExtChunk(_chunk) \
    __DisposeRegionRectangleExtChunk(_chunk, GfxBase)

#define SIZECHUNKBUF 20

struct ChunkExt
{
    struct RegionRectangleExtChunk  Chunk;
    struct ChunkPool               *Owner;
};

struct ChunkPool
{
    struct MinNode  Node;
    struct ChunkExt Chunks[SIZECHUNKBUF];
    struct MinList  ChunkList;
    LONG            NumChunkFree;
};

void __DisposeRegionRectangleExtChunk
(
    struct RegionRectangleExtChunk *Chunk,
    struct GfxBase *GfxBase
);

#endif /* GRAPHICS_INTERN_H */
