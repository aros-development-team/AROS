#ifndef HIDD_GRAPHICS_H
#define HIDD_GRAPHICS_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for the Gfx Hidd system.
    Lang: english
*/

#include <graphics/gfx.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/utility.h>

/**** Graphics definitions ****************************************************/

#define CLID_HW_Gfx      "hw.gfx"

typedef OOP_Object *HIDDT_BitMap;
typedef OOP_Object *HIDDT_GC;

#ifdef __GRAPHICS_NOHIDDBASE__
#define __Hidd_Gfx_NOMETHODBASE__
#define __Hidd_Display_NOMETHODBASE__
#define __Hidd_DMEnum_NOMETHODBASE__
#define __Hidd_Compositor_NOMETHODBASE__
#define __Hidd_BitMap_NOMETHODBASE__
#define __Hidd_ColorMap_NOMETHODBASE__
#define __Hidd_GC_NOMETHODBASE__
#define __Hidd_PlanarBM_NOMETHODBASE__
#endif

/* Sprite types */
#define vHidd_SpriteType_3Plus1      0x01 /* Color 0 transparent, 1-3 visible                */
#define vHidd_SpriteType_2Plus1      0x02 /* Color 0 transparrent, 1 undefined, 2-3 visible */
#define vHidd_SpriteType_DirectColor 0x04 /* Hi- or truecolor image                          */

#define tHidd_Gfx_BASE                  TAG_USER
#define tHidd_Gfx_QuerModeBASE          (tHidd_Gfx_BASE + 0x1000)
#define tHidd_Gfx_CursorBASE            (tHidd_Gfx_BASE + 0x2000)
#define tHidd_Gfx_MemAttribBASE         (tHidd_Gfx_BASE + 0x3000)
#define tHidd_Gfx_BMHistBASE            (tHidd_Gfx_BASE + 0x4000)

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

/* typedefs for the callbacks used by gfx.hidd drivers */
typedef void (*Gfx_ActiveCallBack_t)(void *, OOP_Object *);
typedef void (*Gfx_DisplayChangeCallBack_t)(void *, IPTR, void *);

/* DisplayChange callback types */
typedef enum
{
    vHidd_Gfx_DisplayChange_State,
    vHidd_Gfx_DisplayChange_Characteristics
    
} HIDDT_DisplayChangeType;

/* A structure passed to Gfx_DisplayChangeCallBack_t callbacks */
struct HIDD_DisplayStateData
{
    ULONG               dFlags;
    HIDDT_DPMSLevel     dDPMS;
};

/* A structure passed to Gfx_DisplayChangeCallBack_t callbacks */
struct HIDD_DisplayCharacteristicData
{
    APTR                dReserved;
    struct Rectangle    dBounds;
};

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
#define COMPB_SCALE             4
#define COMPF_SCALE             (1 << COMPB_SCALE)      /* screens scale to the display:   */
                                                        /* DisplayToBMCoords gives their   */
                                                        /* own pixels                      */
#define COMPB_ALPHA             7
#define COMPF_ALPHA             (1 << COMPB_ALPHA)
#define COMPB_SAME              8
#define COMPF_SAME              (1 << COMPB_SAME)       /* can only composit the same sync */

struct ViewPort;
struct View;

/*
 * One thing the compositor places on a display: a whole screen bitmap or a
 * composited layer surface inside a screen. Src and Dest are in pixels; a
 * Dest of another size than Src asks for scaling. Shape (display relative)
 * clips the request, NULL means the Dest rectangle.
 */
struct HIDD_CompositeRequest
{
    struct HIDD_CompositeRequest *Next;      /* back to front within the parent */
    OOP_Object                   *BitMap;    /* HIDD bitmap object to show      */
    struct Rectangle              Src;       /* source rectangle in BitMap      */
    struct Rectangle              Dest;      /* destination, display pixels     */
    struct Region                *Shape;     /* NULL = Dest rectangle           */
    UBYTE                         Opacity;   /* 0..255 constant alpha           */
    UBYTE                         AlphaMode; /* vHidd_CompAlpha_*               */
    UWORD                         Flags;     /* CRF_*                           */
    ULONG                         ZOrder;
    APTR                          UserData;  /* compositor private              */
};

#define vHidd_CompAlpha_None     0
#define vHidd_CompAlpha_Fixed    1
#define vHidd_CompAlpha_PerPixel 2

#define CRB_OPAQUE          0
#define CRF_OPAQUE          (1 << CRB_OPAQUE)   /* covers everything behind it        */
#define CRB_PLANE_OK        1
#define CRF_PLANE_OK        (1 << CRB_PLANE_OK) /* may be scanned out by a plane      */
#define CRB_PLANE_REQUIRED  2
#define CRF_PLANE_REQUIRED  (1 << CRB_PLANE_REQUIRED) /* must not be blitted          */
#define CRB_SCALED          3
#define CRF_SCALED          (1 << CRB_SCALED)   /* Dest size differs from Src size    */

/* A structure passed to ShowViewPorts() method */
struct HIDD_ViewPortData
{
    struct HIDD_ViewPortData *Next;     /* Pointer to a next bitmap             */
    OOP_Object               *Bitmap;   /* The bitmap object itself             */
    struct ViewPortExtra     *vpe;      /* Associated ViewPortExtra             */
    APTR                      UserData; /* The driver can keep own stuff here   */
    OOP_Object               *PreviousBitmap; /* Kept alive until display swap    */
    ULONG                     Flags;
    /* Composition parameters of this ViewPort, set by graphics.library from
     * the ViewPortExtra (VideoControl() VTAG_COMPOSITION_* tags).           */
    ULONG                     CompFlags;      /* COMPF_* for this screen          */
    struct Hook              *PreAlphaHook;   /* called on the backdrop before an
                                               * alpha screen is blended over it   */
    struct HIDD_CompositeRequest *Surfaces;   /* composited layers of this screen,
                                               * back to front; NULL = none        */
};

#define HIDD_VPDF_PREVIOUS_BITMAP_OWNED (1UL << 0)

#define vHidd_ModeID_Invalid ((HIDDT_ModeID)-1)


/* Flags */

/* This will make the gfx hidd, copy back from the framebuffer into
   the old bitmap you were using. Use this option with extreme
   prejudice:  YOU MUST BE TOTALLY SURE THAT THE BITMAP
   YOU CALLED SHOW ON BEFORE THIS CALL HAS NOT BEEN DISPOSED
*/
#define fHidd_Display_Show_CopyBack 0x01

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

/**** BitMap Histogram ******************************************************************/

#include <interface/Hidd_BMHistogram.h>

#define CLID_Hidd_BMHistogram IID_Hidd_BMHistogram

enum
{
    tHidd_BMHistogram_Sort = tHidd_Gfx_BMHistBASE,      /* Sort Mode                    */
};


enum
{
    vBMHistogram_NextEntry      = -1,           /* FindColorEntry special value ...     */
    vBMHistogram_Sort_Frequency = (1 << 0),     /* Sort mode values                     */
    vBMHistogram_Sort_Intensity = (1 << 1),     /* Sort mode values                     */
};

/**** Graphics context definitions ******************************************************/

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

/****************************************************************************************/
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
    vHidd_ColorModel_Video,     /* scan-out only video bitmap (aHidd_BitMap_VideoFormat) */

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

/************* DMEnum class *****************************/

#include <interface/Hidd_DMEnum.h>

#define IS_DMENUM_ATTR(attr, idx)  \
        ( ( ( idx ) = (attr) - HiddDMEnumAttrBase) < num_Hidd_DMEnum_Attrs)

#define CLID_Hidd_DMEnum IID_Hidd_DMEnum

/************* Display class *****************************/

/* Connector types (aHidd_Display_ConnectorType) */
#define vHidd_ConnectorType_Unknown     0
#define vHidd_ConnectorType_VGA         1
#define vHidd_ConnectorType_DVI         2
#define vHidd_ConnectorType_HDMI        3
#define vHidd_ConnectorType_DisplayPort 4
#define vHidd_ConnectorType_eDP         5
#define vHidd_ConnectorType_LVDS        6
#define vHidd_ConnectorType_TV          7
#define vHidd_ConnectorType_DSI         8
#define vHidd_ConnectorType_Virtual     9
#define vHidd_ConnectorType_USBC        10

#include <interface/Hidd_Display.h>

#define IS_DISPLAY_ATTR(attr, idx)  \
        ( ( ( idx ) = (attr) - HiddDisplayAttrBase) < num_Hidd_Display_Attrs)

#define CLID_Hidd_Display IID_Hidd_Display

/************* Compositor class *****************************/

/*
 * Every Display owns a compositor object (aHidd_Display_Compositor). The
 * compositor is the composition policy: which bitmaps are shown where and
 * how. graphics.library and Intuition reach it only through the Display.
 */

#include <interface/Hidd_Compositor.h>

#define IS_COMPOSITOR_ATTR(attr, idx)  \
        ( ( ( idx ) = (attr) - HiddCompositorAttrBase) < num_Hidd_Compositor_Attrs)

#define CLID_Hidd_Compositor IID_Hidd_Compositor

/*
 * Scan-out planes. A display scans out one or more planes, each showing a
 * rectangle of a bitmap at a position on the display, optionally scaled and
 * blended. The base Display has exactly one plane (the primary), which is
 * what Show() drives; a driver with more describes them through
 * GetPlaneInfo() and applies a whole set at once with CommitPlanes(). The
 * display's compositor assigns composition requests to planes within these
 * constraints and blits whatever gets no plane into the primary one.
 */
#define vHidd_Plane_Primary     0   /* full display scan-out, always present       */
#define vHidd_Plane_Overlay     1   /* composed above the primary at a position    */
#define vHidd_Plane_Cursor      2   /* small, topmost; the mouse pointer           */

#define PIB_NEEDS_DISPLAYMEM    0   /* only bitmaps in display memory can be scanned */
#define PIF_NEEDS_DISPLAYMEM    (1 << PIB_NEEDS_DISPLAYMEM)
#define PIB_FULL_WIDTH_ONLY     1   /* the plane spans the whole display width        */
#define PIF_FULL_WIDTH_ONLY     (1 << PIB_FULL_WIDTH_ONLY)
#define PIB_VSPLIT_ONLY         2   /* planes stack vertically only (copper lists)    */
#define PIF_VSPLIT_ONLY         (1 << PIB_VSPLIT_ONLY)
#define PIB_COLORKEY            3   /* destination colour key: the plane shows only    */
#define PIF_COLORKEY            (1 << PIB_COLORKEY) /* where the primary has the key  */
#define PIB_HIDE_OBSCURED       4   /* key-less plane partly covered: hide it rather   */
#define PIF_HIDE_OBSCURED       (1 << PIB_HIDE_OBSCURED) /* than show it on top       */

#define HIDD_PLANE_SCALE_UNITY  0x10000     /* 16.16 */

struct HIDD_PlaneInfo
{
    ULONG   Type;           /* vHidd_Plane_*                                    */
    ULONG   ZMin, ZMax;     /* z-order positions this plane can take            */
    ULONG   PixFmtMask;     /* bit per vHidd_StdPixFmt_* the plane can scan out */
    ULONG   ScaleMin;       /* 16.16; both UNITY = no scaling                   */
    ULONG   ScaleMax;
    ULONG   AlphaModes;     /* bit per vHidd_CompAlpha_*                        */
    UWORD   PosAlignX;      /* position/size alignment in pixels (1 = any)      */
    UWORD   PosAlignY;
    UWORD   MaxWidth;       /* 0 = up to the display size                       */
    UWORD   MaxHeight;
    ULONG   Flags;          /* PIF_*                                            */
    ULONG   VideoFmtMask;   /* bit per vHidd_VideoFmt_* the plane can scan out  */
};

struct HIDD_PlaneState
{
    ULONG            Plane;     /* plane index, 0 .. aHidd_Display_PlaneCount-1 */
    OOP_Object      *BitMap;    /* NULL disables the plane                      */
    struct Rectangle Src;       /* part of BitMap shown                         */
    struct Rectangle Dest;      /* where on the display (size may differ = scale) */
    UBYTE            Opacity;   /* for vHidd_CompAlpha_Fixed                    */
    UBYTE            AlphaMode; /* vHidd_CompAlpha_*                            */
    ULONG            ZOrder;
    ULONG            Flags;     /* PSF_*                                        */
    ULONG            ColorKey;  /* 0x00RRGGBB, with PSF_COLORKEY                */
};

/* HIDD_PlaneState.Flags */
#define PSF_COLORKEY            (1 << 0)    /* show only where the primary has ColorKey */

/*
 * Scan-out only video formats (aHidd_BitMap_VideoFormat, HIDD_PlaneInfo.VideoFmtMask,
 * moHidd_Display_QueryScanout). These are deliberately not vHidd_StdPixFmt_*: no
 * blit or conversion path understands them, only planes scan them out and the
 * owner writes them through moHidd_BitMap_LockBuffer.
 */
#define vHidd_VideoFmt_None     0
#define vHidd_VideoFmt_YUYV     1   /* packed 4:2:2, Y0 U Y1 V, 2 bytes per pixel */
#define vHidd_VideoFmt_UYVY     2   /* packed 4:2:2, U Y0 V Y1                    */
#define vHidd_VideoFmt_NV12     3   /* planar Y + interleaved UV, 4:2:0           */
#define vHidd_VideoFmt_I420     4   /* planar Y, U, V, 4:2:0                      */
#define vHidd_VideoFmt_YV12     5   /* planar Y, V, U, 4:2:0                      */
#define num_Hidd_VideoFmt       6

/* aHidd_BitMap_ColorSpace */
#define vHidd_ColorSpace_BT601  0
#define vHidd_ColorSpace_BT709  1

/* moHidd_BitMap_SwapBuffers flags */
#define fHidd_Swap_NoWait       (1 << 0)    /* return before the displaced buffer is off scan-out */

/* moHidd_Display_QueryScanout */
struct HIDD_ScanoutCaps
{
    ULONG   Flags;          /* SCF_*                                           */
    ULONG   MaxWidth;       /* largest source the plane scans out, 0 = display */
    ULONG   MaxHeight;
    ULONG   MaxBuffers;     /* buffers one scan-out bitmap may have (>= 1)     */
    ULONG   ScaleMin;       /* 16.16, as HIDD_PlaneInfo                        */
    ULONG   ScaleMax;
    ULONG   ColorKey;       /* the display's key colour when SCF_COLORKEY      */
};

#define SCF_PLANE               (1 << 0)    /* a plane can show this format at this size   */
#define SCF_COLORKEY            (1 << 1)    /* ... with a destination colour key           */
#define SCF_SCALE               (1 << 2)    /* ... scaled (ScaleMin/ScaleMax give the range) */

/*
 * Client surfaces (moHidd_Display_AttachSurface). The hook is called from the
 * compositor's context, with its lock dropped, as CallHookPkt(hook, display, msg).
 * It must not block; see the composition plan for the locking rule.
 */
struct HIDD_SurfaceHookMsg
{
    ULONG                         Type;     /* vHidd_SurfaceHook_*                  */
    struct HIDD_CompositeRequest *Request;  /* the attached request                 */
    struct Rectangle              Rect;     /* Damage: screen bitmap coordinates    */
    ULONG                         Plane;    /* PlaneGained: the plane index         */
};

#define vHidd_SurfaceHook_Damage        0   /* the screen changed under/around Dest: recompute Shape */
#define vHidd_SurfaceHook_PlaneGained   1   /* the surface is scanned out by a plane                 */
#define vHidd_SurfaceHook_PlaneLost     2   /* the surface is no longer on a plane                   */
#define vHidd_SurfaceHook_ScreenMoved   3   /* the screen moved on the display (informational)       */

/* aHidd_Display_ColorKey default of the base class (CyberGraphX used the same value) */
#define HIDD_DISPLAY_DEFAULT_COLORKEY   0x00080008

/* CommitPlanes() flags */
#define fHidd_Commit_NoWait     (1 << 0)    /* return before the new state is scanned out */
#define fHidd_Commit_Test       (1 << 1)    /* validate only, change nothing              */

/* aHidd_Compositor_ModePolicy */
#define vHidd_Compositor_ModePolicy_FollowTop    0 /* display switches to the frontmost screen's mode, no scaling     */
#define vHidd_Compositor_ModePolicy_Native       1 /* display keeps its own (highest) mode, every screen is scaled    */
#define vHidd_Compositor_ModePolicy_ScaleLargest 2 /* display takes the mode of the largest screen shown, the smaller */
                                                   /* screens are scaled up to it (prefs SCALE; NATIVEMODE = Native)   */

/* Message passed to the backfill hook and the pre-alpha hook */
struct HIDD_BackFillHookMsg
{
    void                        *dummy;
    struct Rectangle            *bounds;
    LONG                        offsetx;
    LONG                        offsety;
};

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
