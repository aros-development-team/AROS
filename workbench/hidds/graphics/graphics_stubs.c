/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Stubs for graphics, bitmap, gc and colormap hidd class
    Lang: english
*/

#ifndef AROS_USE_OOP
#   define AROS_USE_OOP
#endif

#include <aros/config.h>
#include <exec/types.h>
#include <exec/libraries.h>

#include <proto/oop.h>

#include <utility/tagitem.h>

#include <oop/oop.h>
#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef OOPBase
#define OOPBase ((struct Library *)OCLASS(OCLASS(OCLASS(obj)))->UserData)


/* A small utility function for using varargs when setting attrs */

#warning SetAttrsTags is defined in inline/oop.h

#ifndef SetAttrsTags
IPTR SetAttrsTags(Object *obj, IPTR tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SetAttrs(obj, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST

}
#endif

/***************************************************************/

Object * HIDD_Gfx_NewGC(Object *obj, struct TagItem *tagList)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_NewGC p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewGC);
        
    p.mID      = mid;
    p.attrList = tagList;

    return((Object *) DoMethod(obj, (Msg) &p));
}
/***************************************************************/

void HIDD_Gfx_DisposeGC(Object *obj, Object *gc)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_DisposeGC p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeGC);
        
    p.mID    = mid;
    p.gc     = gc;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

Object * HIDD_Gfx_NewBitMap(Object *obj, struct TagItem *tagList)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_NewBitMap p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewBitMap);
        
    p.mID      = mid;
    p.attrList = tagList;

    return((Object *) DoMethod(obj, (Msg) &p));
}
/***************************************************************/

void HIDD_Gfx_DisposeBitMap(Object *obj, Object *bitMap)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_DisposeBitMap p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeBitMap);
        
    p.mID    = mid;
    p.bitMap = bitMap;

    DoMethod(obj, (Msg) &p);
}

/***************************************************************/

HIDDT_ModeID * HIDD_Gfx_QueryModeIDs(Object *obj, struct TagItem *queryTags)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_QueryModeIDs p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_QueryModeIDs);
        
    p.mID	= mid;
    p.queryTags	= queryTags;

    return (HIDDT_ModeID *)DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_Gfx_ReleaseModeIDs(Object *obj, HIDDT_ModeID *modeIDs)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_ReleaseModeIDs p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_ReleaseModeIDs);
        
    p.mID	= mid;
    p.modeIDs	= modeIDs;

    DoMethod(obj, (Msg) &p);
}


/***************************************************************/
Object *    HIDD_Gfx_GetPixFmt  (Object *obj, HIDDT_StdPixFmt stdPixFmt)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_GetPixFmt p;

    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetPixFmt);
        
    p.mID = mid;
    
    p.stdPixFmt		= stdPixFmt;
    
    return (Object *)DoMethod(obj, (Msg) &p);
}

/***************************************************************/

BOOL HIDD_Gfx_CheckMode(Object *obj, HIDDT_ModeID modeID, Object *sync, Object *pixFmt)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_CheckMode p;

    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_CheckMode);
        
    p.mID = mid;
    
    p.modeID	= modeID;
    p.sync	= sync;
    p.pixFmt	= pixFmt;
    
    return (BOOL)DoMethod(obj, (Msg) &p);
}


/***************************************************************/
BOOL HIDD_Gfx_GetMode(Object *obj, HIDDT_ModeID modeID, Object **syncPtr, Object **pixFmtPtr)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_GetMode p;

    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetMode);
        
    p.mID = mid;
    
    p.modeID	= modeID;
    p.syncPtr	= syncPtr;
    p.pixFmtPtr	= pixFmtPtr;

    return (BOOL)DoMethod(obj, (Msg) &p);
    
}

/***************************************************************/
HIDDT_ModeID HIDD_Gfx_NextModeID(Object *obj, HIDDT_ModeID modeID, Object **syncPtr, Object **pixFmtPtr)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_NextModeID p;

    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NextModeID);
        
    p.mID = mid;
    
    p.modeID	= modeID;
    p.syncPtr	= syncPtr;
    p.pixFmtPtr	= pixFmtPtr;

    return (HIDDT_ModeID)DoMethod(obj, (Msg) &p);
    
}

/***************************************************************/

BOOL HIDD_Gfx_SetCursorShape(Object *obj, Object *shape)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_SetCursorShape p;

    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetCursorShape);
        
    p.mID = mid;
    p.shape = shape;

    return (BOOL)DoMethod(obj, (Msg) &p);
    
}

/***************************************************************/

BOOL HIDD_Gfx_SetCursorPos(Object *obj, LONG x, LONG y)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_SetCursorPos p;

    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetCursorPos);
        
    p.mID = mid;
    
    p.x = x;
    p.y = y;
    
    return (BOOL)DoMethod(obj, (Msg) &p);
    
}

/***************************************************************/

VOID HIDD_Gfx_SetCursorVisible(Object *obj, BOOL visible)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_SetCursorVisible p;

    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetCursorVisible);
        
    p.mID = mid;
    
    p.visible = visible;

    DoMethod(obj, (Msg) &p);
    
    return;
    
}

/***************************************************************/

BOOL HIDD_Gfx_SetMode(Object *obj, HIDDT_ModeID modeID)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_SetMode p;

    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetMode);
        
    p.mID = mid;
    
    p.modeID = modeID;

    return (BOOL)DoMethod(obj, (Msg) &p);
    
}


/***************************************************************/

Object *HIDD_Gfx_Show(Object *obj, Object *bitMap, ULONG flags)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_Show p;

    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_Show);
        
    p.mID = mid;
    
    p.bitMap	= bitMap;
    p.flags	= flags;

    return (Object *)DoMethod(obj, (Msg) &p);
    
}

/***************************************************************/
VOID HIDD_Gfx_CopyBox(Object *obj, Object *src, WORD srcX, WORD srcY, Object *dest, WORD destX, WORD destY, UWORD width, UWORD height, Object *gc)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_CopyBox p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_Gfx_CopyBox);
        
    p.mID    = mid;
    p.src    = src;
    p.srcX   = srcX;
    p.srcY   = srcY;
    p.dest   = dest;
    p.destX  = destX;
    p.destY  = destY;
    p.width  = width;
    p.height = height;
    p.gc     = gc;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

BOOL HIDD_BM_SetColors (Object *obj, HIDDT_Color *colors, ULONG firstColor, ULONG numColors)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_SetColors p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_SetColors);
        
    p.mID        = mid;
    p.colors     = colors;
    p.firstColor = firstColor;
    p.numColors  = numColors;

    return DoMethod(obj, (Msg) &p);
}


/***************************************************************/

ULONG HIDD_BM_PutPixel(Object *obj, WORD x, WORD y, ULONG val)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_PutPixel p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutPixel);
        
    p.mID  = mid;
    p.x    = x;
    p.y    = y;
    p.pixel  = val;

    return(DoMethod(obj, (Msg) &p));
}
/***************************************************************/

ULONG HIDD_BM_DrawPixel(Object *obj, Object *gc, WORD x, WORD y)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawPixel p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPixel);
        
    p.mID  = mid;
    p.gc   = gc;
    p.x    = x;
    p.y    = y;

    return(DoMethod(obj, (Msg) &p));
}
/***************************************************************/

ULONG HIDD_BM_GetPixel(Object *obj, WORD x, WORD y)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_GetPixel p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetPixel);
        
    p.mID  = mid;
    p.x    = x;
    p.y    = y;

    return(DoMethod(obj, (Msg) &p));
}
/***************************************************************/

VOID HIDD_BM_DrawLine(Object *obj, Object *gc, WORD x1, WORD y1, WORD x2, WORD y2)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawLine p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawLine);
        
    p.mID   = mid;
    p.gc    = gc;
    p.x1    = x1;
    p.y1    = y1;
    p.x2    = x2;
    p.y2    = y2;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/


VOID HIDD_BM_DrawRect (Object *obj, Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawRect p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawRect);
        
    p.mID    = mid;
    p.gc     = gc;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillRect (Object *obj, Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawRect p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillRect);
        
    p.mID    = mid;
    p.gc     = gc;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawEllipse (Object *obj, Object *gc, WORD x, WORD y, WORD rx, WORD ry)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawEllipse p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawEllipse);
        
    p.mID    = mid;
    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillEllipse (Object *obj, Object *gc, WORD x, WORD y, WORD ry, WORD rx)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawEllipse p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillEllipse);
        
    p.mID    = mid;
    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawPolygon (Object *obj, Object *gc, UWORD n, WORD *coords)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawPolygon p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPolygon);
        
    p.mID    = mid;
    p.gc     = gc;
    p.n      = n;
    p.coords = coords;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillPolygon (Object *obj, Object *gc, UWORD n, WORD *coords)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawPolygon p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillPolygon);
        
    p.mID    = mid;
    p.gc     = gc;
    p.n      = n;
    p.coords = coords;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawText (Object *obj, Object *gc, WORD x, WORD y, STRPTR text, UWORD length)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawText p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawText);
        
    p.mID    = mid;
    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillText (Object *obj, Object *gc, WORD x, WORD y, STRPTR text, UWORD length)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawText p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillText);
        
    p.mID    = mid;
    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_Clear (Object *obj, Object *gc)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_Clear p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_Clear);
        
    p.mID    = mid;
    p.gc     = gc;

    DoMethod(obj, (Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_GetImage  (Object *obj
	, UBYTE *pixels
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height
	, HIDDT_StdPixFmt pixFmt)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_GetImage p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetImage);
        
    p.mID    = mid;
    p.pixels = pixels;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    
    p.pixFmt = pixFmt;
    
    

    DoMethod(obj, (Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_PutImage  (Object *obj
	, Object *gc
	, UBYTE *pixels
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height
	, HIDDT_StdPixFmt pixFmt)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_PutImage p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutImage);
        
    p.mID    = mid;
    p.gc     = gc;
    p.pixels = pixels;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    p.pixFmt = pixFmt;

    DoMethod(obj, (Msg) &p);
}

/***************************************************************/

VOID	 HIDD_BM_BlitColorExpansion	 (Object *obj, Object *gc, Object *srcBitMap, WORD srcX, WORD srcY, WORD destX, WORD destY,  UWORD width, UWORD height)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_BlitColorExpansion p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_BlitColorExpansion);
        
    p.mID	= mid;
    p.gc	= gc;
    p.srcBitMap	= srcBitMap;
    p.srcX	= srcX;
    p.srcY	= srcY;
    p.destX 	= destX;
    p.destY	= destY;
    p.width	= width;
    p.height	= height;

    DoMethod(obj, (Msg) &p);
}

/***************************************************************/

HIDDT_Pixel HIDD_BM_MapColor(Object *obj, HIDDT_Color *color)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_MapColor p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_MapColor);
    
    p.mID	= mid;
    p.color	= color;
 
    
    return DoMethod(obj, (Msg)&p);
}

/***************************************************************/

VOID HIDD_BM_UnmapPixel(Object *obj, HIDDT_Pixel pixel, HIDDT_Color *color)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_UnmapPixel p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_UnmapPixel);
    
    p.mID	= mid;
    p.pixel	= pixel;
    p.color	= color;

    DoMethod(obj, (Msg)&p);
}


/***************************************************************/

VOID     HIDD_BM_PutImageLUT  (Object *obj, Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_PutImageLUT p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutImageLUT);
        
    p.mID	= mid;
    p.gc	= gc;
    p.pixels	= pixels;
    p.modulo	= modulo;
    p.x		= x;
    p.y		= y;
    p.width	= width;
    p.height	= height;
    p.pixlut	= pixlut;

    DoMethod(obj, (Msg) &p);
}


/***************************************************************/

VOID     HIDD_BM_GetImageLUT  (Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_GetImageLUT p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetImageLUT);
        
    p.mID	= mid;
    p.pixels	= pixels;
    p.modulo	= modulo;
    p.x		= x;
    p.y		= y;
    p.width	= width;
    p.height	= height;
    p.pixlut	= pixlut;

    DoMethod(obj, (Msg) &p);
}


ULONG HIDD_BM_BytesPerLine(Object *obj, HIDDT_StdPixFmt pixFmt, ULONG width)
{
     static MethodID mid = 0;
     struct pHidd_BitMap_BytesPerLine p;
     
     if (!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_BytesPerLine);
     
     p.mID = mid;
     p.pixFmt	= pixFmt;
     p.width	= width;
     
     return DoMethod(obj, (Msg) &p);
     
}

/***************************************************************/


VOID     HIDD_BM_ConvertPixels  (Object *obj
	, APTR *srcPixels
	, HIDDT_PixelFormat *srcPixFmt
	, ULONG srcMod
	, APTR *dstBuf
	, HIDDT_PixelFormat *dstPixFmt
	, ULONG dstMod
	, ULONG width, ULONG height
	, HIDDT_PixelLUT *pixlut
)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_ConvertPixels p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_ConvertPixels);
        
    p.mID = mid;
    p.srcPixFmt = srcPixFmt;
    p.srcPixels = srcPixels;
    
    p.srcMod	= srcMod;
    
    p.dstBuf	= dstBuf;
    p.dstPixFmt	= dstPixFmt;
    
    p.dstMod	= dstMod;
    
    p.width	= width;
    p.height	= height;
    
    p.pixlut	= pixlut;
    
    DoMethod(obj, (Msg) &p);
}


Object * HIDD_BM_SetColorMap(Object *obj, Object *colorMap)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_SetColorMap p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_SetColorMap);
        
    p.mID = mid;
    p.colorMap = colorMap;
    
    return (Object *)DoMethod(obj, (Msg)&p);
}


BOOL HIDD_BM_ObtainDirectAccess(Object *obj
	, UBYTE **addressReturn
	, ULONG *widthReturn
	, ULONG *heightReturn
	, ULONG *bankSizeReturn
	, ULONG *memSizeReturn )
{
    static MethodID mid = 0;
    struct pHidd_BitMap_ObtainDirectAccess p;

    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_ObtainDirectAccess);
        
    p.mID = mid;
    p.addressReturn	= addressReturn;
    p.widthReturn	= widthReturn;
    p.heightReturn	= heightReturn;
    p.bankSizeReturn	= bankSizeReturn;
    p.memSizeReturn	= memSizeReturn;
    
    /* Clear this by default */
    *addressReturn = NULL;
    
    return (BOOL)DoMethod(obj, (Msg)&p);
}

VOID HIDD_BM_ReleaseDirectAccess(Object *obj)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_ReleaseDirectAccess p;

    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_ReleaseDirectAccess);
        
    p.mID = mid;
    
    DoMethod(obj, (Msg)&p);
	
    return;
}

/********* GC *****************************************/
VOID HIDD_GC_SetClipRect(Object *obj, LONG x1, LONG y1, LONG x2, LONG y2)
{
    static MethodID mid = 0;
    struct pHidd_GC_SetClipRect p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_GC, moHidd_GC_SetClipRect);
        
    p.mID	= mid;
    p.x1	= x1;
    p.y1	= y1;
    p.x2	= x2;
    p.y2	= y2;
    
    DoMethod(obj, (Msg)&p);
    
}

VOID HIDD_GC_UnsetClipRect(Object *obj)
{
    static MethodID mid = 0;
    struct pHidd_GC_UnsetClipRect p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_GC, moHidd_GC_UnsetClipRect);
        
    p.mID	= mid;
    
    DoMethod(obj, (Msg)&p);
}

/********* PlanarBM **********************************/
BOOL HIDD_PlanarBM_SetBitMap(Object *obj, struct BitMap *bitMap)
{
    static MethodID mid = 0;
    struct pHidd_PlanarBM_SetBitMap p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_PlanarBM, moHidd_PlanarBM_SetBitMap);
        
    p.mID = mid;
    p.bitMap = bitMap;
    
    return (BOOL)DoMethod(obj, (Msg)&p);
}


/********* ColorMap *********************************/

BOOL HIDD_CM_SetColors(Object *obj, HIDDT_Color *colors, ULONG firstColor, ULONG numColors, Object *pixFmt)
{
    static MethodID mid = 0;
    struct pHidd_ColorMap_SetColors p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_ColorMap, moHidd_ColorMap_SetColors);
        
    p.mID = mid;
    p.colors	 = colors;
    p.firstColor = firstColor;
    p.numColors	 = numColors;
    p.pixFmt	 = pixFmt;
    
    return DoMethod(obj, (Msg)&p);
}

HIDDT_Pixel HIDD_CM_GetPixel(Object *obj, ULONG pixelNo) /* Starts at 0 */
{
    static MethodID mid = 0;
    struct pHidd_ColorMap_GetPixel p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_ColorMap, moHidd_ColorMap_GetPixel);
        
    p.mID = mid;
    p.pixelNo = pixelNo;
    
    return (HIDDT_Pixel)DoMethod(obj, (Msg)&p);
}


BOOL HIDD_CM_GetColor(Object *obj, ULONG colorNo, HIDDT_Color *colorReturn) /* Starts at 0 */
{
    static MethodID mid = 0;
    struct pHidd_ColorMap_GetColor p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_ColorMap, moHidd_ColorMap_GetColor);
        
    p.mID = mid;
    p.colorNo	  = colorNo;
    p.colorReturn = colorReturn;
    
    return (BOOL)DoMethod(obj, (Msg)&p);
}
