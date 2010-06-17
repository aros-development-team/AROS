#ifndef HIDD_GRAPHICS_H
#define HIDD_GRAPHICS_H

/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
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


#define IID_Hidd_Gfx        	"hidd.graphics.graphics"
#define IID_Hidd_BitMap     	"hidd.graphics.bitmap"
#define IID_Hidd_GC         	"hidd.graphics.gc"

#define IID_Hidd_PixFmt	    	"hidd.graphics.pixfmt"

/* Some "example" hidd bitmaps */

#define CLID_Hidd_ChunkyBM "hidd.graphics.bitmap.chunkybm"


typedef OOP_Object *HIDDT_BitMap;
typedef OOP_Object *HIDDT_GC;


/* Attrbases */
#define HiddGCAttrBase          __IHidd_GC
#define HiddGfxAttrBase         __IHidd_Gfx
#define HiddBitMapAttrBase      __IHidd_BitMap

#define HiddPixFmtAttrBase	__IHidd_PixFmt

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddGCAttrBase;
extern OOP_AttrBase HiddGfxAttrBase;
extern OOP_AttrBase HiddBitMapAttrBase;
extern OOP_AttrBase HiddPixFmtAttrBase;
#endif

/**** Graphics definitions ****************************************************/

enum
{
    /* Methods for a graphics hidd */

    moHidd_Gfx_NewGC = 0,      
    moHidd_Gfx_DisposeGC,      
    moHidd_Gfx_NewBitMap,      
    moHidd_Gfx_DisposeBitMap,  
			
			
    
    moHidd_Gfx_QueryModeIDs,
    moHidd_Gfx_ReleaseModeIDs,
    moHidd_Gfx_NextModeID,
    moHidd_Gfx_GetMode,
    
    moHidd_Gfx_CheckMode,
    
    moHidd_Gfx_GetPixFmt,
    
    moHidd_Gfx_SetCursorShape,
    moHidd_Gfx_SetCursorPos,
    moHidd_Gfx_SetCursorVisible,
    
    moHidd_Gfx_SetMode,
    moHidd_Gfx_Show,
    moHidd_Gfx_CopyBox,
    
    moHidd_Gfx_ShowImminentReset,
    
    moHidd_Gfx_ModeProperties,
    moHidd_Gfx_ShowViewPorts,

    moHidd_Gfx_GetSync,

    moHidd_Gfx_GetGamma,
    moHidd_Gfx_SetGamma,

    num_Hidd_Gfx_Methods
};

enum
{
    aoHidd_Gfx_IsWindowed,	    	/* [..G] (BOOL) - Whether the HIDD is using a window 
						  system to render its gfx */
#if 0
    aoHidd_Gfx_ActiveBMCallBack,
    aoHidd_Gfx_ActiveBMCallBackData,
#endif    
    aoHidd_Gfx_DPMSLevel,		/* [ISG] (ULONG) - DPMS level	*/
    
    /* Used in gfxmode registering */
    aoHidd_Gfx_PixFmtTags,		/* [I..] (struct TagItem)	*/
    aoHidd_Gfx_SyncTags,		/* [I..] (struct TagItem)	*/
    aoHidd_Gfx_ModeTags,		/* [I..] (struct TagItem)	*/
    
    aoHidd_Gfx_NumSyncs,		/* [..G] (ULONG) - The number of different syncs the gfxcard can do */
    aoHidd_Gfx_SupportsHWCursor,	/* [..G] (BOOL) - if the hidd supports hardware cursors */
    aoHidd_Gfx_NoFrameBuffer,		/* [..G] (BOOL) - if the hidd does not need a framebuffer */
    
    num_Hidd_Gfx_Attrs
};

#define aHidd_Gfx_IsWindowed 		(HiddGfxAttrBase + aoHidd_Gfx_IsWindowed		)

#if 0
#define aHidd_Gfx_ActiveBMCallBack	(HiddGfxAttrBase + aoHidd_Gfx_ActiveBMCallBack		)
#define aHidd_Gfx_ActiveBMCallBackData	(HiddGfxAttrBase + aoHidd_Gfx_ActiveBMCallBackData	)
#endif

#define aHidd_Gfx_DPMSLevel		(HiddGfxAttrBase + aoHidd_Gfx_DPMSLevel			)
#define aHidd_Gfx_PixFmtTags		(HiddGfxAttrBase + aoHidd_Gfx_PixFmtTags		)
#define aHidd_Gfx_SyncTags		(HiddGfxAttrBase + aoHidd_Gfx_SyncTags			)
#define aHidd_Gfx_ModeTags		(HiddGfxAttrBase + aoHidd_Gfx_ModeTags			)
#define aHidd_Gfx_NumSyncs		(HiddGfxAttrBase + aoHidd_Gfx_NumSyncs			)
#define aHidd_Gfx_SupportsHWCursor	(HiddGfxAttrBase + aoHidd_Gfx_SupportsHWCursor		)
#define aHidd_Gfx_NoFrameBuffer		(HiddGfxAttrBase + aoHidd_Gfx_NoFrameBuffer		)

#define IS_GFX_ATTR(attr, idx)	\
	( ( ( idx ) = (attr) - HiddGfxAttrBase) < num_Hidd_Gfx_Attrs)




/* Parameter tags for the QueryModeIDs method */
enum
{
    tHidd_GfxMode_MinWidth = TAG_USER,
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
typedef IPTR HIDDT_DrawMode;
typedef IPTR HIDDT_ColorModel;
typedef IPTR HIDDT_BitMapType;
typedef IPTR HIDDT_ModeID;

struct HIDD_ModeProperties
{
    ULONG DisplayInfoFlags; /* PropertyFlags value for struct DisplayInfo (see graphics/displayinfo.h).
			       Does not include features emulated by software.				 */
    UWORD NumHWSprites;	    /* Number of supported hardware sprites					 */
    UWORD CompositionFlags; /* Supported composition types, see below					 */

    /* This structure may grow in future */

};

#define COMPF_ABOVE 0x0001 /* We can see another screen above this screen */
#define COMPF_BELOW 0x0002 /* ...below ...				  */
#define COMPF_LEFT  0x0004 /* ... to the left of ...			  */
#define COMPF_RIGHT 0x0008 /* ... to the right of ...			  */

struct HIDD_ViewPortData
{
    struct HIDD_ViewPortData *Next;
    OOP_Object *Bitmap;
};

#define vHidd_ModeID_Invalid ((HIDDT_ModeID)-1)



/* messages for a graphics hidd */

struct pHidd_Gfx_NewGC
{
    OOP_MethodID        mID;
    struct TagItem  	*attrList;
};

struct pHidd_Gfx_DisposeGC
{
    OOP_MethodID    mID;
    OOP_Object      *gc;
};

struct pHidd_Gfx_NewBitMap
{
    OOP_MethodID    mID;
    
    struct TagItem  *attrList;
};

struct pHidd_Gfx_DisposeBitMap
{
    OOP_MethodID    mID;
    OOP_Object      *bitMap;
};


/*
     The four next method calls are used for
     getting information from the gfx hidd mode
     database
*/
     

struct pHidd_Gfx_QueryModeIDs
{
    OOP_MethodID    mID;
    struct TagItem  *queryTags;
};

struct pHidd_Gfx_ReleaseModeIDs
{
    OOP_MethodID    mID;
    HIDDT_ModeID    *modeIDs;
};


struct pHidd_Gfx_GetMode
{
    OOP_MethodID    mID;
    HIDDT_ModeID    modeID;
    OOP_Object      **syncPtr;
    OOP_Object      **pixFmtPtr;
};

struct pHidd_Gfx_NextModeID
{
    OOP_MethodID    mID;
    HIDDT_ModeID    modeID;
    OOP_Object      **syncPtr;
    OOP_Object      **pixFmtPtr;
};

/*
    The two below are used internally in the HIDD. Do *NOT* use
    these from outsode the HIDD
*/
struct pHidd_Gfx_CheckMode
{
    OOP_MethodID    mID;
    HIDDT_ModeID    modeID;
    OOP_Object      *sync;
    OOP_Object      *pixFmt;
};

struct pHidd_Gfx_GetPixFmt
{
    OOP_MethodID    mID;
    HIDDT_StdPixFmt stdPixFmt;
};

struct pHidd_Gfx_SetCursorShape
{
    OOP_MethodID    mID;
    OOP_Object      *shape;
    LONG	    xoffset;
    LONG	    yoffset;
};

struct pHidd_Gfx_SetCursorPos
{
    OOP_MethodID    mID;
    LONG	    x;
    LONG	    y;
};

struct pHidd_Gfx_SetCursorVisible
{
    OOP_MethodID    mID;
    BOOL	    visible;
};

struct pHidd_Gfx_Show
{
    OOP_MethodID    mID;
    OOP_Object      *bitMap;
    ULONG   	    flags;
};

struct pHidd_Gfx_CopyBox
{
    OOP_MethodID    mID;
    OOP_Object	    *src;
    OOP_Object	    *gc;
    WORD            srcX, srcY;
    OOP_Object      *dest;
    WORD            destX, destY;
    UWORD           width, height;
};

/* Flags */

/* This will make the gfx hidd, copy back from the framebuffer into
   the old bitmap you were using. Use this option with extreme
   prejudice:  YOU MUST BE TOTALLY SURE THAT THE BITMAP
   YOU CALLED SHOW ON BEFORE THIS CALL HAS NOT BEEN DISPOSED
*/
#define fHidd_Gfx_Show_CopyBack 0x01


struct pHidd_Gfx_SetMode
{
    OOP_MethodID mID;
    OOP_Object   *Sync;
};


struct pHidd_Gfx_ModeProperties
{
    OOP_MethodID mID;
    HIDDT_ModeID modeID;
    struct HIDD_ModeProperties *props;
    ULONG propsLen;
};

struct pHidd_Gfx_ShowViewPorts
{
    OOP_MethodID mID;
    struct HIDD_ViewPortData *Data;
};

struct pHidd_Gfx_GetSync
{
    OOP_MethodID mID;
    ULONG	 num;
};

struct pHidd_Gfx_Gamma
{
    OOP_MethodID mID;
    UBYTE	 *Red;
    UBYTE	 *Green;
    UBYTE	 *Blue;
};

enum
{
    tHidd_Cursor_BitMap,	/* OOP_Object *, cursor shape bitmap */
    tHidd_Cursor_XPos,		/* ULONG, cursor x position	*/
    tHidd_Cursor_YPos,		/* ULONG, cursor Y position */
    tHidd_Cursor_On		/* BOOL, cursor on, TRUE, FALSE. */
};

/**** BitMap definitions ******************************************************/


	/* Types */


typedef UWORD HIDDT_ColComp;	/* Color component */
typedef ULONG HIDDT_Pixel;

	
typedef struct
{
   HIDDT_ColComp	red;
   HIDDT_ColComp	green;
   HIDDT_ColComp	blue;
   HIDDT_ColComp	alpha;
   
   HIDDT_Pixel		pixval;
   
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
    /* Pseudo formats. These are not real pixelfromats but are passed
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

#define FIRST_RGB_STDPIXFMT 	    	vHidd_StdPixFmt_RGB24
#define LAST_RGB_STDPIXFMT  	    	vHidd_StdPixFmt_0BGR32
#define NUM_RGB_STDPIXFMT   	    	(vHidd_StdPixFmt_0BGR32 - vHidd_StdPixFmt_RGB24 + 1)

#define num_Hidd_StdPixFmt  	    	(num_Hidd_AllPf - num_Hidd_PseudoStdPixFmt)

#define IS_PSEUDO_STDPIXFMT(stdpf)	( (stdpf) > 0 && (stdpf) < num_Hidd_PseudoStdPixFmt )
    
#define IS_REAL_STDPIXFMT(stdpf)	( (stdpf) >= num_Hidd_PseudoStdPixFmt && (stdpf) < num_Hidd_AllPf)
#define IS_STDPIXFMT(stdpf)	    	( (stdpf) > 0 && (stdpf) < num_Hidd_AllPf )

#define REAL_STDPIXFMT_IDX(stdpf)   	( (stdpf) - num_Hidd_PseudoStdPixFmt )




#define MAP_SHIFT	    	    	((sizeof (HIDDT_Pixel) - sizeof (HIDDT_ColComp)) *8)
#define SHIFT_UP_COL(col)   	    	((HIDDT_Pixel)((col) << MAP_SHIFT))

#define MAP_COLCOMP(comp, val, pixfmt)	\
	((SHIFT_UP_COL(val) >> (pixfmt)->comp ## _shift) & (pixfmt)->comp ## _mask)
	

#define MAP_RGB(r, g, b, pixfmt) 	\
	  MAP_COLCOMP(red,   r, pixfmt) \
	| MAP_COLCOMP(green, g, pixfmt)	\
	| MAP_COLCOMP(blue,  b, pixfmt) 


#define MAP_RGBA(r, g, b, a, pixfmt) 	\
	  MAP_COLCOMP(red,   r, pixfmt) 	\
	| MAP_COLCOMP(green, g, pixfmt)	\
	| MAP_COLCOMP(blue,  b, pixfmt)	\
	| MAP_COLCOMP(alpha,  a, pixfmt)
	
	

#define SHIFT_DOWN_PIX(pix) ((pix) >> MAP_SHIFT)
#define GET_COLCOMP(comp, pix, pixfmt) \
	SHIFT_DOWN_PIX( ( (pix) & (pixfmt)-> comp ## _mask) << pixfmt-> comp ## _shift )

#define RED_COMP(pix, pixfmt)	GET_COLCOMP(red,   pix, pixfmt)
#define GREEN_COMP(pix, pixfmt)	GET_COLCOMP(green, pix, pixfmt)
#define BLUE_COMP(pix, pixfmt)	GET_COLCOMP(blue,  pix, pixfmt)
#define ALPHA_COMP(pix, pixfmt)	GET_COLCOMP(alpha,  pix, pixfmt)

typedef struct
{
    UWORD	    depth;
    UWORD	    size;		/* Size of pixel in bits */
    UWORD   	    bytes_per_pixel;

    HIDDT_Pixel     red_mask;
    HIDDT_Pixel     green_mask;
    HIDDT_Pixel     blue_mask;
    HIDDT_Pixel     alpha_mask;

    UBYTE   	    red_shift;
    UBYTE   	    green_shift;
    UBYTE   	    blue_shift;
    UBYTE   	    alpha_shift;

    HIDDT_Pixel     clut_mask;
    UBYTE   	    clut_shift;

    HIDDT_StdPixFmt stdpixfmt;		/* Number of corresponging standard format */
    ULONG   	    flags;		/* Flags, see below */
	
} HIDDT_PixelFormat;



enum
{
    /* Methods for a bitmap */

    moHidd_BitMap_SetColors = 0,
    moHidd_BitMap_PutPixel,
    moHidd_BitMap_DrawPixel,
    moHidd_BitMap_PutImage,
    moHidd_BitMap_PutAlphaImage,
    moHidd_BitMap_PutTemplate,
    moHidd_BitMap_PutAlphaTemplate,
    moHidd_BitMap_PutPattern,
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
    moHidd_BitMap_PutTranspImageLUT,
    moHidd_BitMap_GetImageLUT,
    moHidd_BitMap_BytesPerLine,
    moHidd_BitMap_ConvertPixels,
    moHidd_BitMap_FillMemRect8,
    moHidd_BitMap_FillMemRect16,
    moHidd_BitMap_FillMemRect24,
    moHidd_BitMap_FillMemRect32,
    moHidd_BitMap_InvertMemRect,
    moHidd_BitMap_CopyMemBox8,
    moHidd_BitMap_CopyMemBox16,
    moHidd_BitMap_CopyMemBox24,
    moHidd_BitMap_CopyMemBox32,
    moHidd_BitMap_CopyLUTMemBox16,
    moHidd_BitMap_CopyLUTMemBox24,
    moHidd_BitMap_CopyLUTMemBox32,
    moHidd_BitMap_PutMem32Image8,
    moHidd_BitMap_PutMem32Image16,
    moHidd_BitMap_PutMem32Image24,
    moHidd_BitMap_GetMem32Image8,
    moHidd_BitMap_GetMem32Image16,
    moHidd_BitMap_GetMem32Image24,
    moHidd_BitMap_PutMemTemplate8,
    moHidd_BitMap_PutMemTemplate16,
    moHidd_BitMap_PutMemTemplate24,
    moHidd_BitMap_PutMemTemplate32,
    moHidd_BitMap_PutMemPattern8,
    moHidd_BitMap_PutMemPattern16,
    moHidd_BitMap_PutMemPattern24,
    moHidd_BitMap_PutMemPattern32,
    
    /* This method is used only by subclasses, I repeat:
    ONLY BY SUBCLASSES, to register available modes in the baseclass
    */
    moHidd_BitMap_SetColorMap,
    moHidd_BitMap_ObtainDirectAccess,
    moHidd_BitMap_ReleaseDirectAccess,
    
    moHidd_BitMap_BitMapScale, 
    
    moHidd_BitMap_PrivateSet,
    moHidd_BitMap_SetRGBConversionFunction,

    moHidd_BitMap_UpdateRect,
    
    num_Hidd_BitMap_Methods
};

enum
{
    /* Attributes for a bitmap */
    aoHidd_BitMap_Width,         /*  0 [ISG] Bitmap with                          */
    aoHidd_BitMap_Height,        /*  1 [ISG] Bitmap height                        */
#if 0
    aoHidd_BitMap_Depth,         /*    [I.G] Bitmap depth                         */
#endif
    aoHidd_BitMap_Displayable,   /*  2 [I.G] BOOL bitmap is displayable (default: FALSE)  */
    aoHidd_BitMap_Visible,       /*  3 [..G] Check if a bitmap is visible         */
    aoHidd_BitMap_IsLinearMem,   /*  4 [..G] Is the bitmap memory contigous       */
    aoHidd_BitMap_BytesPerRow,   /*  5 [..G] Number of bytes in a row             */
    aoHidd_BitMap_ColorMap,      /*  6 [..G] Colormap of the bitmap               */
    aoHidd_BitMap_Friend,	 /*  7 [I.G] Friend bitmap. The bitmap will be allocated so that it
    				            is optimized for blitting to this bitmap */
    aoHidd_BitMap_GfxHidd,	 /*  8 [..G] Pointer to the gfxhidd object this bitmap was created with */
    aoHidd_BitMap_StdPixFmt,	 /*  9 [I..] (HIDDT_StdPixFmt) What stdpixel format the bitmap should have.
				             This is a shortcut to create a bitmap with a std pixelformat */
    aoHidd_BitMap_PixFmt,	 /* 10 [..G] (OOP_Object *) This is complete pixmft of a bitmap */
    aoHidd_BitMap_ModeID,	 /* 11 [I.G] (HIDDT_ModeID) must be passed on initialization of
				             aHidd_BitMap_Displayable=TRUE bitmaps. May also be
				             used with non-displayable bitmaps */
    aoHidd_BitMap_ClassPtr,	 /* 12 [I.G] Only used by subclasses of the gfx hidd */
    aoHidd_BitMap_ClassID,	 /* 13 [I..] Only used by subclasses of the gfx hidd */
    aoHidd_BitMap_PixFmtTags,	 /* 14 [I..] Only used by subclasses of BitMap class */
    
#if 0    
    aoHidd_BitMap_Mode,          /*    [ISG] The display mode of this bitmap      */
    aoHidd_BitMap_AllocBuffer,   /*    [I..] BOOL allocate buffer (default: TRUE) */
    
    aoHidd_BitMap_BestSize,      /*    [..G] Best size for depth                  */
#endif
    aoHidd_BitMap_FrameBuffer,	 /* 15 [I.G] BOOL - Allocate framebuffer	  */

    aoHidd_BitMap_LeftEdge,      /* 16 [.SG] Left edge position of the bitmap     */
    aoHidd_BitMap_TopEdge,       /* 17 [.SG] Top edge position of the bitmap      */
    aoHidd_BitMap_Align,         /* 18 [I..] Number of pixels to align bitmap data width to */

    num_Hidd_BitMap_Attrs
};    

#define aHidd_BitMap_Width         (HiddBitMapAttrBase + aoHidd_BitMap_Width)
#define aHidd_BitMap_Height        (HiddBitMapAttrBase + aoHidd_BitMap_Height)
#if 0
#define aHidd_BitMap_Depth         (HiddBitMapAttrBase + aoHidd_BitMap_Depth)
#endif
#define aHidd_BitMap_Displayable   (HiddBitMapAttrBase + aoHidd_BitMap_Displayable)
#define aHidd_BitMap_Visible       (HiddBitMapAttrBase + aoHidd_BitMap_Visible)
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
#define aHidd_BitMap_PixFmt	   (HiddBitMapAttrBase + aoHidd_BitMap_PixFmt)
#define aHidd_BitMap_ModeID	   (HiddBitMapAttrBase + aoHidd_BitMap_ModeID)
#define aHidd_BitMap_ClassPtr	   (HiddBitMapAttrBase + aoHidd_BitMap_ClassPtr)
#define aHidd_BitMap_ClassID	   (HiddBitMapAttrBase + aoHidd_BitMap_ClassID)
#define aHidd_BitMap_PixFmtTags	   (HiddBitMapAttrBase + aoHidd_BitMap_PixFmtTags)
#define aHidd_BitMap_FrameBuffer   (HiddBitMapAttrBase + aoHidd_BitMap_FrameBuffer)
#define aHidd_BitMap_Align	   (HiddBitMapAttrBase + aoHidd_BitMap_Align)

#define IS_BITMAP_ATTR(attr, idx) \
	( ( ( idx ) = (attr) - HiddBitMapAttrBase) < num_Hidd_BitMap_Attrs)

/* messages for a bitmap */

struct pHidd_BitMap_PrivateSet
{
    OOP_MethodID        mID;
    struct TagItem  	*attrList;
};

struct pHidd_BitMap_SetColors
{
    OOP_MethodID    mID;
    HIDDT_Color	    *colors;
    ULONG	    firstColor;
    ULONG	    numColors;
};

/* messages for a graphics context */

struct pHidd_BitMap_PutPixel
{
    OOP_MethodID    mID;
    WORD    	    x, y;
    HIDDT_Pixel     pixel;
};

struct pHidd_BitMap_GetPixel
{
    OOP_MethodID    mID;
    WORD    	    x, y;
};

struct pHidd_BitMap_DrawPixel
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    WORD    	    x, y;
};

struct pHidd_BitMap_DrawLine
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    WORD            x1 ,y1, x2, y2;
};


struct pHidd_BitMap_GetImage
{
    OOP_MethodID    mID;
    UBYTE	    *pixels;
    ULONG	    modulo;
    WORD	    x, y;
    WORD	    width, height;
    HIDDT_StdPixFmt pixFmt;
};

struct pHidd_BitMap_PutImage
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pixels;
    ULONG	    modulo;
    WORD	    x, y;
    WORD	    width, height;
    HIDDT_StdPixFmt pixFmt;
};

struct pHidd_BitMap_PutAlphaImage
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pixels;
    ULONG	    modulo;
    WORD	    x, y;
    WORD	    width, height;
};

struct pHidd_BitMap_PutTemplate
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *Template;
    ULONG	    modulo;
    WORD    	    srcx;
    WORD	    x, y;
    WORD	    width, height;
    BOOL    	    inverttemplate;
};

/* Compatibility hack. In C++ template is a reserved keyword, so we
   can't use it as variable name */
#ifndef __cplusplus
#define template Template
#endif

struct pHidd_BitMap_PutAlphaTemplate
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *alpha;
    ULONG	    modulo;
    WORD	    x, y;
    WORD	    width, height;
    BOOL    	    invertalpha;
};

struct pHidd_BitMap_PutPattern
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pattern;
    WORD    	    patternsrcx;
    WORD    	    patternsrcy;
    WORD    	    patternheight;
    WORD    	    patterndepth;
    HIDDT_PixelLUT *patternlut;
    BOOL    	    invertpattern;
    UBYTE   	    *mask;    
    ULONG	    maskmodulo;
    WORD    	    masksrcx;
    WORD	    x, y;
    WORD	    width, height;
};


struct pHidd_BitMap_DrawRect
{
    OOP_MethodID    mID;
    OOP_Object      *gc;
    WORD            minX, minY, maxX, maxY;
};

struct pHidd_BitMap_DrawEllipse
{
    OOP_MethodID    mID;
    OOP_Object      *gc;
    WORD            x, y;
    UWORD           rx, ry;
};

struct pHidd_BitMap_DrawPolygon
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    WORD            n;         /* number of coordinates */
    WORD            *coords;   /* size 2*n              */
};

struct pHidd_BitMap_DrawText
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    WORD            x, y;      /* Start position, see autodocs */
    STRPTR          text;      /* Latin 1 string               */
    UWORD           length;    /* Number of characters to draw */
};

struct pHidd_BitMap_Clear
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
};

struct pHidd_BitMap_BlitColorExpansion
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    OOP_Object	    *srcBitMap;
    WORD	    srcX;
    WORD	    srcY;
    WORD	    destX;
    WORD	    destY;
    UWORD	    width;
    UWORD	    height;
};

struct pHidd_BitMap_MapColor
{
    OOP_MethodID    mID;
    HIDDT_Color     *color;
};

struct pHidd_BitMap_UnmapPixel
{
    OOP_MethodID    mID;
    HIDDT_Pixel     pixel;
    HIDDT_Color     *color;
};

struct pHidd_BitMap_PutImageLUT
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pixels;
    ULONG	    modulo;
    WORD	    x, y;
    WORD	    width, height;
    HIDDT_PixelLUT  *pixlut;
};

struct pHidd_BitMap_PutTranspImageLUT
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pixels;
    ULONG	    modulo;
    WORD	    x, y;
    WORD	    width, height;
    HIDDT_PixelLUT  *pixlut;
    UWORD   	    transparent;
};

struct pHidd_BitMap_GetImageLUT
{
    OOP_MethodID    mID;
    UBYTE	    *pixels;
    ULONG	    modulo;
    WORD	    x, y;
    WORD	    width, height;
    HIDDT_PixelLUT  *pixlut;
};



struct pHidd_BitMap_BytesPerLine
{
    OOP_MethodID    mID;
    HIDDT_StdPixFmt pixFmt;
    ULONG   	    width;
};


struct pHidd_BitMap_ConvertPixels
{
    OOP_MethodID    	mID;
    APTR    	    	*srcPixels;
    HIDDT_PixelFormat 	*srcPixFmt;
    
    ULONG   	    	srcMod;	/* Source modulo */
    
    APTR    	    	*dstBuf;
    HIDDT_PixelFormat 	*dstPixFmt;
    
    ULONG   	    	dstMod;
    
    ULONG   	    	width;
    ULONG   	    	height;

    HIDDT_PixelLUT  	*pixlut;
    
};

/* Fill rect area in 8 bit memory chunky buffer with pixel */

struct pHidd_BitMap_FillMemRect8
{
    OOP_MethodID mID;
    APTR    	 dstBuf;
    WORD    	 minX;
    WORD    	 minY;
    WORD    	 maxX;
    WORD    	 maxY;
    ULONG   	 dstMod;
    UWORD   	 fill; 
};

/* Fill rect area in 16 bit memory chunky buffer with pixel */

struct pHidd_BitMap_FillMemRect16
{
    OOP_MethodID mID;
    APTR   	 dstBuf;
    WORD    	 minX;
    WORD    	 minY;
    WORD    	 maxX;
    WORD    	 maxY;
    ULONG   	 dstMod;
    UWORD   	 fill; 
};

/* Fill rect area in 24 bit memory chunky buffer with pixel */

struct pHidd_BitMap_FillMemRect24
{
    OOP_MethodID mID;
    APTR   	 dstBuf;
    WORD    	 minX;
    WORD    	 minY;
    WORD    	 maxX;
    WORD    	 maxY;
    ULONG   	 dstMod;
    ULONG   	 fill; 
};

/* Fill rect area in 32 bit memory chunky buffer with pixel */

struct pHidd_BitMap_FillMemRect32
{
    OOP_MethodID mID;
    APTR   	 dstBuf;
    WORD    	 minX;
    WORD    	 minY;
    WORD    	 maxX;
    WORD    	 maxY;
    ULONG   	 dstMod;
    ULONG   	 fill; 
};

/* Invert rect area in 8 bit memory chunky buffer */

struct pHidd_BitMap_InvertMemRect
{
    OOP_MethodID mID;
    APTR    	 dstBuf;
    WORD    	 minX;
    WORD    	 minY;
    WORD    	 maxX;
    WORD    	 maxY;
    ULONG   	 dstMod;
};

/* copy src rect from 8 bit chunky memory buffer to dst rect in 8 bit chunky memory buffer */

struct pHidd_BitMap_CopyMemBox8
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD            srcX, srcY;
    APTR      	    dst;
    WORD            dstX, dstY;
    UWORD   	    width, height;
    ULONG   	    srcMod;
    ULONG           dstMod;
};

/* copy src rect from 16 bit chunky memory buffer to dst rect in 16 bit chunky memory buffer */

struct pHidd_BitMap_CopyMemBox16
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD            srcX, srcY;
    APTR      	    dst;
    WORD            dstX, dstY;
    UWORD   	    width, height;
    ULONG   	    srcMod;
    ULONG           dstMod;
};

/* copy src rect from 24 bit chunky memory buffer to dst rect in 24 bit chunky memory buffer */

struct pHidd_BitMap_CopyMemBox24
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD            srcX, srcY;
    APTR      	    dst;
    WORD            dstX, dstY;
    UWORD   	    width, height;
    ULONG   	    srcMod;
    ULONG           dstMod;
};

/* copy src rect from 32 bit chunky memory buffer to dst rect in 32 bit chunky memory buffer */

struct pHidd_BitMap_CopyMemBox32
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD            srcX, srcY;
    APTR      	    dst;
    WORD            dstX, dstY;
    UWORD   	    width, height;
    ULONG   	    srcMod;
    ULONG           dstMod;
};

/* copy src rect from 8 bit chunky memory buffer to
   dst rect in 16 bit chunky memory buffer using
   a HIDDT_PixelLUT lookup to convert from 8 --> 16*/

struct pHidd_BitMap_CopyLUTMemBox16
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD            srcX, srcY;
    APTR      	    dst;
    WORD            dstX, dstY;
    UWORD   	    width, height;
    ULONG   	    srcMod;
    ULONG           dstMod;
    HIDDT_PixelLUT  *pixlut;    
};

/* copy src rect from 8 bit chunky memory buffer to
   dst rect in 24 bit chunky memory buffer using
   a HIDDT_PixelLUT lookup to convert from 8 --> 24*/

struct pHidd_BitMap_CopyLUTMemBox24
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD            srcX, srcY;
    APTR      	    dst;
    WORD            dstX, dstY;
    UWORD   	    width, height;
    ULONG   	    srcMod;
    ULONG           dstMod;
    HIDDT_PixelLUT  *pixlut;    
};

/* copy src rect from 8 bit chunky memory buffer to
   dst rect in 32 bit chunky memory buffer using
   a HIDDT_PixelLUT lookup to convert from 8 --> 32*/

struct pHidd_BitMap_CopyLUTMemBox32
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD            srcX, srcY;
    APTR      	    dst;
    WORD            dstX, dstY;
    UWORD   	    width, height;
    ULONG   	    srcMod;
    ULONG           dstMod;
    HIDDT_PixelLUT  *pixlut;    
};

/* copy a chunky 8 bit image buffer contained in a 32 bit chunky array
   to dest 8 bit chunky memory buffer */

struct pHidd_BitMap_PutMem32Image8
{
    OOP_MethodID    mID;
    APTR    	    src;
    APTR	    dst;
    WORD	    dstX, dstY;
    UWORD	    width, height;
    ULONG   	    srcMod;
    ULONG   	    dstMod;
};

/* copy a chunky 16 bit image contained in a 32 bit chunky array
   to dest 16 bit chunky memory buffer */

struct pHidd_BitMap_PutMem32Image16
{
    OOP_MethodID    mID;
    APTR    	    src;
    APTR	    dst;
    WORD	    dstX, dstY;
    UWORD	    width, height;
    ULONG   	    srcMod;
    ULONG   	    dstMod;
};

/* copy a chunky 24 bit image contained in a 32 bit chunky array
   to dest 24 bit chunky memory buffer */

struct pHidd_BitMap_PutMem32Image24
{
    OOP_MethodID    mID;
    APTR    	    src;
    APTR	    dst;
    WORD	    dstX, dstY;
    UWORD	    width, height;
    ULONG   	    srcMod;
    ULONG   	    dstMod;
};

/* copy an area of a 8 bit chunky memory buffer into a
   8 bit image which is organized as a 32 bit chunky array */

struct pHidd_BitMap_GetMem32Image8
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD	    srcX, srcY;
    APTR	    dst;
    UWORD	    width, height;
    ULONG   	    srcMod;
    ULONG   	    dstMod;
};

/* copy an area of a 16 bit chunky memory buffer into a
   16 bit image which is organized in a 32 bit chunky array */

struct pHidd_BitMap_GetMem32Image16
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD	    srcX, srcY;
    APTR	    dst;
    UWORD	    width, height;
    ULONG   	    srcMod;
    ULONG   	    dstMod;
};

/* copy an area of a 24 bit chunky memory buffer into a
   24 bit image which is organized in a 32 bit chunky array */

struct pHidd_BitMap_GetMem32Image24
{
    OOP_MethodID    mID;
    APTR    	    src;
    WORD	    srcX, srcY;
    APTR	    dst;
    UWORD	    width, height;
    ULONG   	    srcMod;
    ULONG   	    dstMod;
};

struct pHidd_BitMap_PutMemTemplate8
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE   	    *Template;
    ULONG   	    modulo;
    WORD    	    srcx;
    APTR    	    dst;
    ULONG   	    dstMod;
    WORD    	    x, y;
    WORD    	    width, height;
    BOOL    	    inverttemplate;
};

struct pHidd_BitMap_PutMemTemplate16
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE   	    *Template;
    ULONG   	    modulo;
    WORD    	    srcx;
    APTR    	    dst;
    ULONG   	    dstMod;
    WORD    	    x, y;
    WORD    	    width, height;
    BOOL    	    inverttemplate;  
};

struct pHidd_BitMap_PutMemTemplate24
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE   	    *Template;
    ULONG   	    modulo;
    WORD    	    srcx;
    APTR    	    dst;
    ULONG   	    dstMod;
    WORD    	    x, y;
    WORD    	    width, height;
    BOOL    	    inverttemplate;  
};

struct pHidd_BitMap_PutMemTemplate32
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE   	    *Template;
    ULONG   	    modulo;
    WORD    	    srcx;
    APTR    	    dst;
    ULONG   	    dstMod;
    WORD    	    x, y;
    WORD    	    width, height;
    BOOL    	    inverttemplate;  
};

struct pHidd_BitMap_PutMemPattern8
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pattern;
    WORD    	    patternsrcx;
    WORD    	    patternsrcy;
    WORD    	    patternheight;
    WORD    	    patterndepth;
    HIDDT_PixelLUT *patternlut;
    BOOL    	    invertpattern;
    UBYTE   	    *mask;    
    ULONG	    maskmodulo;
    WORD    	    masksrcx;
    APTR    	    dst;
    ULONG   	    dstMod;
    WORD	    x, y;
    WORD	    width, height;
};

struct pHidd_BitMap_PutMemPattern16
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pattern;
    WORD    	    patternsrcx;
    WORD    	    patternsrcy;
    WORD    	    patternheight;
    WORD    	    patterndepth;
    HIDDT_PixelLUT *patternlut;
    BOOL    	    invertpattern;
    UBYTE   	    *mask;    
    ULONG	    maskmodulo;
    WORD    	    masksrcx;
    APTR    	    dst;
    ULONG   	    dstMod;
    WORD	    x, y;
    WORD	    width, height;
};

struct pHidd_BitMap_PutMemPattern24
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pattern;
    WORD    	    patternsrcx;
    WORD    	    patternsrcy;
    WORD    	    patternheight;
    WORD    	    patterndepth;
    HIDDT_PixelLUT *patternlut;
    BOOL    	    invertpattern;
    UBYTE   	    *mask;    
    ULONG	    maskmodulo;
    WORD    	    masksrcx;
    APTR    	    dst;
    ULONG   	    dstMod;
    WORD	    x, y;
    WORD	    width, height;
};

struct pHidd_BitMap_PutMemPattern32
{
    OOP_MethodID    mID;
    OOP_Object	    *gc;
    UBYTE 	    *pattern;
    WORD    	    patternsrcx;
    WORD    	    patternsrcy;
    WORD    	    patternheight;
    WORD    	    patterndepth;
    HIDDT_PixelLUT *patternlut;
    BOOL    	    invertpattern;
    UBYTE   	    *mask;    
    ULONG	    maskmodulo;
    WORD    	    masksrcx;
    APTR    	    dst;
    ULONG   	    dstMod;
    WORD	    x, y;
    WORD	    width, height;
};

struct pHidd_BitMap_SetColorMap
{
    OOP_MethodID    mID;
    OOP_Object      *colorMap;
};

struct pHidd_BitMap_ObtainDirectAccess
{
    OOP_MethodID    mID;
    UBYTE   	    **addressReturn;
    ULONG   	    *widthReturn;
    ULONG   	    *heightReturn;
    ULONG   	    *bankSizeReturn;
    ULONG   	    *memSizeReturn;
};

struct pHidd_BitMap_ReleaseDirectAccess
{
    OOP_MethodID    mID;
};

struct pHidd_BitMap_BitMapScale
{
    OOP_MethodID    	mID;
    OOP_Object      	*src;
    OOP_Object      	*dst;
    struct BitScaleArgs *bsa;
    OOP_Object	    	*gc;
};

typedef ULONG (*HIDDT_RGBConversionFunction)(APTR srcPixels, ULONG srcMod, HIDDT_StdPixFmt srcPixFmt, 
    	    	    	    	    	     APTR dstPixels, ULONG dstMod, HIDDT_StdPixFmt dstPixFmt,
					     ULONG width, ULONG height);

struct pHidd_BitMap_SetRGBConversionFunction
{
    OOP_MethodID    	    	    	mID;
    HIDDT_StdPixFmt 	    	    	srcPixFmt;
    HIDDT_StdPixFmt 	    	    	dstPixFmt;
    HIDDT_RGBConversionFunction	    	function;
};

struct pHidd_BitMap_UpdateRect
{
    OOP_MethodID    mID;
    WORD            x, y;
    WORD            width, height;
};

/**** Graphics context definitions ********************************************/
    /* Methods for a graphics context */
    
enum
{
    moHidd_GC_SetClipRect,
    moHidd_GC_UnsetClipRect
};

struct pHidd_GC_SetClipRect
{
    OOP_MethodID    mID;
    LONG    	    x1;
    LONG    	    y1;
    LONG    	    x2;
    LONG    	    y2;
};


struct pHidd_GC_UnsetClipRect
{
    OOP_MethodID mID;
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
    aoHidd_GC_LinePattern,         /* [.SG] Pattern for line drawing           */
    aoHidd_GC_LinePatternCnt,	   /* [.SG] Pattern start bit for line drawing */
    aoHidd_GC_PlaneMask,           /* [.SG] Shape bitmap                       */
    aoHidd_GC_ColorExpansionMode,  /* [.SG] Mode for color expansion 	       */
    
    num_Hidd_GC_Attrs
};

#define aHidd_GC_UserData    	    (HiddGCAttrBase + aoHidd_GC_UserData)
#define aHidd_GC_BitMap      	    (HiddGCAttrBase + aoHidd_GC_BitMap)
#define aHidd_GC_Foreground  	    (HiddGCAttrBase + aoHidd_GC_Foreground)
#define aHidd_GC_Background  	    (HiddGCAttrBase + aoHidd_GC_Background)
#define aHidd_GC_DrawMode    	    (HiddGCAttrBase + aoHidd_GC_DrawMode)
#define aHidd_GC_Font        	    (HiddGCAttrBase + aoHidd_GC_Font)
#define aHidd_GC_ColorMask   	    (HiddGCAttrBase + aoHidd_GC_ColorMask)
#define aHidd_GC_LinePattern 	    (HiddGCAttrBase + aoHidd_GC_LinePattern)
#define aHidd_GC_LinePatternCnt	    (HiddGCAttrBase + aoHidd_GC_LinePatternCnt)
#define aHidd_GC_PlaneMask   	    (HiddGCAttrBase + aoHidd_GC_PlaneMask)
#define aHidd_GC_ColorExpansionMode  (HiddGCAttrBase + aoHidd_GC_ColorExpansionMode)


/* Drawmodes for a graphics context */


#define vHidd_GC_DrawMode_Clear 	0x00 /* 0   	    	    */
#define vHidd_GC_DrawMode_And 		0x01 /* src AND dst	    */
#define vHidd_GC_DrawMode_AndReverse	0x02 /* src AND NOT dst     */
#define vHidd_GC_DrawMode_Copy 		0x03 /* src		    */
#define vHidd_GC_DrawMode_AndInverted	0x04 /* NOT src AND dst     */
#define vHidd_GC_DrawMode_NoOp	    	0x05 /* dst 	    	    */
#define vHidd_GC_DrawMode_Xor  		0x06 /* src XOR dst	    */
#define vHidd_GC_DrawMode_Or	    	0x07 /* src OR dst  	    */
#define vHidd_GC_DrawMode_Nor	    	0x08 /* NOT src AND NOT dst */
#define vHidd_GC_DrawMode_Equiv     	0x09 /* NOT src XOR dst     */
#define vHidd_GC_DrawMode_Invert  	0x0A /* NOT dst		    */
#define vHidd_GC_DrawMode_OrReverse 	0x0B /* src OR NOT dst	    */
#define vHidd_GC_DrawMode_CopyInverted  0x0C /* NOT src     	    */
#define vHidd_GC_DrawMode_OrInverted	0x0D /* NOT src OR dst	    */
#define vHidd_GC_DrawMode_Nand	    	0x0E /* NOT src OR NOT dst  */
#define vHidd_GC_DrawMode_Set	    	0x0F /* 1   	    	    */


#define vHidd_GC_ColExp_Transparent	(1 << 0)
#define vHidd_GC_ColExp_Opaque		(1 << 1)

/* Predeclarations of stubs in libhiddgraphicsstubs.h */

OOP_Object * HIDD_Gfx_NewGC        (OOP_Object *hiddGfx, struct TagItem *tagList);
VOID         HIDD_Gfx_DisposeGC    (OOP_Object *hiddGfx, OOP_Object *gc);
OOP_Object * HIDD_Gfx_NewBitMap    (OOP_Object *hiddGfx, struct TagItem *tagList);
VOID         HIDD_Gfx_DisposeBitMap(OOP_Object *hiddGfx, OOP_Object *bitMap);

HIDDT_ModeID *HIDD_Gfx_QueryModeIDs(OOP_Object *hiddGfx, struct TagItem *queryTags);
VOID 	      HIDD_Gfx_ReleaseModeIDs(OOP_Object *hiddGfx, HIDDT_ModeID *modeIDs);
OOP_Object   *HIDD_Gfx_GetPixFmt(OOP_Object *obj, HIDDT_StdPixFmt pixFmt);
BOOL 	      HIDD_Gfx_CheckMode(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object *sync, OOP_Object *pixFmt);
BOOL 	      HIDD_Gfx_GetMode(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object **syncPtr, OOP_Object **pixFmtPtr);
HIDDT_ModeID  HIDD_Gfx_NextModeID(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object **syncPtr, OOP_Object **pixFmtPtr);

BOOL HIDD_Gfx_SetCursorShape(OOP_Object *obj, OOP_Object *shape, LONG xoffset, LONG yoffset);
BOOL HIDD_Gfx_SetCursorPos(OOP_Object *obj, LONG x, LONG y);
VOID HIDD_Gfx_SetCursorVisible(OOP_Object *obj, BOOL visible);

OOP_Object *HIDD_Gfx_Show(OOP_Object *obj, OOP_Object *bitMap, ULONG flags);
BOOL 	    HIDD_Gfx_SetMode(OOP_Object *obj, OOP_Object *sync);
VOID  	    HIDD_Gfx_CopyBox(OOP_Object *obj, OOP_Object *src, WORD srcX, WORD srcY, OOP_Object *dest, WORD destX, WORD destY, UWORD width, UWORD height, OOP_Object *gc);
ULONG       HIDD_Gfx_ModeProperties(OOP_Object *obj, HIDDT_ModeID modeID, struct HIDD_ModeProperties *props, ULONG propsLen);
ULONG	    HIDD_Gfx_ShowViewPorts(OOP_Object *obj, struct HIDD_ViewPortData *data);
OOP_Object *HIDD_Gfx_GetSync(OOP_Object *obj, ULONG num);
BOOL HIDD_Gfx_GetGamma(OOP_Object *obj, UBYTE *Red, UBYTE *Green, UBYTE *Blue);
BOOL HIDD_Gfx_SetGamma(OOP_Object *obj, UBYTE *Red, UBYTE *Green, UBYTE *Blue);

VOID HIDD_GC_SetClipRect(OOP_Object *gc, LONG x1, LONG y1, LONG x2, LONG y2);
VOID HIDD_GC_UnsetClipRect(OOP_Object *gc);

VOID     HIDD_BM_BltBitMap   (OOP_Object *obj, OOP_Object *dest, WORD srcX, WORD srcY, WORD destX, WORD destY, WORD width, WORD height);
BOOL     HIDD_BM_Show        (OOP_Object *obj);
VOID     HIDD_BM_Move        (OOP_Object *obj, WORD x, WORD y);
BOOL     HIDD_BM_DepthArrange(OOP_Object *obj, OOP_Object *bm);
BOOL	 HIDD_BM_SetColors	(OOP_Object *obj, HIDDT_Color *tab, ULONG firstcolor, ULONG numcolors);

ULONG       HIDD_BM_PutPixel	    	(OOP_Object *obj, WORD x, WORD y, HIDDT_Pixel pixel);
HIDDT_Pixel HIDD_BM_GetPixel        	(OOP_Object *obj, WORD x, WORD y);
ULONG       HIDD_BM_DrawPixel       	(OOP_Object *obj, OOP_Object *gc, WORD x, WORD y);
VOID        HIDD_BM_GetImage	    	(OOP_Object *obj, UBYTE *pixelArray, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_StdPixFmt pixFmt);
VOID	    HIDD_BM_PutImage 	    	(OOP_Object *obj, OOP_Object *gc, UBYTE *pixelArray, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_StdPixFmt pixFmt);
VOID	    HIDD_BM_PutAlphaImage 	(OOP_Object *obj, OOP_Object *gc, UBYTE *pixelArray, ULONG modulo, WORD x, WORD y, WORD width, WORD height);
VOID	    HIDD_BM_PutTemplate 	(OOP_Object *obj, OOP_Object *gc, UBYTE *Template, ULONG modulo, WORD srcx, WORD x, WORD y, WORD width, WORD height, BOOL inverttemplate);
VOID	    HIDD_BM_PutAlphaTemplate 	(OOP_Object *obj, OOP_Object *gc, UBYTE *alpha, ULONG modulo, WORD x, WORD y, WORD width, WORD height, BOOL invertalpha);
VOID	    HIDD_BM_PutPattern	 	(OOP_Object *obj, OOP_Object *gc, UBYTE *pattern, WORD patternsrcx, WORD patternsrcy, WORD patternheight, WORD patterndepth, HIDDT_PixelLUT *patternlut, BOOL invertpattern, UBYTE *mask, ULONG maskmodulo, WORD masksrcx, WORD x, WORD y, WORD width, WORD height);
VOID        HIDD_BM_DrawLine        	(OOP_Object *obj, OOP_Object *gc, WORD x1, WORD y1, WORD x2, WORD y2);
VOID        HIDD_BM_DrawRect        	(OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY);
VOID        HIDD_BM_FillRect        	(OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY);
VOID        HIDD_BM_DrawEllipse     	(OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, WORD ry, WORD rx);
VOID        HIDD_BM_FillEllipse     	(OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, WORD ry, WORD rx);
VOID        HIDD_BM_DrawArc         	(OOP_Object *obj, OOP_Object *gc);
VOID        HIDD_BM_FillArc         	(OOP_Object *obj, OOP_Object *gc);
VOID        HIDD_BM_DrawPolygon     	(OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords);
VOID        HIDD_BM_FillPolygon     	(OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords);
VOID        HIDD_BM_DrawText        	(OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, STRPTR text, UWORD length);
VOID        HIDD_BM_FillText        	(OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, STRPTR text, UWORD length);
VOID        HIDD_BM_FillSpan        	(OOP_Object *obj);
VOID        HIDD_BM_Clear           	(OOP_Object *obj, OOP_Object *gc);
VOID	    HIDD_BM_BlitColorExpansion	(OOP_Object *destObj, OOP_Object *gc, OOP_Object *srcObj, WORD srcX, WORD srcY, WORD destX, WORD destY,  UWORD width, UWORD height);

HIDDT_Pixel HIDD_BM_MapColor 	(OOP_Object *destObj, HIDDT_Color *color);
VOID	    HIDD_BM_UnmapPixel	(OOP_Object *destObj, HIDDT_Pixel pixel, HIDDT_Color *color);

VOID	    HIDD_BM_PutImageLUT (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut);
VOID	    HIDD_BM_PutTranspImageLUT (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut, UBYTE transparent);
VOID	    HIDD_BM_GetImageLUT (OOP_Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut);

ULONG 	    HIDD_BM_BytesPerLine(OOP_Object *obj, HIDDT_StdPixFmt pixFmt, ULONG width);

VOID     HIDD_BM_ConvertPixels  (OOP_Object *obj,
				 APTR *srcPixels,
				 HIDDT_PixelFormat *srcPixFmt,
				 ULONG srcMod,
				 APTR *dstBuf,
				 HIDDT_PixelFormat *dstPixFmt,
				 ULONG dstMod,
				 ULONG width,
				 ULONG height,
				 HIDDT_PixelLUT *pixlut);

VOID	HIDD_BM_FillMemRect8 (OOP_Object *obj,
    			      APTR dstBuf,
			      WORD minX,
			      WORD minY,
			      WORD maxX,
			      WORD maxY,
			      ULONG dstMod,
			      UBYTE fill);

VOID	HIDD_BM_FillMemRect16 (OOP_Object *obj,
    			       APTR dstBuf,
			       WORD minX,
			       WORD minY,
			       WORD maxX,
			       WORD maxY,
			       ULONG dstMod,
			       UWORD fill);

VOID	HIDD_BM_FillMemRect24 (OOP_Object *obj,
    			       APTR dstBuf,
			       WORD minX,
			       WORD minY,
			       WORD maxX,
			       WORD maxY,
			       ULONG dstMod,
			       ULONG fill);

VOID	HIDD_BM_FillMemRect32 (OOP_Object *obj,
    			       APTR dstBuf,
			       WORD minX,
			       WORD minY,
			       WORD maxX,
			       WORD maxY,
			       ULONG dstMod,
			       ULONG fill);

VOID	HIDD_BM_InvertMemRect(OOP_Object *obj,
    			      APTR dstBuf,
			      WORD minX,
			      WORD minY,
			      WORD maxX,
			      WORD maxY,
			      ULONG dstMod);

VOID	HIDD_BM_CopyMemBox8(OOP_Object *obj,
    			    APTR src,
			    WORD srcX,
			    WORD srcY,
			    APTR dst,
			    WORD dstX,
			    WORD dstY,
			    UWORD width,
			    UWORD height,
			    ULONG srcMod,
			    ULONG dstMod);
	
VOID	HIDD_BM_CopyMemBox16(OOP_Object *obj,
    			     APTR src,
			     WORD srcX,
			     WORD srcY,
			     APTR dst,
			     WORD dstX,
			     WORD dstY,
			     UWORD width,
			     UWORD height,
			     ULONG srcMod,
			     ULONG dstMod);

VOID	HIDD_BM_CopyMemBox24(OOP_Object *obj,
    	    	    	     APTR src,
			     WORD srcX,
			     WORD srcY,
			     APTR dst,
			     WORD dstX,
			     WORD dstY,
			     UWORD width,
			     UWORD height,
			     ULONG srcMod,
			     ULONG dstMod);

VOID	HIDD_BM_CopyMemBox32(OOP_Object *obj,
    	    	    	     APTR src,
			     WORD srcX,
			     WORD srcY,
			     APTR dst,
			     WORD dstX,
			     WORD dstY,
			     UWORD width,
			     UWORD height,
			     ULONG srcMod,
			     ULONG dstMod);

VOID	HIDD_BM_CopyLUTMemBox16(OOP_Object *obj,
    				APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				WORD dstX,
				WORD dstY,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod,
				HIDDT_PixelLUT *pixlut);

VOID	HIDD_BM_CopyLUTMemBox24(OOP_Object *obj,
    				APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				WORD dstX,
				WORD dstY,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod,
				HIDDT_PixelLUT *pixlut);

VOID	HIDD_BM_CopyLUTMemBox32(OOP_Object *obj,
    				APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				WORD dstX,
				WORD dstY,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod,
				HIDDT_PixelLUT *pixlut);
	
VOID	HIDD_BM_PutMem32Image8(OOP_Object *obj,
    	    	    	       APTR src,
			       APTR dst,
			       WORD dstX,
			       WORD dstY,
			       UWORD width,
			       UWORD height,
			       ULONG srcMod,
			       ULONG dstMod);

VOID	HIDD_BM_PutMem32Image16(OOP_Object *obj,
    	    	    		APTR src,
				APTR dst,
				WORD dstX,
				WORD dstY,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod);

VOID	HIDD_BM_PutMem32Image24(OOP_Object *obj,
    	    	    		APTR src,
				APTR dst,
				WORD dstX,
				WORD dstY,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod);

VOID	HIDD_BM_GetMem32Image8(OOP_Object *obj,
    	    	    	       APTR src,
			       WORD srcX,
			       WORD srcY,
			       APTR dst,
			       UWORD width,
			       UWORD height,
			       ULONG srcMod,
			       ULONG dstMod);

VOID	HIDD_BM_GetMem32Image16(OOP_Object *obj,
    	    	    		APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod);

VOID	HIDD_BM_GetMem32Image24(OOP_Object *obj,
    	    	    		APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod);

VOID	HIDD_BM_PutMemTemplate8	(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *Template,
				 ULONG modulo,
				 WORD srcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height,
				 BOOL inverttemplate);
				 
VOID	HIDD_BM_PutMemTemplate16(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *Template,
				 ULONG modulo,
				 WORD srcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height,
				 BOOL inverttemplate);
				 
VOID	HIDD_BM_PutMemTemplate24(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *Template,
				 ULONG modulo,
				 WORD srcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height,
				 BOOL inverttemplate);
				 
VOID	HIDD_BM_PutMemTemplate32(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *Template,
				 ULONG modulo,
				 WORD srcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height,
				 BOOL inverttemplate);

VOID	HIDD_BM_PutMemPattern8	(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *pattern,
				 WORD patternsrcx,
				 WORD patternsrcy,
				 WORD patternheight,
				 WORD patterndepth,
				 HIDDT_PixelLUT *patternlut,
				 BOOL invertpattern,
				 UBYTE *mask,
				 ULONG maskmodulo,
				 WORD masksrcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height);
				 
VOID	HIDD_BM_PutMemPattern16	(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *pattern,
				 WORD patternsrcx,
				 WORD patternsrcy,
				 WORD patternheight,
				 WORD patterndepth,
				 HIDDT_PixelLUT *patternlut,
				 BOOL invertpattern,
				 UBYTE *mask,
				 ULONG maskmodulo,
				 WORD masksrcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height);
				 
VOID	HIDD_BM_PutMemPattern24	(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *pattern,
				 WORD patternsrcx,
				 WORD patternsrcy,
				 WORD patternheight,
				 WORD patterndepth,
				 HIDDT_PixelLUT *patternlut,
				 BOOL invertpattern,
				 UBYTE *mask,
				 ULONG maskmodulo,
				 WORD masksrcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height);

VOID	HIDD_BM_PutMemPattern32	(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *pattern,
				 WORD patternsrcx,
				 WORD patternsrcy,
				 WORD patternheight,
				 WORD patterndepth,
				 HIDDT_PixelLUT *patternlut,
				 BOOL invertpattern,
				 UBYTE *mask,
				 ULONG maskmodulo,
				 WORD masksrcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height);

OOP_Object * HIDD_BM_SetColorMap(OOP_Object *o, OOP_Object *colorMap);

BOOL HIDD_BM_ObtainDirectAccess(OOP_Object *o,
    	    	    	    	UBYTE **addressReturn,
				ULONG *widthReturn,
				ULONG *heightReturn,
				ULONG *bankSizeReturn,
				ULONG *memSizeReturn);

VOID HIDD_BM_ReleaseDirectAccess(OOP_Object *obj);

VOID HIDD_BM_BitMapScale(OOP_Object *obj, OOP_Object *src, OOP_Object *dest, struct BitScaleArgs * bsa, OOP_Object *gc);

HIDDT_RGBConversionFunction HIDD_BM_SetRGBConversionFunction(OOP_Object *o,
    	    	    	    	HIDDT_StdPixFmt srcPixFmt,
				HIDDT_StdPixFmt dstPixFmt,
				HIDDT_RGBConversionFunction function);

VOID HIDD_BM_UpdateRect(OOP_Object *obj, WORD x, WORD y, WORD width, WORD height);
				
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


#define BM_PIXFMT(bm) (HIDDT_PixelFormat *)(((struct _hidd_bitmap_protected *)bm)->pixfmt)
#define BM_DEPTH(bm)	BM_PIXFMT(bm)->depth


/****** !!!! PROTECTED DATA !!!!
	This typedef can be used to acces a GC object directly from within
	a bitmap class, but NEVER EVER use it to access a GC object
	from outside a bitmap class. And when accessing from inside a
	bitmap class, use the macros below !
*/

typedef struct
{
    HIDDT_Pixel     fg;        /* foreground color                                 */
    HIDDT_Pixel     bg;        /* background color                                 */
    HIDDT_DrawMode  drMode;    /* drawmode                                         */
    /* WARNING: type of font could be change */
    APTR    	    font;      /* current fonts                                    */
    ULONG   	    colMask;   /* ColorMask prevents some color bits from changing */
    UWORD   	    linePat;   /* LinePattern                                      */
    UWORD   	    linePatCnt; /* LinePattern start bit    	    	    	   */
    APTR    	    planeMask; /* Pointer to a shape bitMap                        */
    ULONG   	    colExp;
    
    BOOL    	    doClip;

    LONG    	    clipX1;
    LONG    	    clipY1;
    LONG      	    clipX2;
    LONG    	    clipY2;
 
} HIDDT_GC_Intern;

#define GCINT(gc)	    ((HIDDT_GC_Intern *)gc)
#define GC_FG(gc)	    (GCINT(gc)->fg)
#define GC_BG(gc)	    (GCINT(gc)->bg)
#define GC_DRMD(gc)	    (GCINT(gc)->drMode)
#define GC_FONT(gc)	    (GCINT(gc)->font)
#define GC_COLMASK(gc)	    (GCINT(gc)->colMask)
#define GC_LINEPAT(gc)	    (GCINT(gc)->linePat)
#define GC_LINEPATCNT(gc)   (GCINT(gc)->linePatCnt)
#define GC_PLANEMASK(gc)    (GCINT(gc)->planeMask)
#define GC_COLEXP(gc)	    (GCINT(gc)->colExp)

#define GC_DOCLIP(gc)	    (GCINT(gc)->doClip)
#define GC_CLIPX1(gc)	    (GCINT(gc)->clipX1)
#define GC_CLIPY1(gc)	    (GCINT(gc)->clipY1)
#define GC_CLIPX2(gc)	    (GCINT(gc)->clipX2)
#define GC_CLIPY2(gc)	    (GCINT(gc)->clipY2)

#define GC_(gc)	    	    (GCINT(gc)->)



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


#define vHidd_ColorModel_Mask 	0x0000000F
#define vHidd_ColorModel_Shift	0
#define vHidd_BitMapType_Mask 	0x000000F0
#define vHidd_BitMapType_Shift	4

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
#define SET_PF_COLMODEL(pf, cm)	\
     (pf)->flags &= ~vHidd_ColorModel_Mask;	\
     (pf)->flags |= ( (cm) << vHidd_ColorModel_Shift);
     

#define IS_PALETTIZED(pf) (    (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_Palette)	\
			    || (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_StaticPalette) )
			    
#define IS_TRUECOLOR(pf) 	( (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_TrueColor) )
#define IS_PALETTE(pf)  	( (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_Palette) )
#define IS_STATICPALETTE(pf)  	( (HIDD_PF_COLMODEL(pf) == vHidd_ColorModel_StaticPalette) )



#define HIDD_PF_BITMAPTYPE(pf) ( ((pf)->flags & vHidd_BitMapType_Mask) >> vHidd_BitMapType_Shift )
#define SET_PF_BITMAPTYPE(pf, bmt)	\
     (pf)->flags &= ~vHidd_BitMapType_Mask;	\
     (pf)->flags |= ( (bmt) << vHidd_BitMapType_Shift );


#define PF_GRAPHTYPE(cmodel, bmtype)	\
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

enum
{
    aoHidd_PixFmt_ColorModel = 0,	/* HIDDT_ColorModel */
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
    aoHidd_PixFmt_BitMapType,		/* HIDDT_BitMapType */
    aoHidd_PixFmt_SwapPixelBytes,  
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
#define aHidd_PixFmt_ColorModel		(HiddPixFmtAttrBase + aoHidd_PixFmt_ColorModel)
#define aHidd_PixFmt_StdPixFmt		(HiddPixFmtAttrBase + aoHidd_PixFmt_StdPixFmt)
#define aHidd_PixFmt_CLUTShift		(HiddPixFmtAttrBase + aoHidd_PixFmt_CLUTShift)
#define aHidd_PixFmt_CLUTMask		(HiddPixFmtAttrBase + aoHidd_PixFmt_CLUTMask)
#define aHidd_PixFmt_BitMapType		(HiddPixFmtAttrBase + aoHidd_PixFmt_BitMapType)
#define aHidd_PixFmt_SwapPixelBytes	(HiddPixFmtAttrBase + aoHidd_PixFmt_SwapPixelBytes)


#define IS_PIXFMT_ATTR(attr, idx) \
	( ( ( idx ) = (attr) - HiddPixFmtAttrBase) < num_Hidd_PixFmt_Attrs)



/********** Planar bitmap *******************/

#define CLID_Hidd_PlanarBM "hidd.graphics.bitmap.planarbm"
#define IID_Hidd_PlanarBM  "hidd.graphics.bitmap.planarbm"

#define HiddPlanarBMAttrBase __IHidd_PlanarBM


#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddPlanarBMAttrBase;
#endif

enum
{
    moHidd_PlanarBM_SetBitMap,
    moHidd_PlanarBM_GetBitMap,
};

struct pHidd_PlanarBM_SetBitMap
{
    OOP_MethodID    mID;
    struct BitMap   *bitMap;
};

struct pHidd_PlanarBM_GetBitMap
{
    OOP_MethodID    mID;
    struct BitMap   *bitMap;
};

BOOL HIDD_PlanarBM_SetBitMap(OOP_Object *obj, struct BitMap *bitMap);
BOOL HIDD_PlanarBM_GetBitMap(OOP_Object *obj, struct BitMap *bitMap);

enum
{
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

#define HiddColorMapAttrBase __IHidd_ColorMap

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddColorMapAttrBase;
#endif

/* Methods */
enum
{
    moHidd_ColorMap_SetColors,
    moHidd_ColorMap_GetPixel,
    moHidd_ColorMap_GetColor
};

struct pHidd_ColorMap_SetColors
{
    OOP_MethodID    mID;
    HIDDT_Color	    *colors;
    ULONG	    firstColor;
    ULONG	    numColors;
    OOP_Object	    *pixFmt;
};

struct pHidd_ColorMap_GetPixel
{
    OOP_MethodID    mID;
    ULONG   	    pixelNo;
};

struct pHidd_ColorMap_GetColor
{
    OOP_MethodID    mID;
    ULONG   	    colorNo;
    HIDDT_Color     *colorReturn;
};

BOOL HIDD_CM_SetColors(OOP_Object *obj, HIDDT_Color *colors, ULONG firstColor, ULONG numColors, OOP_Object *pixFmt);
HIDDT_Pixel HIDD_CM_GetPixel(OOP_Object *obj, ULONG pixelNo);
BOOL HIDD_CM_GetColor(OOP_Object *obj, ULONG colorNo, HIDDT_Color *colorReturn);

/* Attrs */
enum
{
    aoHidd_ColorMap_NumEntries,	/* [I.G] ULONG - number of colors in the colormap */
    
    num_Hidd_ColorMap_Attrs
};

#define aHidd_ColorMap_NumEntries   (HiddColorMapAttrBase + aoHidd_ColorMap_NumEntries)
#define aHidd_ColorMap_		    (HiddColorMapAttrBase + aoHidd_ColorMap_)



#define IS_COLORMAP_ATTR(attr, idx) \
	( ( ( idx ) = (attr) - HiddColorMapAttrBase) < num_Hidd_ColorMap_Attrs)


/************* Sync class **************************************/

/* This class contains info on the horizontal/vertical syncing */

#define HiddSyncAttrBase __IHidd_Sync
#define IID_Hidd_Sync "hidd.gfx.sync"

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddSyncAttrBase;
#endif

enum
{
    
    /* Linux framebuffer device alike specification, deprecated */
    aoHidd_Sync_PixelTime = 0,  /* [ISG] ULONG - pixel clock in picoseconds (1E-12 second) ie. time it takes to draw one pixel */

    aoHidd_Sync_LeftMargin,	/* [ISG] ULONG */
    aoHidd_Sync_RightMargin,	/* [ISG] ULONG */
    aoHidd_Sync_HSyncLength,	/* [ISG] ULONG */

    aoHidd_Sync_UpperMargin,	/* [ISG] ULONG */
    aoHidd_Sync_LowerMargin,	/* [ISG] ULONG */
    aoHidd_Sync_VSyncLength,	/* [ISG] ULONG */

    /* Alternative description used by newer drivers. Use this one. */
    aoHidd_Sync_PixelClock,	/* [ISG] ULONG - Pixel clock in Hz */

    aoHidd_Sync_HDisp,		/* [I.G] ULONG - displayed pixels per line */
    aoHidd_Sync_HSyncStart,	/* [ISG] ULONG - time to the start of the horizontal sync */
    aoHidd_Sync_HSyncEnd,	/* [ISG] ULONG - time to the end of the horizontal sync */
    aoHidd_Sync_HTotal,		/* [ISG] ULONG - total time to draw one line + the hsync time	*/

    aoHidd_Sync_VDisp,		/* [I.G] ULONG - displayed rows */
    aoHidd_Sync_VSyncStart,	/* [ISG] ULONG - rows to the start of the horizontal sync */
    aoHidd_Sync_VSyncEnd,	/* [ISG] ULONG - rows to the end of the horizontal synf */
    aoHidd_Sync_VTotal,		/* [ISG] ULONG - number of rows in the screen includeing vsync 	*/

    aoHidd_Sync_Description,	/* [I.G] STRPTR - guess what */

    aoHidd_Sync_HMin,		/* [ISG] ULONG - minimum acceptable bitmap width */
    aoHidd_Sync_HMax,		/* [ISG] ULONG - maximum acceptable bitmap width */
    aoHidd_Sync_VMin,		/* [ISG] ULONG - minimum acceptable bitmap height */
    aoHidd_Sync_VMax,		/* [ISG] ULONG - maximum acceptable bitmap height */

    aoHidd_Sync_Flags,		/* [I.G] ULONG - mode tags */

    aoHidd_Sync_Variable,	/* [I.G] BOOL  - data can be modified */
    aoHidd_Sync_MonitorSpec,	/* [I.G] struct MonitorSpec *	- MonitorSpec structure		     */
    aoHidd_Sync_GfxHidd,	/* [I.G] OOP_Object *		- Driver to which the object belongs */
    aoHidd_Sync_BoardNumber,	/* [I..] ULONG - Number of board (replaces '%b' in description) */

    num_Hidd_Sync_Attrs
    
};

#define aHidd_Sync_PixelTime	(HiddSyncAttrBase + aoHidd_Sync_PixelTime)

#define aHidd_Sync_LeftMargin	(HiddSyncAttrBase + aoHidd_Sync_LeftMargin)
#define aHidd_Sync_RightMargin	(HiddSyncAttrBase + aoHidd_Sync_RightMargin)
#define aHidd_Sync_HSyncLength	(HiddSyncAttrBase + aoHidd_Sync_HSyncLength)

#define aHidd_Sync_UpperMargin	(HiddSyncAttrBase + aoHidd_Sync_UpperMargin)
#define aHidd_Sync_LowerMargin	(HiddSyncAttrBase + aoHidd_Sync_LowerMargin)
#define aHidd_Sync_VSyncLength	(HiddSyncAttrBase + aoHidd_Sync_VSyncLength)


#define aHidd_Sync_PixelClock	(HiddSyncAttrBase + aoHidd_Sync_PixelClock)

#define aHidd_Sync_HDisp	(HiddSyncAttrBase + aoHidd_Sync_HDisp)
#define aHidd_Sync_HSyncStart	(HiddSyncAttrBase + aoHidd_Sync_HSyncStart)
#define aHidd_Sync_HSyncEnd	(HiddSyncAttrBase + aoHidd_Sync_HSyncEnd)
#define aHidd_Sync_HTotal	(HiddSyncAttrBase + aoHidd_Sync_HTotal)

#define aHidd_Sync_VDisp	(HiddSyncAttrBase + aoHidd_Sync_VDisp)
#define aHidd_Sync_VSyncStart	(HiddSyncAttrBase + aoHidd_Sync_VSyncStart)
#define aHidd_Sync_VSyncEnd	(HiddSyncAttrBase + aoHidd_Sync_VSyncEnd)
#define aHidd_Sync_VTotal	(HiddSyncAttrBase + aoHidd_Sync_VTotal)

#define aHidd_Sync_Description	(HiddSyncAttrBase + aoHidd_Sync_Description)

#define aHidd_Sync_HMin		(HiddSyncAttrBase + aoHidd_Sync_HMin)
#define aHidd_Sync_HMax 	(HiddSyncAttrBase + aoHidd_Sync_HMax)
#define aHidd_Sync_VMin		(HiddSyncAttrBase + aoHidd_Sync_VMin)
#define aHidd_Sync_VMax		(HiddSyncAttrBase + aoHidd_Sync_VMax)

#define aHidd_Sync_Flags	(HiddSyncAttrBase + aoHidd_Sync_Flags)

#define aHidd_Sync_Variable	(HiddSyncAttrBase + aoHidd_Sync_Variable)
#define aHidd_Sync_MonitorSpec	(HiddSyncAttrBase + aoHidd_Sync_MonitorSpec)
#define aHidd_Sync_GfxHidd	(HiddSyncAttrBase + aoHidd_Sync_GfxHidd)
#define aHidd_Sync_BoardNumber	(HiddSyncAttrBase + aoHidd_Sync_BoardNumber)

/* Sync flags */
#define vHidd_Sync_HSyncPlus		0x0001	/* HSYNC + if set */
#define vHidd_Sync_VSyncPlus		0x0002	/* VSYNC + if set */
#define vHidd_Sync_Interlaced		0x0004 	/* Interlaced mode */
#define vHidd_Sync_DblScan		0x0008 	/* Double scanline */

#define IS_SYNC_ATTR(attr, idx) \
	( ( ( idx ) = (attr) - HiddSyncAttrBase) < num_Hidd_Sync_Attrs)


#endif /* HIDD_GRAPHICS_H */



