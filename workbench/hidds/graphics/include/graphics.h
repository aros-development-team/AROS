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

#define CLID_Hidd_Gfx           "graphics.hidd"
#define CLID_Hidd_BitMap        "bitmap.hidd"
#define CLID_Hidd_GCQuick       "gcquick.hidd"
#define CLID_Hidd_GCClip        "gcclip.hidd"

#define IID_Hidd_Gfx      "I_GfxHidd"
#define IID_Hidd_BitMap   "I_HiddBitMap"
#define IID_Hidd_GC       "I_HiddGC"

typedef struct Object *HIDDT_BitMap;
typedef struct Object *HIDDT_GC;


/* Attrbases */
#define HiddGCAttrBase          __IHidd_GC
#define HiddGfxAttrBase         __IHidd_Gfx
#define HiddBitMapAttrBase      __IHidd_BitMap

extern AttrBase HiddGCAttrBase;
extern AttrBase HiddGfxAttrBase;
extern AttrBase HiddBitMapAttrBase;


/**** Graphics definitions ****************************************************/

enum
{
    /* Methods for a graphics hidd */

    moHidd_Gfx_NewGC = 0,       /* Its only allowd to use these methods   */
    moHidd_Gfx_DisposeGC,       /* to create and to dispose a GC and      */
    moHidd_Gfx_NewBitMap,       /* bitmap because only the graphics-hidd  */
    moHidd_Gfx_DisposeBitMap   /* class knows which gc- and bitmap-class */
				/* works together.                        */
};


/* GC types */

#define vHIDD_Gfx_GCType_Quick  0x1
#define vHIDD_Gfx_GCType_Clip   0x2


/* messages for a graphics hidd */

struct pHidd_Gfx_NewGC
{
    MethodID       mID;
    ULONG          gcType;
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
    struct TagItem *attrList;
};

struct pHidd_Gfx_DisposeBitMap
{
    MethodID    mID;
    Object      *bitMap;
};



/**** BitMap definitions ******************************************************/


	/* Types */
	
typedef struct
{
   UWORD	red;
   UWORD	green;
   UWORD	blue;
   
} HIDDT_Color;


enum
{
    /* Methods for a bitmap */

    moHidd_BitMap_SetColors,
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
    aoHidd_BitMap_Mode,          /* [ISG] The display mode of this bitmap      */
    aoHidd_BitMap_BaseAddress,   /* [ISG] Bitmap adress in RAM                 */
    aoHidd_BitMap_Format,        /* [..G] Tell the format of the bitmap data   */
    aoHidd_BitMap_BytesPerRow,   /* [..G] Number of bytes in a row             */
    aoHidd_BitMap_BytesPerPixel, /* [..G] Number of byter per pixel            */
    aoHidd_BitMap_BestSize,      /* [..G] Best size for depth                  */
    aoHidd_BitMap_LeftEdge,      /* [I.G] Left edge position of the bitmap     */
    aoHidd_BitMap_TopEdge,       /* [I.G] Top edge position of the bitmap      */
    aoHidd_BitMap_ColorTab,      /* [ISG] Colormap of the bitmap               */
    aoHidd_BitMap_AllocBuffer,   /* [I..] BOOL allocate buffer (default: TRUE) */
    
    num_Hidd_BitMap_Attrs
};    

#define aHidd_BitMap_BitMap        (HiddBitMapAttrBase + aoHidd_BitMap_BitMap)
#define aHidd_BitMap_Width         (HiddBitMapAttrBase + aoHidd_BitMap_Width)
#define aHidd_BitMap_Height        (HiddBitMapAttrBase + aoHidd_BitMap_Height)
#define aHidd_BitMap_Depth         (HiddBitMapAttrBase + aoHidd_BitMap_Depth)
#define aHidd_BitMap_Displayable   (HiddBitMapAttrBase + aoHidd_BitMap_Displayable)
#define aHidd_BitMap_Visible       (HiddBitMapAttrBase + aoHidd_BitMap_Visible)
#define aHidd_BitMap_Mode          (HiddBitMapAttrBase + aoHidd_BitMap_Mode)
#define aHidd_BitMap_BaseAddress   (HiddBitMapAttrBase + aoHidd_BitMap_BaseAddress)
#define aHidd_BitMap_Format        (HiddBitMapAttrBase + aoHidd_BitMap_Format)
#define aHidd_BitMap_BytesPerRow   (HiddBitMapAttrBase + aoHidd_BitMap_BytesPerRow)
#define aHidd_BitMap_BytesPerPixel (HiddBitMapAttrBase + aoHidd_BitMap_BytesPerPixel)
#define aHidd_BitMap_BestSize      (HiddBitMapAttrBase + aoHidd_BitMap_BestSize)
#define aHidd_BitMap_LeftEdge      (HiddBitMapAttrBase + aoHidd_BitMap_LeftEdge)
#define aHidd_BitMap_TopEdge       (HiddBitMapAttrBase + aoHidd_BitMap_TopEdge)
#define aHidd_BitMap_ColorTab      (HiddBitMapAttrBase + aoHidd_BitMap_ColorTab)
#define aHidd_BitMap_AllocBuffer   (HiddBitMapAttrBase + aoHidd_BitMap_AllocBuffer)


/* BitMap formats */

#define vHIDD_BitMap_Format_Planar   0x1
#define vHIDD_BitMap_Format_Chunky   0x2


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



/**** Graphics context definitions ********************************************/

enum
{
    /* Methods for a graphics context */

    moHidd_GC_CopyArea,
    moHidd_GC_WritePixelDirect,
    moHidd_GC_WritePixel,
    moHidd_GC_ReadPixel,
    moHidd_GC_DrawLine,
    moHidd_GC_DrawRect,
    moHidd_GC_FillRect,
    moHidd_GC_DrawEllipse,
    moHidd_GC_FillEllipse,
    moHidd_GC_DrawPolygon,
    moHidd_GC_FillPolygon,
    moHidd_GC_DrawText,
    moHidd_GC_FillText,
    moHidd_GC_FillSpan,
    moHidd_GC_Clear,
    moHidd_GC_ReadPixelArray,
    moHidd_GC_WritePixelArray
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


/* Drawmodes for a graphics context */

#define vHIDD_GC_DrawMode_Copy 0x03 /* Copy src into destination            */
#define vHIDD_GC_DrawMode_XOR  0x06 /* XOR                                  */

/* obsolete */
#define HIDDV_GC_DrawMode_Copy 0x03 /* Copy src into destination            */
#define HIDDV_GC_DrawMode_XOR  0x06 /* XOR                                  */


/* messages for a graphics context */

struct pHidd_GC_WritePixelDirect
{
    MethodID  mID;
    WORD x, y;
    ULONG val;
};

struct pHidd_GC_ReadPixel
{
    MethodID  mID;
    WORD x, y;
};

struct pHidd_GC_WritePixel
{
    MethodID  mID;
    WORD x, y;
};

struct pHidd_GC_DrawLine
{
    MethodID    mID;
    WORD        x1 ,y1, x2, y2;
};

struct pHidd_GC_CopyArea
{
    MethodID    mID;
    WORD        srcX, srcY;
    Object      *dest;
    WORD        destX, destY;
    UWORD       width, height;
};

struct pHidd_GC_DrawRect
{
    MethodID    mID;
    WORD        minX, minY, maxX, maxY;
};

struct pHidd_GC_DrawEllipse
{
    MethodID    mID;
    WORD        x, y;
    UWORD       rx, ry;
};

struct pHidd_GC_DrawPolygon
{
    MethodID    mID;
    WORD        n;         /* number of coordinates */
    WORD        *coords;   /* size 2*n              */
};

struct pHidd_GC_DrawText
{
    MethodID    mID;
    WORD        x, y;      /* Start position, see autodocs */
    STRPTR      text;      /* Latin 1 string               */
    UWORD       length;    /* Number of characters to draw */
};

struct pHidd_GC_Clear
{
    MethodID    mID;
};

struct pHidd_GC_ReadPixelArray
{
    MethodID mID;
    ULONG	*pixelArray;
    WORD	x, y;
    WORD	width, height;
};

struct pHidd_GC_WritePixelArray
{
    MethodID mID;
    ULONG 	*pixelArray;
    WORD	x, y;
    WORD	width, height;
};


/* Predeclarations of stubs in libhiddgraphicsstubs.h */

Object * HIDD_Gfx_NewGC        (Object *hiddGfx, ULONG gcType, struct TagItem *tagList);
VOID     HIDD_Gfx_DisposeGC    (Object *hiddGfx, Object *gc);
Object * HIDD_Gfx_NewBitMap    (Object *hiddGfx, struct TagItem *tagList);
VOID     HIDD_Gfx_DisposeBitMap(Object *hiddGfx, Object *bitMap);


VOID     HIDD_BM_BltBitMap   (Object obj, Object dest, WORD srcX, WORD srcY, WORD destX, WORD destY, WORD width, WORD height);
BOOL     HIDD_BM_Show        (Object obj);
VOID     HIDD_BM_Move        (Object obj, WORD x, WORD y);
BOOL     HIDD_BM_DepthArrange(Object obj, Object bm);
BOOL	 HIDD_BM_SetColors	(Object *obj, HIDDT_Color *tab, ULONG firstcolor, ULONG numcolors);

ULONG    HIDD_GC_WritePixelDirect(Object *obj, WORD x, WORD y, ULONG val);
ULONG    HIDD_GC_ReadPixel       (Object *obj, WORD x, WORD y);
ULONG    HIDD_GC_WritePixel      (Object *obj, WORD x, WORD y);
VOID     HIDD_GC_CopyArea        (Object *obj, WORD srcX, WORD srcY, Object *dest, WORD destX, WORD destY, UWORD width, UWORD height);
VOID     HIDD_GC_DrawLine        (Object *obj, WORD x1, WORD y1, WORD x2, WORD y2);
VOID     HIDD_GC_DrawRect        (Object *obj, WORD minX, WORD minY, WORD maxX, WORD maxY);
VOID     HIDD_GC_FillRect        (Object *obj, WORD minX, WORD minY, WORD maxX, WORD maxY);
VOID     HIDD_GC_DrawEllipse     (Object *obj, WORD x, WORD y, WORD ry, WORD rx);
VOID     HIDD_GC_FillEllipse     (Object *obj, WORD x, WORD y, WORD ry, WORD rx);
VOID     HIDD_GC_DrawArc         (Object *obj);
VOID     HIDD_GC_FillArc         (Object *obj);
VOID     HIDD_GC_DrawPolygon     (Object *obj, UWORD n, WORD *coords);
VOID     HIDD_GC_FillPolygon     (Object *obj, UWORD n, WORD *coords);
VOID     HIDD_GC_DrawText        (Object *obj, WORD x, WORD y, STRPTR text, UWORD length);
VOID     HIDD_GC_FillText        (Object *obj, WORD x, WORD y, STRPTR text, UWORD length);
VOID     HIDD_GC_FillSpan        (Object *obj);
VOID     HIDD_GC_Clear           (Object *obj);

VOID     HIDD_GC_ReadPixelArray	 (Object *obj, ULONG *pixelArray, WORD x, WORD y, WORD width, WORD height);
VOID	 HIDD_GC_WritePixelArray (Object *obj, ULONG *pixelArray, WORD x, WORD y, WORD width, WORD height);

#endif /* HIDD_GRAPHICS_H */
