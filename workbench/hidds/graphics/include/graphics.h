#ifndef HIDD_GRAPHICS_H
#define HIDD_GRAPHICS_H

/*
    Copyright (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Definitions for the Graphics HIDD system.
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include <utility/utility.h>


#define CLID_Hidd_Gfx           "hidd.graphics.graphics"
#define CLID_Hidd_BitMap        "hidd.graphics.bitmap"
#define CLID_Hidd_GC		"hidd.graphics.gc"


#define IID_Hidd_Gfx      "hidd.graphics.graphics"
#define IID_Hidd_BitMap   "hidd.graphics.bitmap"
#define IID_Hidd_GC       "hidd.graphics.gc"

#define IID_Hidd_GfxMode	"hidd.graphics.gfxmode"
#define IID_Hidd_PixFmt	"hidd.graphics.pixfmt"

/* Some "example" hidd bitmaps */

#define CLID_Hidd_ChunkyBM "hidd.graphics.bitmap.chunkybm"


typedef struct Object *HIDDT_BitMap;
typedef struct Object *HIDDT_GC;


/* Attrbases */
#define HiddGCAttrBase          __IHidd_GC
#define HiddGfxAttrBase         __IHidd_Gfx
#define HiddBitMapAttrBase      __IHidd_BitMap

#define HiddGfxModeAttrBase	__IHidd_GfxMode
#define HiddPixFmtAttrBase	__IHidd_PixFmt

extern AttrBase HiddGCAttrBase;
extern AttrBase HiddGfxAttrBase;
extern AttrBase HiddBitMapAttrBase;

extern AttrBase HiddGfxModeAttrBase;
extern AttrBase HiddPixFmtAttrBase;


/**** Graphics definitions ****************************************************/

enum
{
    /* Methods for a graphics hidd */

    moHidd_Gfx_NewGC = 0,       /* Its only allowd to use these methods   */
    moHidd_Gfx_DisposeGC,       /* to create and to dispose a GC and      */
    moHidd_Gfx_NewBitMap,       /* bitmap because only the graphics-hidd  */
    moHidd_Gfx_DisposeBitMap,   /* class knows which gc- and bitmap-class */
				/* works together.                        */
				
    /* This method is used only by subclasses, I repeat:
    ONLY BY SUBCLASSES, to register available modes in the baseclass
    */
    moHidd_Gfx_RegisterGfxModes,
    
    /* These methods are used by apps to get available modes */
    moHidd_Gfx_QueryGfxModes,
    moHidd_Gfx_ReleaseGfxModes,
    
    num_Hidd_Gfx_Methods
};

enum {
    aoHidd_Gfx_IsWindowed,
    aoHidd_Gfx_ActiveBMCallBack,
    aoHidd_Gfx_ActiveBMCallBackData,
    
    num_Hidd_Gfx_Attrs
};

#define aHidd_Gfx_IsWindowed 		(HiddGfxAttrBase + aoHidd_Gfx_IsWindowed		)
#define aHidd_Gfx_ActiveBMCallBack	(HiddGfxAttrBase + aoHidd_Gfx_ActiveBMCallBack		)
#define aHidd_Gfx_ActiveBMCallBackData	(HiddGfxAttrBase + aoHidd_Gfx_ActiveBMCallBackData	)

#define IS_GFX_ATTR(attr, idx)	\
	( ( ( idx ) = (attr) - HiddGfxAttrBase) < num_Hidd_Gfx_Attrs)


/* Parameter tags for the QueryGfxMode method */
enum {
	tHidd_GfxMode_MinWidth = TAG_USER,
	tHidd_GfxMode_MaxWidth,
	tHidd_GfxMode_MinHeight,
	tHidd_GfxMode_MaxHeight,
	tHidd_GfxMode_PixFmts
};




/* messages for a graphics hidd */

struct pHidd_Gfx_NewGC
{
    MethodID       mID;
    struct TagItem *attrList;
};

struct pHidd_Gfx_DisposeGC
{
    MethodID    mID;
    Object      *gc;
};

struct pHidd_Gfx_NewBitMap
{
    MethodID       mID;
    
    /* Subclasses should fill one of the below, and set the other one to NULL,
       and then call the base gfx hidd class through DoSuperMethod */
    Class	*classPtr;
    STRPTR	classID;
    
    
    struct TagItem *attrList;
};

struct pHidd_Gfx_DisposeBitMap
{
    MethodID    mID;
    Object      *bitMap;
};

struct pHidd_Gfx_RegisterGfxModes {
    MethodID mID;
    struct TagItem **modeTags;
};

struct pHidd_Gfx_QueryGfxModes {
    MethodID mID;
    struct TagItem *queryTags;
};

struct pHidd_Gfx_ReleaseGfxModes {
    MethodID mID;
#warning Use a OOP list class instead of this
    struct List *modeList;
};


struct ModeNode {
	struct MinNode node;
	Object *gfxMode;
};



/**** BitMap definitions ******************************************************/


	/* Types */


typedef UWORD HIDDT_ColComp;	/* Color component */

typedef ULONG HIDDT_Pixel;

	
typedef struct {
   HIDDT_ColComp	red;
   HIDDT_ColComp	green;
   HIDDT_ColComp	blue;
   HIDDT_ColComp	alpha;
   
   HIDDT_Pixel		pixval;
   
} HIDDT_Color;



typedef struct {
	ULONG entries;
	HIDDT_Pixel *pixels;
} HIDDT_PixelLUT;

typedef struct {
	ULONG entries;
	HIDDT_Color *colors;
} HIDDT_ColorLUT;


/* GT == Graphids Type */
enum { 
	vHidd_GT_TrueColor,
	vHidd_GT_Palette,
	vHidd_GT_StaticPalette,
	num_Hidd_GT
};

#define vHidd_GT_Mask 0x03

#define HIDD_PF_GRAPHTYPE(pf) ((pf)->flags & vHidd_GT_Mask)
#define IS_PALETTIZED(pf) (    (HIDD_PF_GRAPHTYPE(pf) == vHidd_GT_Palette)	\
			    || (HIDD_PF_GRAPHTYPE(pf) == vHidd_GT_StaticPalette) )
			    
#define IS_TRUECOLOR(pf) 	( (HIDD_PF_GRAPHTYPE(pf) == vHidd_GT_TrueColor) )
#define IS_PALETTE(pf)  	( (HIDD_PF_GRAPHTYPE(pf) == vHidd_GT_Palette) )
#define IS_STATICPALETTE(pf)  	( (HIDD_PF_GRAPHTYPE(pf) == vHidd_GT_StaticPalette) )

typedef ULONG HIDDT_StdPixFmt;
typedef ULONG HIDDT_DrawMode;

/* Standard pixelformats */
enum {
	vHidd_PixFmt_Unknown,
	vHidd_PixFmt_Native,
	vHidd_PixFmt_Native32,
	
	num_Hidd_PseudoPixFmt,
	
	/* Chunky formats */
	vHidd_PixFmt_RGB24 = num_Hidd_PseudoPixFmt,
	vHidd_PixFmt_RGB16,
	vHidd_PixFmt_ARGB32,
	vHidd_PixFmt_RGBA32,
	vHidd_PixFmt_LUT8,
	
	
	num_Hidd_PixFmt
};

#define num_Hidd_StdPixFmt (num_Hidd_PixFmt - num_Hidd_PseudoPixFmt)


#define MAP_SHIFT	((sizeof (HIDDT_Pixel) - sizeof (HIDDT_ColComp)) *8)
#define SHIFT_UP_COL(col) ((HIDDT_Pixel)((col) << MAP_SHIFT))

#define MAP_COLCOMP(comp, val, pixfmt)	\
	((SHIFT_UP_COL(val) >> (pixfmt)-> ## comp ## _shift) & (pixfmt)-> ## comp ## _mask)
	

#define MAP_RGB(r, g, b, pixfmt) 	\
	  MAP_COLCOMP(red,   r, pixfmt) \
	| MAP_COLCOMP(green, g, pixfmt)	\
	| MAP_COLCOMP(blue,  b, pixfmt) 


#define MAP_RGBA(r, g, b, pixfmt) 	\
	  MAP_COMP(red,   r, pixfmt) 	\
	| MAP_COMP(green, g, pixfmt)	\
	| MAP_COMP(blue,  b, pixfmt)	\
	| MAP_COMP(blue,  b, pixfmt)
	
	

#define SHIFT_DOWN_PIX(pix) ((pix) >> MAP_SHIFT)
#define GET_COLCOMP(comp, pix, pixfmt) \
	SHIFT_DOWN_PIX( ( (pix) & (pixfmt)-> ## comp ## _mask) << pixfmt-> ## comp ## _shift )

#define RED_COMP(pix, pixfmt)	GET_COLCOMP(red,   pix, pixfmt)
#define GREEN_COMP(pix, pixfmt)	GET_COLCOMP(green, pix, pixfmt)
#define BLUE_COMP(pix, pixfmt)	GET_COLCOMP(blue,  pix, pixfmt)

typedef struct {
	UWORD	depth;
	UWORD	size;	/* Size of pixel in bits */
	UWORD bytes_per_pixel;	
	
	HIDDT_Pixel red_mask;
	HIDDT_Pixel green_mask;
	HIDDT_Pixel blue_mask;
	HIDDT_Pixel alpha_mask;
	
	ULONG red_shift;
	ULONG green_shift;
	ULONG blue_shift;
	ULONG alpha_shift;
	
	HIDDT_Pixel clut_mask;
	UWORD clut_shift;
	
	HIDDT_StdPixFmt stdpixfmt;
	ULONG flags;
	
} HIDDT_PixelFormat;


enum
{
    /* Methods for a bitmap */

    moHidd_BitMap_SetColors,
    moHidd_BitMap_CopyBox,
    moHidd_BitMap_PutPixel,
    moHidd_BitMap_DrawPixel,
    moHidd_BitMap_PutImage,
    moHidd_BitMap_GetImage,
    moHidd_BitMap_GetPixel,
    moHidd_BitMap_DrawLine,
    moHidd_BitMap_DrawRect,
    moHidd_BitMap_FillRect,
    moHidd_BitMap_DrawEllipse,
    moHidd_BitMap_FillEllipse,
    moHidd_BitMap_DrawPolygon,
    moHidd_BitMap_FillPolygon,
    moHidd_BitMap_DrawText,
    moHidd_BitMap_FillText,
    moHidd_BitMap_FillSpan,
    moHidd_BitMap_Clear,
    moHidd_BitMap_BlitColorExpansion,
    moHidd_BitMap_MapColor,
    moHidd_BitMap_UnmapPixel,
    moHidd_BitMap_PutImageLUT,
    moHidd_BitMap_GetImageLUT,
    moHidd_BitMap_BytesPerLine,
    moHidd_BitMap_GetPixelFormat,
    moHidd_BitMap_ConvertPixels,
 
    /* This method is used only by subclasses, I repeat:
    ONLY BY SUBCLASSES, to register available modes in the baseclass
    */
    moHidd_BitMap_SetPixelFormat,
    moHidd_BitMap_SetColorMap,
    
    moHidd_BitMap_PrivateSet
};

enum {
    /* Attributes for a bitmap */
    aoHidd_BitMap_BitMap,        /* [..G] pointer to bitmap structure        */
    aoHidd_BitMap_Width,         /* [ISG] Bitmap with                          */
    aoHidd_BitMap_Height,        /* [ISG] Bitmap height                        */
    aoHidd_BitMap_Depth,         /* [I.G] Bitmap depth                         */
    aoHidd_BitMap_Displayable,   /* [I.G] BOOL bitmap is displayable           */
    aoHidd_BitMap_Visible,       /* [..G] Check if a bitmap is visible         */
    aoHidd_BitMap_BaseAddress,   /* [ISG] Bitmap adress in RAM                 */
    aoHidd_BitMap_IsLinearMem,   /* [..G] Is the bitmap memory contigous       */
    aoHidd_BitMap_BytesPerRow,   /* [..G] Number of bytes in a row             */

#if 0    
    aoHidd_BitMap_Mode,          /* [ISG] The display mode of this bitmap      */
    aoHidd_BitMap_BytesPerPixel, /* [..G] Number of byter per pixel            */
    aoHidd_BitMap_Format,        /* [..G] Tell the format of the bitmap data   */
    aoHidd_BitMap_AllocBuffer,   /* [I..] BOOL allocate buffer (default: TRUE) */
#endif
    
    aoHidd_BitMap_BestSize,      /* [..G] Best size for depth                  */
    aoHidd_BitMap_LeftEdge,      /* [I.G] Left edge position of the bitmap     */
    aoHidd_BitMap_TopEdge,       /* [I.G] Top edge position of the bitmap      */
    aoHidd_BitMap_ColorMap,      /* [..G] Colormap of the bitmap               */
    

    aoHidd_BitMap_Friend,	/* [I.G] Friend bitmap. The bitmap will be allocated so that it
    				   is optimized for blitting to this bitmap */

    aoHidd_BitMap_GfxHidd,
    aoHidd_BitMap_StdPixFmt,	/* What pixel format the bitmap should have */
    
    num_Hidd_BitMap_Attrs
};    

#define aHidd_BitMap_BitMap        (HiddBitMapAttrBase + aoHidd_BitMap_BitMap)
#define aHidd_BitMap_Width         (HiddBitMapAttrBase + aoHidd_BitMap_Width)
#define aHidd_BitMap_Height        (HiddBitMapAttrBase + aoHidd_BitMap_Height)
#define aHidd_BitMap_Depth         (HiddBitMapAttrBase + aoHidd_BitMap_Depth)
#define aHidd_BitMap_Displayable   (HiddBitMapAttrBase + aoHidd_BitMap_Displayable)
#define aHidd_BitMap_Visible       (HiddBitMapAttrBase + aoHidd_BitMap_Visible)
#define aHidd_BitMap_BaseAddress   (HiddBitMapAttrBase + aoHidd_BitMap_BaseAddress)
#define aHidd_BitMap_IsLinearMem   (HiddBitMapAttrBase + aoHidd_BitMap_IsLinearMem)

#if 0
#define aHidd_BitMap_Mode          (HiddBitMapAttrBase + aoHidd_BitMap_Mode)
#define aHidd_BitMap_Format        (HiddBitMapAttrBase + aoHidd_BitMap_Format)
#define aHidd_BitMap_BytesPerPixel (HiddBitMapAttrBase + aoHidd_BitMap_BytesPerPixel)
#define aHidd_BitMap_AllocBuffer   (HiddBitMapAttrBase + aoHidd_BitMap_AllocBuffer)
#endif

#define aHidd_BitMap_BytesPerRow   (HiddBitMapAttrBase + aoHidd_BitMap_BytesPerRow)
#define aHidd_BitMap_BestSize      (HiddBitMapAttrBase + aoHidd_BitMap_BestSize)
#define aHidd_BitMap_LeftEdge      (HiddBitMapAttrBase + aoHidd_BitMap_LeftEdge)
#define aHidd_BitMap_TopEdge       (HiddBitMapAttrBase + aoHidd_BitMap_TopEdge)
#define aHidd_BitMap_ColorMap	   (HiddBitMapAttrBase + aoHidd_BitMap_ColorMap)
#define aHidd_BitMap_Friend	   (HiddBitMapAttrBase + aoHidd_BitMap_Friend)
#define aHidd_BitMap_GfxHidd	   (HiddBitMapAttrBase + aoHidd_BitMap_GfxHidd)
#define aHidd_BitMap_StdPixFmt	   (HiddBitMapAttrBase + aoHidd_BitMap_StdPixFmt)



/* messages for a bitmap */

struct pHidd_BitMap_PrivateSet
{
    MethodID       mID;
    struct TagItem *attrList;
};

struct pHidd_BitMap_SetColors
{
    MethodID	mID;
    HIDDT_Color	*colors;
    ULONG	firstColor;
    ULONG	numColors;
};

/* messages for a graphics context */

struct pHidd_BitMap_PutPixel
{
    MethodID  mID;
    WORD x, y;
    HIDDT_Pixel pixel;
};

struct pHidd_BitMap_GetPixel
{
    MethodID  mID;
    WORD x, y;
};

struct pHidd_BitMap_DrawPixel
{
    MethodID  mID;
    Object	*gc;
    WORD x, y;
};

struct pHidd_BitMap_DrawLine
{
    MethodID    mID;
    Object	*gc;
    WORD        x1 ,y1, x2, y2;
};

struct pHidd_BitMap_CopyBox
{
    MethodID    mID;
    Object	*gc;
    WORD        srcX, srcY;
    Object      *dest;
    WORD        destX, destY;
    UWORD       width, height;
};

struct pHidd_BitMap_GetImage
{
    MethodID mID;
    UBYTE	*pixels;
    ULONG	modulo;
    WORD	x, y;
    WORD	width, height;
    HIDDT_StdPixFmt pixFmt;
};

struct pHidd_BitMap_PutImage
{
    MethodID mID;
    Object	*gc;
    UBYTE 	*pixels;
    ULONG	modulo;
    WORD	x, y;
    WORD	width, height;
    HIDDT_StdPixFmt pixFmt;
};


struct pHidd_BitMap_DrawRect
{
    MethodID    mID;
    Object *gc;
    WORD        minX, minY, maxX, maxY;
};

struct pHidd_BitMap_DrawEllipse
{
    MethodID    mID;
    Object *gc;
    WORD        x, y;
    UWORD       rx, ry;
};

struct pHidd_BitMap_DrawPolygon
{
    MethodID    mID;
    Object	*gc;
    WORD        n;         /* number of coordinates */
    WORD        *coords;   /* size 2*n              */
};

struct pHidd_BitMap_DrawText
{
    MethodID    mID;
    Object	*gc;
    WORD        x, y;      /* Start position, see autodocs */
    STRPTR      text;      /* Latin 1 string               */
    UWORD       length;    /* Number of characters to draw */
};

struct pHidd_BitMap_Clear
{
    MethodID    mID;
    Object	*gc;
};

struct pHidd_BitMap_BlitColorExpansion
{
    MethodID mID;
    Object	*gc;
    Object	*srcBitMap;
    WORD	srcX;
    WORD	srcY;
    WORD	destX;
    WORD	destY;
    UWORD	width;
    UWORD	height;
};

struct pHidd_BitMap_MapColor
{
    MethodID mID;
    HIDDT_Color *color;
};

struct pHidd_BitMap_UnmapPixel
{
    MethodID mID;
    HIDDT_Pixel pixel;
    HIDDT_Color *color;
};

struct pHidd_BitMap_PutImageLUT
{
    MethodID mID;
    Object	*gc;
    UBYTE 	*pixels;
    ULONG	modulo;
    WORD	x, y;
    WORD	width, height;
    HIDDT_PixelLUT *pixlut;
};

struct pHidd_BitMap_GetImageLUT
{
    MethodID mID;
    UBYTE	*pixels;
    ULONG	modulo;
    WORD	x, y;
    WORD	width, height;
    HIDDT_PixelLUT *pixlut;
};

struct pHidd_BitMap_GetPixelFormat
{
    MethodID mID;
    HIDDT_StdPixFmt stdPixFmt;
};


struct pHidd_BitMap_BytesPerLine
{
    MethodID mID;
    HIDDT_StdPixFmt pixFmt;
    ULONG width;
};


struct pHidd_BitMap_ConvertPixels
{
    MethodID mID;
    APTR *srcPixels;
    HIDDT_PixelFormat *srcPixFmt;
    
    ULONG srcMod;	/* Source modulo */
    
    APTR *dstBuf;
    HIDDT_PixelFormat *dstPixFmt;
    
    ULONG dstMod;
    
    ULONG width;
    ULONG height;

    HIDDT_PixelLUT *pixlut;
    
};

struct pHidd_BitMap_SetPixelFormat {
    MethodID mID;
    struct TagItem *pixFmtTags;
};


struct pHidd_BitMap_SetColorMap {
    MethodID mID;
    Object *colorMap;
};


/**** Graphics context definitions ********************************************/
    /* Methods for a graphics context */
    
enum {
    moHidd_GC_SetClipRect,
    moHidd_GC_UnsetClipRect

};

struct pHidd_GC_SetClipRect {
    MethodID mID;
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
};


struct pHidd_GC_UnsetClipRect {
    MethodID mID;
};

enum
{
    /* Attributes for a graphics context */

    aoHidd_GC_UserData,            /* [.SG] User data                          */
    aoHidd_GC_BitMap,              /* [I.G] Bitmap which this gc uses          */
    aoHidd_GC_Foreground,          /* [.SG] Foreground color                   */
    aoHidd_GC_Background,          /* [.SG] Background color                   */
    aoHidd_GC_DrawMode,            /* [.SG] Draw mode                          */
    aoHidd_GC_Font,                /* [.SG] Current font                       */
    aoHidd_GC_ColorMask,           /* [.SG] Prevents some color bits from      */
                                   /*       changing                           */
    aoHidd_GC_LinePattern,         /* [.SG] Pattern for line drawing          */
    aoHidd_GC_PlaneMask,           /* [.SG] Shape bitmap                       */
    aoHidd_GC_ColorExpansionMode,	   /* [.SG] Mode for color expansion */
    
    num_Hidd_GC_Attrs
};

#define aHidd_GC_UserData    (HiddGCAttrBase + aoHidd_GC_UserData)
#define aHidd_GC_BitMap      (HiddGCAttrBase + aoHidd_GC_BitMap)
#define aHidd_GC_Foreground  (HiddGCAttrBase + aoHidd_GC_Foreground)
#define aHidd_GC_Background  (HiddGCAttrBase + aoHidd_GC_Background)
#define aHidd_GC_DrawMode    (HiddGCAttrBase + aoHidd_GC_DrawMode)
#define aHidd_GC_Font        (HiddGCAttrBase + aoHidd_GC_Font)
#define aHidd_GC_ColorMask   (HiddGCAttrBase + aoHidd_GC_ColorMask)
#define aHidd_GC_LinePattern (HiddGCAttrBase + aoHidd_GC_LinePattern)
#define aHidd_GC_PlaneMask   (HiddGCAttrBase + aoHidd_GC_PlaneMask)
#define aHidd_GC_ColorExpansionMode  (HiddGCAttrBase + aoHidd_GC_ColorExpansionMode)


/* Drawmodes for a graphics context */


#define vHidd_GC_DrawMode_Clear 	0x00 /* 0 */
#define vHidd_GC_DrawMode_And 		0x01 /* src AND dst	*/
#define vHidd_GC_DrawMode_Copy 		0x03 /* src		*/
#define vHidd_GC_DrawMode_Xor  		0x06 /* src XOR dst	*/
#define vHidd_GC_DrawMode_Invert  	0x0A /* NOT dst		*/


#define vHidd_GC_ColExp_Transparent	(1 << 0)
#define vHidd_GC_ColExp_Opaque		(1 << 1)

/* Predeclarations of stubs in libhiddgraphicsstubs.h */

Object * HIDD_Gfx_NewGC        (Object *hiddGfx, struct TagItem *tagList);
VOID     HIDD_Gfx_DisposeGC    (Object *hiddGfx, Object *gc);
Object * HIDD_Gfx_NewBitMap    (Object *hiddGfx, struct TagItem *tagList);
VOID     HIDD_Gfx_DisposeBitMap(Object *hiddGfx, Object *bitMap);


BOOL	 HIDD_Gfx_RegisterGfxModes(Object *hiddGfx, struct TagItem **modeTags);

struct List *HIDD_Gfx_QueryGfxModes(Object *hiddGfx, struct TagItem *queryTags);
VOID HIDD_Gfx_ReleaseGfxModes(Object *hiddGfx, struct List *modeList);


VOID HIDD_GC_SetClipRect(Object *gc, LONG x1, LONG y1, LONG x2, LONG y2);
VOID HIDD_GC_UnsetClipRect(Object *gc);

VOID     HIDD_BM_BltBitMap   (Object obj, Object dest, WORD srcX, WORD srcY, WORD destX, WORD destY, WORD width, WORD height);
BOOL     HIDD_BM_Show        (Object obj);
VOID     HIDD_BM_Move        (Object obj, WORD x, WORD y);
BOOL     HIDD_BM_DepthArrange(Object obj, Object bm);
BOOL	 HIDD_BM_SetColors	(Object *obj, HIDDT_Color *tab, ULONG firstcolor, ULONG numcolors);

ULONG    HIDD_BM_PutPixel(Object *obj, WORD x, WORD y, HIDDT_Pixel pixel);
HIDDT_Pixel    HIDD_BM_GetPixel       (Object *obj, WORD x, WORD y);
ULONG    HIDD_BM_DrawPixel       (Object *obj, Object *gc, WORD x, WORD y);
VOID     HIDD_BM_CopyBox         (Object *obj, Object *gc, WORD srcX, WORD srcY, Object *dest, WORD destX, WORD destY, UWORD width, UWORD height);
VOID     HIDD_BM_GetImage	 (Object *obj, UBYTE *pixelArray, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_StdPixFmt pixFmt);
VOID	 HIDD_BM_PutImage 	 (Object *obj, Object *gc, UBYTE *pixelArray, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_StdPixFmt pixFmt);
VOID     HIDD_BM_DrawLine        (Object *obj, Object *gc, WORD x1, WORD y1, WORD x2, WORD y2);
VOID     HIDD_BM_DrawRect        (Object *obj, Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY);
VOID     HIDD_BM_FillRect        (Object *obj, Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY);
VOID     HIDD_BM_DrawEllipse     (Object *obj, Object *gc, WORD x, WORD y, WORD ry, WORD rx);
VOID     HIDD_BM_FillEllipse     (Object *obj, Object *gc, WORD x, WORD y, WORD ry, WORD rx);
VOID     HIDD_BM_DrawArc         (Object *obj, Object *gc);
VOID     HIDD_BM_FillArc         (Object *obj, Object *gc);
VOID     HIDD_BM_DrawPolygon     (Object *obj, Object *gc, UWORD n, WORD *coords);
VOID     HIDD_BM_FillPolygon     (Object *obj, Object *gc, UWORD n, WORD *coords);
VOID     HIDD_BM_DrawText        (Object *obj, Object *gc, WORD x, WORD y, STRPTR text, UWORD length);
VOID     HIDD_BM_FillText        (Object *obj, Object *gc, WORD x, WORD y, STRPTR text, UWORD length);
VOID     HIDD_BM_FillSpan        (Object *obj);
VOID     HIDD_BM_Clear           (Object *obj, Object *gc);
VOID	 HIDD_BM_BlitColorExpansion	 (Object *destObj, Object *gc, Object *srcObj, WORD srcX, WORD srcY, WORD destX, WORD destY,  UWORD width, UWORD height);

HIDDT_Pixel HIDD_BM_MapColor (Object *destObj, HIDDT_Color *color);
VOID	 HIDD_BM_UnmapPixel(Object *destObj, HIDDT_Pixel pixel, HIDDT_Color *color);

VOID	 HIDD_BM_PutImageLUT 	 (Object *obj, Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut);
VOID	 HIDD_BM_GetImageLUT 	 (Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut);

ULONG HIDD_BM_BytesPerLine(Object *obj, HIDDT_StdPixFmt pixFmt, ULONG width);
Object *HIDD_BM_GetPixelFormat(Object *obj, HIDDT_StdPixFmt pixFmt);

VOID     HIDD_BM_ConvertPixels  (Object *obj
	, APTR *srcPixels
	, HIDDT_PixelFormat *srcPixFmt
	, ULONG srcMod
	, APTR *dstBuf
	, HIDDT_PixelFormat *dstPixFmt
	, ULONG dstMod
	, ULONG width, ULONG height
	, HIDDT_PixelLUT *pixlut
);

Object * HIDD_BM_SetPixelFormat(Object *o, struct TagItem *pixFmtTags);
Object * HIDD_BM_SetColorMap(Object *o, Object *colorMap);

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
	Object *pixfmt;
};


#define BM_PIXFMT(bm) (HIDDT_PixelFormat *)(((struct _hidd_bitmap_protected *)bm)->pixfmt)
#define BM_DEPTH(bm)	BM_PIXFMT(bm)->depth


/****** !!!! PROTECTED DATA !!!!
	This typedef can be used to acces a GC object directly from within
	a bitmap class, but NEVER EVER use it to access a GC object
	from outside a bitmap class. And when accessing from inside a
	bitmap class, use the macros below !
*/

typedef struct {

    HIDDT_Pixel fg;        /* foreground color                                 */
    HIDDT_Pixel bg;        /* background color                                 */
    HIDDT_DrawMode drMode;    /* drawmode                                         */
    /* WARNING: type of font could be change */
    APTR  font;      /* current fonts                                    */
    ULONG colMask;   /* ColorMask prevents some color bits from changing */
    UWORD linePat;   /* LinePattern                                      */
    APTR  planeMask; /* Pointer to a shape bitMap                        */
    ULONG colExp;
    
    BOOL  doClip;

    LONG  clipX1;
    LONG  clipY1;
    LONG  clipX2;
    LONG  clipY2;

    
} HIDDT_GC_Intern;

#define GCINT(gc)	((HIDDT_GC_Intern *)gc)
#define GC_FG(gc)	(GCINT(gc)->fg)
#define GC_BG(gc)	(GCINT(gc)->bg)
#define GC_DRMD(gc)	(GCINT(gc)->drMode)
#define GC_FONT(gc)	(GCINT(gc)->font)
#define GC_COLMASK(gc)	(GCINT(gc)->colMask)
#define GC_LINEPAT(gc)	(GCINT(gc)->linePat)
#define GC_PLANEMASK(gc) (GCINT(gc)->planeMask)
#define GC_COLEXP(gc)	(GCINT(gc)->colExp)

#define GC_DOCLIP(gc)	(GCINT(gc)->doClip)
#define GC_CLIPX1(gc)	(GCINT(gc)->clipX1)
#define GC_CLIPY1(gc)	(GCINT(gc)->clipY1)
#define GC_CLIPX2(gc)	(GCINT(gc)->clipX2)
#define GC_CLIPY2(gc)	(GCINT(gc)->clipY2)

#define GC_(gc)	(GCINT(gc)->)

/******************** Gfx Mode definitions ********************/

enum {
     moHidd_GfxMode_LookupPixFmt = 0
};


struct pHidd_GfxMode_LookupPixFmt {
    MethodID mID;
    ULONG pixFmtNo;
};

Object *HIDD_GM_LookupPixFmt(Object *obj, ULONG pixFmtNo); /* Starts at 0 */

enum {
     aoHidd_GfxMode_Width = 0,
     aoHidd_GfxMode_Height,
     aoHidd_GfxMode_StdPixFmt,
     aoHidd_GfxMode_PixFmtTags,
     aoHidd_GfxMode_NumPixFmts,
     
     num_Hidd_GfxMode_Attrs
};

#define aHidd_GfxMode_Width		(HiddGfxModeAttrBase + aoHidd_GfxMode_Width	)
#define aHidd_GfxMode_Height		(HiddGfxModeAttrBase + aoHidd_GfxMode_Height	)
#define aHidd_GfxMode_StdPixFmt		(HiddGfxModeAttrBase + aoHidd_GfxMode_StdPixFmt	)
#define aHidd_GfxMode_PixFmtTags	(HiddGfxModeAttrBase + aoHidd_GfxMode_PixFmtTags	)
#define aHidd_GfxMode_NumPixFmts	(HiddGfxModeAttrBase + aoHidd_GfxMode_NumPixFmts	)



/****************** PixFmt definitions **************************/
enum {
    aoHidd_PixFmt_GraphType,
    aoHidd_PixFmt_RedShift,
    aoHidd_PixFmt_GreenShift,
    aoHidd_PixFmt_BlueShift,
    aoHidd_PixFmt_AlphaShift,
    aoHidd_PixFmt_RedMask,
    aoHidd_PixFmt_GreenMask,
    aoHidd_PixFmt_BlueMask,
    aoHidd_PixFmt_AlphaMask,
    aoHidd_PixFmt_Depth,
    aoHidd_PixFmt_BitsPerPixel,
    aoHidd_PixFmt_BytesPerPixel,
    aoHidd_PixFmt_StdPixFmt,
    aoHidd_PixFmt_CLUTMask,
    aoHidd_PixFmt_CLUTShift,
    
    num_Hidd_PixFmt_Attrs
};


#define aHidd_PixFmt_RedShift		(HiddPixFmtAttrBase + aoHidd_PixFmt_RedShift)
#define aHidd_PixFmt_GreenShift		(HiddPixFmtAttrBase + aoHidd_PixFmt_GreenShift)
#define aHidd_PixFmt_BlueShift		(HiddPixFmtAttrBase + aoHidd_PixFmt_BlueShift)
#define aHidd_PixFmt_AlphaShift		(HiddPixFmtAttrBase + aoHidd_PixFmt_AlphaShift)
#define aHidd_PixFmt_RedMask		(HiddPixFmtAttrBase + aoHidd_PixFmt_RedMask)
#define aHidd_PixFmt_GreenMask		(HiddPixFmtAttrBase + aoHidd_PixFmt_GreenMask)
#define aHidd_PixFmt_BlueMask		(HiddPixFmtAttrBase + aoHidd_PixFmt_BlueMask)
#define aHidd_PixFmt_AlphaMask		(HiddPixFmtAttrBase + aoHidd_PixFmt_AlphaMask)
#define aHidd_PixFmt_Depth		(HiddPixFmtAttrBase + aoHidd_PixFmt_Depth)
#define aHidd_PixFmt_BytesPerPixel	(HiddPixFmtAttrBase + aoHidd_PixFmt_BytesPerPixel)
#define aHidd_PixFmt_BitsPerPixel	(HiddPixFmtAttrBase + aoHidd_PixFmt_BitsPerPixel)
#define aHidd_PixFmt_GraphType		(HiddPixFmtAttrBase + aoHidd_PixFmt_GraphType)
#define aHidd_PixFmt_StdPixFmt		(HiddPixFmtAttrBase + aoHidd_PixFmt_StdPixFmt)
#define aHidd_PixFmt_CLUTShift		(HiddPixFmtAttrBase + aoHidd_PixFmt_CLUTShift)
#define aHidd_PixFmt_CLUTMask		(HiddPixFmtAttrBase + aoHidd_PixFmt_CLUTMask)


#define IS_PIXFMT_ATTR(attr, idx) \
	( ( ( idx ) = (attr) - HiddPixFmtAttrBase) < num_Hidd_PixFmt_Attrs)



/********** Planar bitmap *******************/

#define CLID_Hidd_PlanarBM "hidd.graphics.bitmap.planarbm"
#define IID_Hidd_PlanarBM  "hidd.graphics.bitmap.planarbm"

#define HiddPlanarBMAttrBase __IHIDD_PlanarBM

extern AttrBase HiddPlanarBMAttrBase;

enum {

    moHidd_PlanarBM_SetBitMap	/* AROS sepecific method */
};

struct pHidd_PlanarBM_SetBitMap
{
    MethodID mID;
    struct BitMap *bitMap;
};

VOID HIDD_PlanarBM_SetBitMap(Object *obj, struct BitMap *bitMap);

enum {
    aoHidd_PlanarBM_AllocPlanes,	/* [I..] BOOL */
    
    num_Hidd_PlanarBM_Attrs
};

#define aHidd_PlanarBM_AllocPlanes	(HiddPlanarBMAttrBase + aoHidd_PlanarBM_AllocPlanes)
#define aHidd_PlanarBM_		(HiddPlanarBMAttrBase + aoHidd_PlanarBM_)



#define IS_PLANARBM_ATTR(attr, idx) \
	( ( ( idx ) = (attr) - HiddPlanarBMAttrBase) < num_Hidd_PlanarBM_Attrs)
    


/********** ColorMap *******************/

#define CLID_Hidd_ColorMap "hidd.graphics.colormap"
#define IID_Hidd_ColorMap  "hidd.graphics.colormap"

#define HiddColorMapAttrBase __IHIDD_ColorMap

extern AttrBase HiddColorMapAttrBase;

/* Methods */
enum {
    moHidd_ColorMap_SetColors,
    moHidd_ColorMap_GetPixel
};

struct pHidd_ColorMap_SetColors {
    MethodID	mID;
    HIDDT_Color	*colors;
    ULONG	firstColor;
    ULONG	numColors;
    Object	*pixFmt;
};

struct pHidd_ColorMap_GetPixel {
    MethodID mID;
    ULONG pixelNo;
};

BOOL HIDD_CM_SetColors(Object *obj, HIDDT_Color *colors, ULONG firstColor, ULONG numColors, Object *pixFmt);
HIDDT_Pixel HIDD_CM_GetPixel(Object *obj, ULONG pixelNo);

/* Attrs */
enum {
    aoHidd_ColorMap_NumEntries,	/* [I.G] ULONG */
    
    num_Hidd_ColorMap_Attrs
};

#define aHidd_ColorMap_NumEntries	(HiddColorMapAttrBase + aoHidd_ColorMap_NumEntries)
#define aHidd_ColorMap_		(HiddColorMapAttrBase + aoHidd_ColorMap_)



#define IS_COLORMAP_ATTR(attr, idx) \
	( ( ( idx ) = (attr) - HiddColorMapAttrBase) < num_Hidd_ColorMap_Attrs)

#endif /* HIDD_GRAPHICS_H */


