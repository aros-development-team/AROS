#ifndef HIDD_GRAPHICS_H
#define HIDD_GRAPHICS_H

/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for the Gfx Hidd system.
    Lang: english
*/

#include <graphics/gfx.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/utility.h>

typedef OOP_Object *HIDDT_BitMap;
typedef OOP_Object *HIDDT_GC;

#ifdef __GRAPHICS_NOHIDDBASE__
#define __Hidd_Gfx_NOMETHODBASE__
#define __Hidd_BitMap_NOMETHODBASE__
#define __Hidd_ColorMap_NOMETHODBASE__
#define __Hidd_GC_NOMETHODBASE__
#define __Hidd_PlanarBM_NOMETHODBASE__
#endif

/**** Graphics definitions ****************************************************/

#define CLID_HW_Gfx      "hw.gfx"

/* Sprite types */
#define vHidd_SpriteType_3Plus1      0x01 /* Color 0 transparent, 1-3 visible                */
#define vHidd_SpriteType_2Plus1      0x02 /* Color 0 transparrent, 1 undefined, 2-3 visible */
#define vHidd_SpriteType_DirectColor 0x04 /* Hi- or truecolor image                          */

#define tHidd_Gfx_BASE                  TAG_USER
#define tHidd_Gfx_QuerModeBASE          (tHidd_Gfx_BASE + 0x1000)
#define tHidd_Gfx_CursorBASE            (tHidd_Gfx_BASE + 0x2000)
#define tHidd_Gfx_MemAttribBASE         (tHidd_Gfx_BASE + 0x3000)

/* Parameter tags for the QueryModeIDs method */
enum
{
    tHidd_GfxMode_MinWidth = tHidd_Gfx_QuerModeBASE,
    tHidd_GfxMode_MaxWidth,
    tHidd_GfxMode_MinHeight,
    tHidd_GfxMode_MaxHeight,
    tHidd_GfxMode_PixFmts
};

typedef enum
{
    vHidd_Gfx_DPMSLevel_On,
    vHidd_Gfx_DPMSLevel_Standby,
    vHidd_Gfx_DPMSLevel_Suspend,
    vHidd_Gfx_DPMSLevel_Off
    
} HIDDT_DPMSLevel;

typedef IPTR HIDDT_StdPixFmt;
typedef IPTR HIDDT_ModeID;
typedef IPTR HIDDT_DrawMode;
typedef IPTR HIDDT_ColorModel;
typedef IPTR HIDDT_BitMapType;

struct HIDD_ModeProperties
{
    ULONG DisplayInfoFlags; /* PropertyFlags value for struct DisplayInfo (see graphics/displayinfo.h).
                               Does not include features emulated by software.                           */
    UWORD NumHWSprites;     /* Number of supported hardware sprites                                      */
    UWORD CompositionFlags; /* Supported composition types, see below                                    */

    /* This structure may grow in future */

};

#define COMPB_ABOVE             0
#define COMPF_ABOVE             (1 << COMPB_ABOVE)      /* Compositive above screens       */
#define COMPB_BELOW             1
#define COMPF_BELOW             (1 << COMPB_BELOW)      /* ... below ...                   */
#define COMPB_LEFT              2
#define COMPF_LEFT              (1 << COMPB_LEFT)        /* ... to the left of ...         */
#define COMPB_RIGHT             3
#define COMPF_RIGHT             (1 << COMPB_RIGHT)      /* ... and to the right of ...     */
#define COMPB_ALPHA             7
#define COMPF_ALPHA             (1 << COMPB_ALPHA)
#define COMPB_SAME              8
#define COMPF_SAME              (1 << COMPB_SAME)       /* can only composit the same sync */

struct ViewPort;
struct View;

/* A structure passed to ShowViewPorts() method */
struct HIDD_ViewPortData
{
    struct HIDD_ViewPortData *Next;     /* Pointer to a next bitmap             */
    OOP_Object               *Bitmap;   /* The bitmap object itself             */
    struct ViewPortExtra     *vpe;      /* Associated ViewPortExtra             */
    APTR                      UserData; /* The driver can keep own stuff here   */
};

#define vHidd_ModeID_Invalid ((HIDDT_ModeID)-1)


/* Flags */

/* This will make the gfx hidd, copy back from the framebuffer into
   the old bitmap you were using. Use this option with extreme
   prejudice:  YOU MUST BE TOTALLY SURE THAT THE BITMAP
   YOU CALLED SHOW ON BEFORE THIS CALL HAS NOT BEEN DISPOSED
*/
#define fHidd_Gfx_Show_CopyBack 0x01

enum
{
    tHidd_Cursor_BitMap = tHidd_Gfx_CursorBASE,         /* OOP_Object *, cursor shape bitmap */
    tHidd_Cursor_XPos,                                  /* ULONG, cursor x position     */
    tHidd_Cursor_YPos,                                  /* ULONG, cursor Y position */
    tHidd_Cursor_On                                     /* BOOL, cursor on, TRUE, FALSE. */
};

/* Framebuffer types */
enum
{
    vHidd_FrameBuffer_None,
    vHidd_FrameBuffer_Direct,
    vHidd_FrameBuffer_Mirrored
};

/**** BitMap definitions ******************************************************/


        /* Types */


typedef UWORD HIDDT_ColComp;    /* Color component */
typedef ULONG HIDDT_Pixel;

        
typedef struct
{
   HIDDT_ColComp        red;
   HIDDT_ColComp        green;
   HIDDT_ColComp        blue;
   HIDDT_ColComp        alpha;
   
   HIDDT_Pixel          pixval;
   
} HIDDT_Color;



typedef struct
{
    ULONG entries;
    HIDDT_Pixel *pixels;
    
} HIDDT_PixelLUT;

typedef struct
{
    ULONG entries;
    HIDDT_Color *colors;
    
} HIDDT_ColorLUT;



/* Standard pixelformats */
enum
{
    /* Pseudo formats. These are not real pixelformats but are passed
       for example to HIDD_BM_PutImage() to tell the format of the data
    */
      
    vHidd_StdPixFmt_Unknown,
    vHidd_StdPixFmt_Native,
    vHidd_StdPixFmt_Native32,
        
    num_Hidd_PseudoStdPixFmt,
        
    /* Chunky formats. The order here must match those in ../stdpixfmts_??.h array !! */
    vHidd_StdPixFmt_RGB24 = num_Hidd_PseudoStdPixFmt,
    vHidd_StdPixFmt_BGR24,
    vHidd_StdPixFmt_RGB16,
    vHidd_StdPixFmt_RGB16_LE,
    vHidd_StdPixFmt_BGR16,
    vHidd_StdPixFmt_BGR16_LE,
    vHidd_StdPixFmt_RGB15,
    vHidd_StdPixFmt_RGB15_LE,
    vHidd_StdPixFmt_BGR15,
    vHidd_StdPixFmt_BGR15_LE,
    vHidd_StdPixFmt_ARGB32,
    vHidd_StdPixFmt_BGRA32,
    vHidd_StdPixFmt_RGBA32,
    vHidd_StdPixFmt_ABGR32,
    vHidd_StdPixFmt_0RGB32,
    vHidd_StdPixFmt_BGR032,
    vHidd_StdPixFmt_RGB032,
    vHidd_StdPixFmt_0BGR32,
    vHidd_StdPixFmt_LUT8,
    vHidd_StdPixFmt_Plane,

    num_Hidd_AllPf
};

#if AROS_BIG_ENDIAN
#define vHidd_StdPixFmt_ARGB32_Native vHidd_StdPixFmt_ARGB32
#define vHidd_StdPixFmt_BGRA32_Native vHidd_StdPixFmt_BGRA32
#define vHidd_StdPixFmt_RGBA32_Native vHidd_StdPixFmt_RGBA32
#define vHidd_StdPixFmt_ABGR32_Native vHidd_StdPixFmt_ABGR32
#define vHidd_StdPixFmt_0RGB32_Native vHidd_StdPixFmt_0RGB32
#define vHidd_StdPixFmt_BGR032_Native vHidd_StdPixFmt_BGR032
#define vHidd_StdPixFmt_RGB032_Native vHidd_StdPixFmt_RGB032
#define vHidd_StdPixFmt_0BGR32_Native vHidd_StdPixFmt_0BGR32
#else
#define vHidd_StdPixFmt_ARGB32_Native vHidd_StdPixFmt_BGRA32
#define vHidd_StdPixFmt_BGRA32_Native vHidd_StdPixFmt_ARGB32
#define vHidd_StdPixFmt_RGBA32_Native vHidd_StdPixFmt_ABGR32
#define vHidd_StdPixFmt_ABGR32_Native vHidd_StdPixFmt_RGBA32
#define vHidd_StdPixFmt_0RGB32_Native vHidd_StdPixFmt_BGR032
#define vHidd_StdPixFmt_BGR032_Native vHidd_StdPixFmt_0RGB32
#define vHidd_StdPixFmt_RGB032_Native vHidd_StdPixFmt_0BGR32
#define vHidd_StdPixFmt_0BGR32_Native vHidd_StdPixFmt_RGB032
#endif

#define FIRST_RGB_STDPIXFMT             vHidd_StdPixFmt_RGB24
#define LAST_RGB_STDPIXFMT              vHidd_StdPixFmt_0BGR32
#define NUM_RGB_STDPIXFMT               (vHidd_StdPixFmt_0BGR32 - vHidd_StdPixFmt_RGB24 + 1)

#define num_Hidd_StdPixFmt              (num_Hidd_AllPf - num_Hidd_PseudoStdPixFmt)

#define IS_PSEUDO_STDPIXFMT(stdpf)      ( (stdpf) > 0 && (stdpf) < num_Hidd_PseudoStdPixFmt )
    
#define IS_REAL_STDPIXFMT(stdpf)        ( (stdpf) >= num_Hidd_PseudoStdPixFmt && (stdpf) < num_Hidd_AllPf)
#define IS_STDPIXFMT(stdpf)             ( (stdpf) > 0 && (stdpf) < num_Hidd_AllPf )

#define REAL_STDPIXFMT_IDX(stdpf)       ( (stdpf) - num_Hidd_PseudoStdPixFmt )




#define MAP_SHIFT                       ((sizeof (HIDDT_Pixel) - sizeof (HIDDT_ColComp)) *8)
#define SHIFT_UP_COL(col)               ((HIDDT_Pixel)((col) << MAP_SHIFT))

#define MAP_COLCOMP(comp, val, pixfmt)  \
        ((SHIFT_UP_COL(val) >> (pixfmt)->comp ## _shift) & (pixfmt)->comp ## _mask)
        

#define MAP_RGB(r, g, b, pixfmt)        \
          MAP_COLCOMP(red,   r, pixfmt) \
        | MAP_COLCOMP(green, g, pixfmt) \
        | MAP_COLCOMP(blue,  b, pixfmt) 


#define MAP_RGBA(r, g, b, a, pixfmt)    \
          MAP_COLCOMP(red,   r, pixfmt)         \
        | MAP_COLCOMP(green, g, pixfmt) \
        | MAP_COLCOMP(blue,  b, pixfmt) \
        | MAP_COLCOMP(alpha,  a, pixfmt)
        
        

#define SHIFT_DOWN_PIX(pix) ((pix) >> MAP_SHIFT)
#define GET_COLCOMP(comp, pix, pixfmt) \
        SHIFT_DOWN_PIX( ( (pix) & (pixfmt)-> comp ## _mask) << pixfmt-> comp ## _shift )

#define RED_COMP(pix, pixfmt)   GET_COLCOMP(red,   pix, pixfmt)
#define GREEN_COMP(pix, pixfmt) GET_COLCOMP(green, pix, pixfmt)
#define BLUE_COMP(pix, pixfmt)  GET_COLCOMP(blue,  pix, pixfmt)
#define ALPHA_COMP(pix, pixfmt) GET_COLCOMP(alpha,  pix, pixfmt)

typedef struct
{
    UWORD           depth;
    UWORD           size;               /* Size of pixel in bits */
    UWORD           bytes_per_pixel;

    HIDDT_Pixel     red_mask;
    HIDDT_Pixel     green_mask;
    HIDDT_Pixel     blue_mask;
    HIDDT_Pixel     alpha_mask;

    UBYTE           red_shift;
    UBYTE           green_shift;
    UBYTE           blue_shift;
    UBYTE           alpha_shift;

    HIDDT_Pixel     clut_mask;
    UBYTE           clut_shift;

    HIDDT_StdPixFmt stdpixfmt;          /* Number of corresponding standard format */
    ULONG           flags;              /* Flags, see below */
        
} HIDDT_PixelFormat;

#include <interface/Hidd_PixFmt.h>

typedef ULONG (*HIDDT_RGBConversionFunction)(APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt, 
                                             APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt,
                                             UWORD width, UWORD height);

#include <interface/Hidd_BitMap.h>

#define CLID_Hidd_BitMap IID_Hidd_BitMap

#define IS_BITMAP_ATTR(attr, idx) \
        ( ( ( idx ) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)


/**** Graphics context definitions ********************************************/

#include <interface/Hidd_GC.h>

#define CLID_Hidd_GC IID_Hidd_GC

/* Drawmodes for a graphics context */
#define vHidd_GC_DrawMode_Clear         0x00 /* 0                   */
#define vHidd_GC_DrawMode_And           0x01 /* src AND dst         */
#define vHidd_GC_DrawMode_AndReverse    0x02 /* src AND NOT dst     */
#define vHidd_GC_DrawMode_Copy          0x03 /* src                 */
#define vHidd_GC_DrawMode_AndInverted   0x04 /* NOT src AND dst     */
#define vHidd_GC_DrawMode_NoOp          0x05 /* dst                 */
#define vHidd_GC_DrawMode_Xor           0x06 /* src XOR dst         */
#define vHidd_GC_DrawMode_Or            0x07 /* src OR dst          */
#define vHidd_GC_DrawMode_Nor           0x08 /* NOT src AND NOT dst */
#define vHidd_GC_DrawMode_Equiv         0x09 /* NOT src XOR dst     */
#define vHidd_GC_DrawMode_Invert        0x0A /* NOT dst             */
#define vHidd_GC_DrawMode_OrReverse     0x0B /* src OR NOT dst      */
#define vHidd_GC_DrawMode_CopyInverted  0x0C /* NOT src             */
#define vHidd_GC_DrawMode_OrInverted    0x0D /* NOT src OR dst      */
#define vHidd_GC_DrawMode_Nand          0x0E /* NOT src OR NOT dst  */
#define vHidd_GC_DrawMode_Set           0x0F /* 1                   */

#define vHidd_GC_ColExp_Transparent     (1 << 0)
#define vHidd_GC_ColExp_Opaque          (1 << 1)

/*******************************************************/
/**  PROTECTED DATA 
        !! These structures are at the top of the gfx hidd baseclasses.
        DO NEVER ATTEMPT TO ACCESS THESE FROM OUTSIDE OF SUBCLASSES.
        DON'T EVEN THINK ABOUT IT !!!
        THEY SHOULD ONLY BE ACCESSED FROM THE SUBCLASSES, AND THEN
        BY USING THE MACROS BELOW !! 
*/

struct _hidd_bitmap_protected
{
    OOP_Object *pixfmt;
};


#define BM_PIXFMT(bm) \
 ((HIDDT_PixelFormat *)(((struct _hidd_bitmap_protected *)bm)->pixfmt))
#define BM_DEPTH(bm)    BM_PIXFMT(bm)->depth


/****** !!!! PROTECTED DATA !!!!
        This typedef can be used to acces a GC object directly from within
        a bitmap class, but NEVER EVER use it to access a GC object
        from outside a bitmap class. And when accessing from inside a
        bitmap class, use the macros below !
*/

typedef struct
{
    struct Rectangle *clipRect;
    HIDDT_Pixel       fg;         /* foreground color                                 */
    HIDDT_Pixel       bg;         /* background color                                 */
    ULONG             colMask;    /* ColorMask prevents some color bits from changing */
    UWORD             linePat;    /* LinePattern                                      */
    UBYTE             linePatCnt; /* LinePattern start bit                            */
    UBYTE             drMode;     /* drawmode                                         */
    UBYTE             colExp;
} HIDDT_GC_Intern;

#define GCINT(gc)           ((HIDDT_GC_Intern *)(gc))
#define GC_FG(gc)           (GCINT(gc)->fg)
#define GC_BG(gc)           (GCINT(gc)->bg)
#define GC_DRMD(gc)         (GCINT(gc)->drMode)
#define GC_COLMASK(gc)      (GCINT(gc)->colMask)
#define GC_LINEPAT(gc)      (GCINT(gc)->linePat)
#define GC_LINEPATCNT(gc)   (GCINT(gc)->linePatCnt)
#define GC_COLEXP(gc)       (GCINT(gc)->colExp)

#define GC_DOCLIP(gc)       (GCINT(gc)->clipRect)
#define GC_CLIPX1(gc)       (GCINT(gc)->clipRect->MinX)
#define GC_CLIPY1(gc)       (GCINT(gc)->clipRect->MinY)
#define GC_CLIPX2(gc)       (GCINT(gc)->clipRect->MaxX)
#define GC_CLIPY2(gc)       (GCINT(gc)->clipRect->MaxY)

/****************** PixFmt definitions **************************/

/* Color model, bitmap type, and swapping bytes flag are stored
   in Flags member of the HIDDT_PixelFormat structure */

/* CM == Color model */
enum
{ 
    vHidd_ColorModel_TrueColor,
    vHidd_ColorModel_DirectColor,
    vHidd_ColorModel_Palette,
    vHidd_ColorModel_StaticPalette,
    vHidd_ColorModel_GrayScale,
    vHidd_ColorModel_StaticGray,

    num_Hidd_CM
};


/* Bitmap types */
enum
{
    vHidd_BitMapType_Unknown,
    vHidd_BitMapType_Chunky,
    vHidd_BitMapType_Planar,
    vHidd_BitMapType_InterleavedPlanar,
    
    num_Hidd_BitMapTypes
};


#define vHidd_ColorModel_Mask   0x0000000F
#define vHidd_ColorModel_Shift  0
#define vHidd_BitMapType_Mask   0x000000F0
#define vHidd_BitMapType_Shift  4

/* stegerg: The SwapPixelBytes flag is used to indicate that
            one must swap the pixel bytes after reading a pixel
            and before writing a pixel when doing some calculations
            with the pixfmt's shift/mask values. This is for
            pixel format which otherwise cannot be described through
            shift/mask values. For example a 0x0RRRRRGG 0xGGGBBBBB 16 bit
            pixfmt on a little endian machine, where a WORD-pixel-access
            requires the pixel-value to be in 0xGGGBBBBB0RRRRRGG format. */
            
#define vHidd_PixFmt_SwapPixelBytes_Flag 0x00000100

#define HIDD_PF_COLMODEL(pf) ( ((pf)->flags & vHidd_ColorModel_Mask) >> vHidd_ColorModel_Shift )
#define SET_PF_COLMODEL(pf, cm) \
     (pf)->flags &= ~vHidd_ColorModel_Mask;     \
     (pf)->flags |= ( (cm) << vHidd_ColorModel_Shift);
     

#define IS_PALETTIZED(pf) (    (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_Palette)       \
                            || (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_StaticPalette) )
                            
#define IS_TRUECOLOR(pf)        ( (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_TrueColor) )
#define IS_PALETTE(pf)          ( (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_Palette) )
#define IS_STATICPALETTE(pf)    ( (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_StaticPalette) )



#define HIDD_PF_BITMAPTYPE(pf) ( ((pf)->flags & vHidd_BitMapType_Mask) >> vHidd_BitMapType_Shift )
#define SET_PF_BITMAPTYPE(pf, bmt)      \
     (pf)->flags &= ~vHidd_BitMapType_Mask;     \
     (pf)->flags |= ( (bmt) << vHidd_BitMapType_Shift );


#define PF_GRAPHTYPE(cmodel, bmtype)    \
    (   (vHidd_ColorModel_ ## cmodel << vHidd_ColorModel_Shift) \
      | (vHidd_BitMapType_ ## bmtype << vHidd_BitMapType_Shift) )

#define SET_PF_SWAPPIXELBYTES_FLAG(pf, on) \
    do \
    { \
        if (on) \
            (pf)->flags |= vHidd_PixFmt_SwapPixelBytes_Flag; \
        else \
            (pf)->flags &= ~vHidd_PixFmt_SwapPixelBytes_Flag; \
    } while (0)
    
#define HIDD_PF_SWAPPIXELBYTES(pf) \
    ( ( (pf)->flags & vHidd_PixFmt_SwapPixelBytes_Flag) ? 1 : 0 )

#define IS_PIXFMT_ATTR(attr, idx) \
        ( ( ( idx ) = (attr) - HiddPixFmtAttrBase) < num_Hidd_PixFmt_Attrs)



/********** Planar bitmap *******************/

#include <interface/Hidd_PlanarBM.h>

#define CLID_Hidd_PlanarBM IID_Hidd_PlanarBM

#define IS_PLANARBM_ATTR(attr, idx) \
        ( ( ( idx ) = (attr) - HiddPlanarBMAttrBase) < num_Hidd_PlanarBM_Attrs)
  


/********** Chunky bitmap *******************/

#include <interface/Hidd_ChunkyBM.h>

#define CLID_Hidd_ChunkyBM IID_Hidd_ChunkyBM

#define IS_CHUNKYBM_ATTR(attr, idx) \
        ( ( ( idx ) = (attr) - HiddChunkyBMAttrBase) < num_Hidd_ChunkyBM_Attrs)


/********** ColorMap *******************/

#include <interface/Hidd_ColorMap.h>

#define CLID_Hidd_ColorMap IID_Hidd_ColorMap

#define IS_COLORMAP_ATTR(attr, idx) \
        ( ( ( idx ) = (attr) - HiddColorMapAttrBase) < num_Hidd_ColorMap_Attrs)


/************* Sync class **************************************/

/* Sync flags */
#define vHidd_Sync_HSyncPlus            0x0001  /* HSYNC + if set */
#define vHidd_Sync_VSyncPlus            0x0002  /* VSYNC + if set */
#define vHidd_Sync_Interlaced           0x0004  /* Interlaced mode */
#define vHidd_Sync_DblScan              0x0008  /* Double scanline */

#include <interface/Hidd_Sync.h>

#define IS_SYNC_ATTR(attr, idx) \
        ( ( ( idx ) = (attr) - HiddSyncAttrBase) < num_Hidd_Sync_Attrs)

/************* Video overlay class *****************************/

#include <interface/Hidd_Overlay.h>

#define IS_OVERLAY_ATTR(attr, idx) \
        (((idx) = (attr) - HiddOverlayAttrBase) < num_Hidd_Overlay_Attrs)

/************* Graphics class *****************************/

#include <interface/Hidd_Gfx.h>

#define IS_GFX_ATTR(attr, idx)  \
        ( ( ( idx ) = (attr) - HiddGfxAttrBase) < num_Hidd_Gfx_Attrs)

#define CLID_Hidd_Gfx IID_Hidd_Gfx

/* Parameter tags for the MemoryAttribs attribute */
enum
{
    tHidd_Gfx_MemTotal = tHidd_Gfx_MemAttribBASE,       // Total video memory
    tHidd_Gfx_MemFree,                                  // unused video memory
    tHidd_Gfx_MemAddressableTotal,                      // Total addressable video memory
    tHidd_Gfx_MemAddressableFree,                       // unused addressable video memory
    tHidd_Gfx_MemClock,                                 // video card's memory clock in Hz
};

/* Compatability types */
#define pHidd_Gfx_Gamma pHidd_Gfx_GetGamma

/* How we hide HIDD bitmaps inside struct BitMap */
#define HIDD_BM_PAD_MAGIC        0x6148 /* 'Ah' - AROS HIDD */
#define IS_HIDD_BM(bitmap) ((bitmap)->pad == HIDD_BM_PAD_MAGIC)

#define HIDD_BM_EXTRAPLANES       8
struct HiddBitMapPlaneData
{
    STACKED OOP_Object *HBMPD_Object;
    STACKED struct monitor_driverdata *HBMPD_DriverData;
    STACKED OOP_Object *HBMPD_ColMap;
    STACKED HIDDT_ColorModel HBMPD_ColMod;
    STACKED HIDDT_Pixel *HBMPD_PixTab;
    STACKED LONG HBMPD_RealDepth;
    STACKED ULONG HBMPD_Flags;
    STACKED HIDDT_ModeID HBMPD_HiddMode;
};

#define HIDD_BM_OBJ(bitmap)       (((struct HiddBitMapPlaneData *)&(bitmap)->Planes[HIDD_BM_EXTRAPLANES])->HBMPD_Object)
#define HIDD_BM_DRVDATA(bitmap)   (((struct HiddBitMapPlaneData *)&(bitmap)->Planes[HIDD_BM_EXTRAPLANES])->HBMPD_DriverData)
#define HIDD_BM_COLMAP(bitmap)	  (((struct HiddBitMapPlaneData *)&(bitmap)->Planes[HIDD_BM_EXTRAPLANES])->HBMPD_ColMap)
#define HIDD_BM_COLMOD(bitmap)    (((struct HiddBitMapPlaneData *)&(bitmap)->Planes[HIDD_BM_EXTRAPLANES])->HBMPD_ColMod)
#define HIDD_BM_PIXTAB(bitmap)	  (((struct HiddBitMapPlaneData *)&(bitmap)->Planes[HIDD_BM_EXTRAPLANES])->HBMPD_PixTab)
#define HIDD_BM_REALDEPTH(bitmap) (((struct HiddBitMapPlaneData *)&(bitmap)->Planes[HIDD_BM_EXTRAPLANES])->HBMPD_RealDepth)
#define HIDD_BM_FLAGS(bitmap)	  (((struct HiddBitMapPlaneData *)&(bitmap)->Planes[HIDD_BM_EXTRAPLANES])->HBMPD_Flags)
#define HIDD_BM_HIDDMODE(bitmap)  (((struct HiddBitMapPlaneData *)&(bitmap)->Planes[HIDD_BM_EXTRAPLANES])->HBMPD_HiddMode)

#endif /* HIDD_GRAPHICS_H */
