#ifndef GRAPHICS_INTERN_H
#define GRAPHICS_INTERN_H
/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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

#define REGIONS_USE_MEMPOOL 	1

/* Setting BMDEPTH_COMPATIBILITY to 1 will cause bitmap->Depth
   to be never bigger than 8. The same seems to be the case under
   AmigaOS with CyberGraphX/Picasso96. GetBitMapAttr() OTOH will
   actually return the real depth. */
   
#define BMDEPTH_COMPATIBILITY	1

extern struct GfxBase * GfxBase;


#define SIZERECTBUF 128

struct RegionRectangleExt
{
    struct RegionRectangle     RR;
    ULONG                      Counter;
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
    struct Library  *oopbase;
    struct Library  *utilitybase;
    struct GfxBase  *gfxbase;
    struct ExecBase *sysbase;
    
    OOP_AttrBase    hiddFakeFBAttrBase;
    
    OOP_Class 	    *fakegfxclass;
    OOP_Class 	    *fakefbclass;
    OOP_Class 	    *fakedbmclass;
    
    OOP_Object      *fakegfxobj;
};

struct shared_driverdata
{
    OOP_Object      	     *gfxhidd;
    OOP_Object      	     *gfxhidd_orig;
    OOP_Object      	     *gfxhidd_fake;
    
    ObjectCache     	     *gc_cache;
    ObjectCache     	     *planarbm_cache;
    
    OOP_Object	    	     *framebuffer;
    
    OOP_Object	    	     *bm_bak;
    OOP_Object	    	     *colmap_bak;
    HIDDT_ColorModel 	     colmod_bak;
    
    OOP_AttrBase    	     hiddBitMapAttrBase;
    OOP_AttrBase    	     hiddGCAttrBase;
    OOP_AttrBase    	     hiddSyncAttrBase;
    OOP_AttrBase    	     hiddPixFmtAttrBase;
    OOP_AttrBase    	     hiddPlanarBMAttrBase;
    OOP_AttrBase    	     hiddGfxAttrBase;
    OOP_AttrBase    	     hiddFakeGfxHiddAttrBase;
    
    /* The frontmost screen's bitmap */
    struct BitMap   	     *frontbm;

    /* Does the gfx hidd support hardware pointers ? */    
    BOOL    	    	     has_hw_cursor;

    /* This is used if the gfx hidd does not support hardware mouse pointers */
    OOP_Object      	     *pointerbm;
    LONG    	    	     pointer_x;
    LONG    	    	     pointer_y;
    
    struct class_static_data fakegfx_staticdata;
    BOOL    	    	     fakegfx_inited;
    
#if 0    
    /* Has the code to handle active screens been activated ? */
    BOOL    	    	     activescreen_inited;
#endif    
    APTR    	    	     dispinfo_db; /* Display info database */
};

#define SDD(base)   	    ((struct shared_driverdata *)&PrivGBase(base)->shared_driverdata)
#define __IHidd_BitMap      SDD(GfxBase)->hiddBitMapAttrBase
#define __IHidd_GC          SDD(GfxBase)->hiddGCAttrBase
#define __IHidd_Sync        SDD(GfxBase)->hiddSyncAttrBase
#define __IHidd_PixFmt      SDD(GfxBase)->hiddPixFmtAttrBase
#define __IHidd_PlanarBM    SDD(GfxBase)->hiddPlanarBMAttrBase
#define __IHidd_Gfx         SDD(GfxBase)->hiddGfxAttrBase
#define __IHidd_FakeGfxHidd SDD(GfxBase)->hiddFakeGfxHiddAttrBase


/* Internal GFXBase struct */
struct GfxBase_intern
{
    struct GfxBase 	 	gfxbase;

    struct Library  	    	*oopbase;
    /* Driver data shared between all rastports (allocated once) */
    struct shared_driverdata	shared_driverdata;


#define TFE_HASHTABSIZE   	16 /* This MUST be a power of two */

    struct tfe_hashnode   	* tfe_hashtab[TFE_HASHTABSIZE];
    struct SignalSemaphore  	tfe_hashtab_sema;
    struct SignalSemaphore  	fontsem;
#if REGIONS_USE_MEMPOOL
    struct SignalSemaphore  	regionsem;
    APTR    	    	    	regionpool;
    struct MinList              ChunkPoolList;
#endif
    ULONG                      *pixel_buf;   // used in graphics_driver
    struct SignalSemaphore      pixbuf_sema;
    struct SignalSemaphore      blit_sema;
};


/* Macros */

#define PrivGBase(x)   	    	((struct GfxBase_intern *)x)

#define OOPBase     	    	(PrivGBase(GfxBase)->oopbase)

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

/* Defines */
#define BMT_STANDARD		0x0000	/* Standard bitmap */
#define BMT_RGB 		0x1234	/* RTG Bitmap. 24bit RGB chunky */
#define BMT_RGBA		0x1238	/* RTG Bitmap. 32bit RGBA chunky */
#define BMT_DRIVER		0x8000	/* Special RTG bitmap.
				   	   Use this as an offset. */

#define TFE(tfe) 		((struct TextFontExtension*)tfe)

#define TFE_MATCHWORD	    	0xDFE7 /* randomly invented */

/* Defines for flags in areainfo->FlagPtr */

#define AREAINFOFLAG_MOVE   	0x0
#define AREAINFOFLAG_DRAW   	0x03
#define AREAINFOFLAG_CLOSEDRAW	0x02
#define AREAINFOFLAG_ELLIPSE	0x83

/* Forward declaration */
struct ViewPort;

#ifdef SysBase
#undef SysBase
#endif
#define SysBase 		((struct ExecBase *)(GfxBase->ExecBase))
#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase 		((struct Library *)(GfxBase->UtilBase))

/* Needed for close() */
#define expunge()		AROS_LC0(BPTR, expunge, struct GfxBase *, GfxBase, 3, Gfx)

/* a function needed by GfxAssocate(), GfxLookUp(), GfxFree() */
extern ULONG CalcHashIndex(ULONG n);

/* a function needed by Draw() */
BOOL CorrectDriverData (struct RastPort * rp, struct GfxBase * GfxBase);

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
extern BOOL driver_LateGfxInit(APTR, struct GfxBase *GfxBase);

extern struct BitMap * driver_AllocBitMap (ULONG, ULONG, ULONG, ULONG,
			struct BitMap *, ULONG , struct GfxBase *);
extern void driver_BltClear (void * memBlock, ULONG bytecount, ULONG flags,
			struct GfxBase * GfxBase);
extern LONG driver_BltBitMap ( struct BitMap * srcBitMap, LONG xSrc,
			LONG ySrc, struct BitMap * destBitMap, LONG xDest,
			LONG yDest, LONG xSize, LONG ySize, ULONG minterm,
			ULONG mask, PLANEPTR tempA, struct GfxBase *);
extern VOID driver_BitMapScale(struct BitScaleArgs * bsa,
                               struct GfxBase * GfxBase);
extern VOID driver_BltBitMapRastPort(struct BitMap *,  LONG, LONG,
			    struct RastPort *, LONG, LONG , LONG, LONG,
			    ULONG, struct GfxBase *);
extern VOID driver_BltMaskBitMapRastPort(struct BitMap *srcBitMap
    		, LONG xSrc, LONG ySrc
		, struct RastPort *destRP
		, LONG xDest, LONG yDest
		, ULONG xSize, ULONG ySize
		, ULONG minterm
		, PLANEPTR bltMask
		, struct GfxBase *GfxBase );

extern VOID driver_BltPattern(struct RastPort *rp, PLANEPTR mask, LONG xMin, LONG yMin,
		LONG xMax, LONG yMax, ULONG byteCnt, struct GfxBase *GfxBase);
		
extern VOID driver_BltTemplate(PLANEPTR source, LONG xSrc, LONG srcMod, struct RastPort * destRP,
	LONG xDest, LONG yDest, LONG xSize, LONG ySize, struct GfxBase *GfxBase);
extern int driver_CloneRastPort (struct RastPort *, struct RastPort *,
			struct GfxBase *);
extern void driver_DeinitRastPort (struct RastPort *, struct GfxBase *);
extern void driver_DrawEllipse (struct RastPort *, LONG x, LONG y,
			LONG rx, LONG ry, struct GfxBase *);
extern void driver_Draw( struct RastPort *rp, LONG x, LONG y, struct GfxBase  *GfxBase);
extern void driver_EraseRect (struct RastPort *, LONG, LONG, LONG, LONG,
			    struct GfxBase *);
extern void driver_FreeBitMap (struct BitMap *, struct GfxBase *);
extern ULONG driver_ReadPixel (struct RastPort *, LONG, LONG,
			    struct GfxBase *);
extern LONG driver_ReadPixelArray8 (struct RastPort * rp, ULONG xstart,
			    ULONG ystart, ULONG xstop, ULONG ystop,
			    UBYTE * array, struct RastPort * temprp,
			    struct GfxBase *);
extern void driver_RectFill (struct RastPort *, LONG, LONG, LONG, LONG,
			    struct GfxBase *);
extern BOOL driver_MoveRaster (struct RastPort *,
			    LONG, LONG, LONG, LONG, LONG, LONG, BOOL, BOOL,
			    struct GfxBase *);
extern void driver_SetABPenDrMd (struct RastPort *, ULONG, ULONG, ULONG,
			    struct GfxBase * GfxBase);
extern void driver_SetAPen (struct RastPort *, ULONG, struct GfxBase *);
extern void driver_SetBPen (struct RastPort *, ULONG, struct GfxBase *);
extern void driver_SetDrMd (struct RastPort *, ULONG, struct GfxBase *);
extern void driver_SetOutlinePen (struct RastPort *, ULONG, struct GfxBase *);
extern void driver_SetRast (struct RastPort *, ULONG, struct GfxBase *);
extern void driver_SetRGB32 (struct ViewPort *, ULONG, ULONG, ULONG, ULONG,
			    struct GfxBase *);
extern void driver_Text (struct RastPort *, STRPTR, LONG, struct GfxBase *);
extern VOID driver_WriteChunkyPixels(struct RastPort *rp,
		ULONG, ULONG, ULONG, ULONG,
		UBYTE *, LONG, struct GfxBase *);
extern LONG driver_WritePixel (struct RastPort *, LONG, LONG,
			    struct GfxBase *);
extern LONG driver_WritePixelArray8 (struct RastPort * rp, ULONG xstart,
			    ULONG ystart, ULONG xstop, ULONG ystop,
			    UBYTE * array, struct RastPort * temprp,
			    struct GfxBase *);

extern BOOL driver_ExtendFont(struct TextFont *font, struct tfe_hashnode *hn, struct GfxBase *GfxBase);
extern void driver_StripFont(struct TextFont *font, struct tfe_hashnode *hn, struct GfxBase *GfxBase);

extern ULONG driver_GetDisplayInfoData(DisplayInfoHandle handle, UBYTE *buf, ULONG size, ULONG tagid, ULONG id2, struct GfxBase *GfxBase);
extern DisplayInfoHandle driver_FindDisplayInfo(ULONG id, struct GfxBase *GfxBase);
extern ULONG driver_NextDisplayInfo(ULONG lastid, struct GfxBase *GfxBase);
extern ULONG driver_GetVPModeID(struct ViewPort *vp, struct GfxBase *GfxBase);
extern ULONG driver_BestModeIDA(struct TagItem *tags, struct GfxBase *GfxBase);
extern struct BitMap *driver_AllocScreenBitMap(ULONG modeid, struct GfxBase *GfxBase);
extern BOOL driver_MouseCoordsRelative(struct GfxBase *GfxBase);
VOID driver_SetPointerPos(UWORD x, UWORD y, struct GfxBase *GfxBase);
VOID driver_SetPointerShape(UWORD *shape, UWORD width, UWORD height
		, UWORD xoffset, UWORD yoffset, struct GfxBase *GfxBase);

extern BOOL driver_SetFrontBitMap(struct BitMap *bm, BOOL copyback, struct GfxBase *GfxBase);

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
                     UWORD              bytesperrow,
                     struct GfxBase   * GfxBase);

void areafillellipse(struct RastPort   * rp,
		     struct Rectangle  * bounds,
                     UWORD              * CurVctr,
                     UWORD               BytesPerRow,
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

VOID color_set(struct ColorMap * cm,
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



