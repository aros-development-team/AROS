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

ULONG HIDD_BM_DrawPixel(Object *obj, WORD x, WORD y)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawPixel p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPixel);
        
    p.mID  = mid;
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

VOID HIDD_BM_DrawLine(Object *obj, WORD x1, WORD y1, WORD x2, WORD y2)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawLine p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawLine);
        
    p.mID  = mid;
    p.x1    = x1;
    p.y1    = y1;
    p.x2    = x2;
    p.y2    = y2;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_CopyBox(Object *obj, WORD srcX, WORD srcY, Object *dest, WORD destX, WORD destY, UWORD width, UWORD height)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_CopyBox p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyBox);
        
    p.mID    = mid;
    p.srcX   = srcX;
    p.srcY   = srcY;
    p.dest   = dest;
    p.destX  = destX;
    p.destY  = destY;
    p.width  = width;
    p.height = height;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawRect (Object *obj, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawRect p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawRect);
        
    p.mID    = mid;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillRect (Object *obj, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawRect p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillRect);
        
    p.mID    = mid;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawEllipse (Object *obj, WORD x, WORD y, WORD ry, WORD rx)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawEllipse p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawEllipse);
        
    p.mID    = mid;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillEllipse (Object *obj, WORD x, WORD y, WORD ry, WORD rx)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawEllipse p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillEllipse);
        
    p.mID    = mid;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawPolygon (Object *obj, UWORD n, WORD *coords)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawPolygon p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPolygon);
        
    p.mID    = mid;
    p.n      = n;
    p.coords = coords;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillPolygon (Object *obj, UWORD n, WORD *coords)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawPolygon p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillPolygon);
        
    p.mID    = mid;
    p.n      = n;
    p.coords = coords;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawText (Object *obj, WORD x, WORD y, STRPTR text, UWORD length)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawText p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawText);
        
    p.mID    = mid;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillText (Object *obj, WORD x, WORD y, STRPTR text, UWORD length)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_DrawText p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillText);
        
    p.mID    = mid;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    DoMethod(obj, (Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_Clear (Object *obj)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_Clear p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_Clear);
        
    p.mID    = mid;

    DoMethod(obj, (Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_GetImage  (Object *obj, ULONG *pixels, WORD x, WORD y, WORD width, WORD height)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_GetImage p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetImage);
        
    p.mID    = mid;
    p.pixels = pixels;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    

    DoMethod(obj, (Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_PutImage  (Object *obj, ULONG *pixels, WORD x, WORD y, WORD width, WORD height)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_PutImage p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutImage);
        
    p.mID    = mid;
    p.pixels = pixels;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;

    DoMethod(obj, (Msg) &p);
}

/***************************************************************/

VOID	 HIDD_BM_BlitColorExpansion	 (Object *obj, Object *srcBitMap, WORD srcX, WORD srcY, WORD destX, WORD destY,  UWORD width, UWORD height)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_BlitColorExpansion p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_BlitColorExpansion);
        
    p.mID	= mid;
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

VOID     HIDD_BM_PutImageLUT  (Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    static MethodID mid = 0;
    struct pHidd_BitMap_PutImageLUT p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutImageLUT);
        
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

