/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#define OOPBase ((struct Library *)OOP_OCLASS(obj)->OOPBasePtr)

#ifndef AROS_CREATE_ROM
#  define STATIC_MID static OOP_MethodID mid
#else
#  define STATIC_MID OOP_MethodID mid = 0
#endif

/* A small utility function for using varargs when setting attrs */

#warning OOP_SetAttrsTags is defined in inline/oop.h

#ifndef OOP_SetAttrsTags
IPTR OOP_SetAttrsTags(OOP_Object *obj, IPTR tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = OOP_SetAttrs(obj, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST

}
#endif

/***************************************************************/

OOP_Object * HIDD_Gfx_NewGC(OOP_Object *obj, struct TagItem *tagList)
{
    STATIC_MID;
    struct pHidd_Gfx_NewGC p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewGC);
        
    p.mID      = mid;
    p.attrList = tagList;

    return((OOP_Object *) OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

void HIDD_Gfx_DisposeGC(OOP_Object *obj, OOP_Object *gc)
{
    STATIC_MID;
    struct pHidd_Gfx_DisposeGC p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeGC);
        
    p.mID    = mid;
    p.gc     = gc;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

OOP_Object * HIDD_Gfx_NewBitMap(OOP_Object *obj, struct TagItem *tagList)
{
    STATIC_MID;
    struct pHidd_Gfx_NewBitMap p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewBitMap);
        
    p.mID      = mid;
    p.attrList = tagList;

    return((OOP_Object *) OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

void HIDD_Gfx_DisposeBitMap(OOP_Object *obj, OOP_Object *bitMap)
{
    STATIC_MID;
    struct pHidd_Gfx_DisposeBitMap p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeBitMap);
        
    p.mID    = mid;
    p.bitMap = bitMap;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

HIDDT_ModeID * HIDD_Gfx_QueryModeIDs(OOP_Object *obj, struct TagItem *queryTags)
{
    STATIC_MID;
    struct pHidd_Gfx_QueryModeIDs p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_QueryModeIDs);
        
    p.mID	= mid;
    p.queryTags	= queryTags;

    return (HIDDT_ModeID *)OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_Gfx_ReleaseModeIDs(OOP_Object *obj, HIDDT_ModeID *modeIDs)
{
    STATIC_MID;
    struct pHidd_Gfx_ReleaseModeIDs p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_ReleaseModeIDs);
        
    p.mID	= mid;
    p.modeIDs	= modeIDs;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}


/***************************************************************/
OOP_Object *    HIDD_Gfx_GetPixFmt  (OOP_Object *obj, HIDDT_StdPixFmt stdPixFmt)
{
    STATIC_MID;
    struct pHidd_Gfx_GetPixFmt p;

    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetPixFmt);
        
    p.mID = mid;
    
    p.stdPixFmt		= stdPixFmt;
    
    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

BOOL HIDD_Gfx_CheckMode(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object *sync, OOP_Object *pixFmt)
{
    STATIC_MID;
    struct pHidd_Gfx_CheckMode p;

    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_CheckMode);
        
    p.mID = mid;
    
    p.modeID	= modeID;
    p.sync	= sync;
    p.pixFmt	= pixFmt;
    
    return (BOOL)OOP_DoMethod(obj, (OOP_Msg) &p);
}


/***************************************************************/
BOOL HIDD_Gfx_GetMode(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object **syncPtr, OOP_Object **pixFmtPtr)
{
    STATIC_MID;
    struct pHidd_Gfx_GetMode p;

    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetMode);
        
    p.mID = mid;
    
    p.modeID	= modeID;
    p.syncPtr	= syncPtr;
    p.pixFmtPtr	= pixFmtPtr;

    return (BOOL)OOP_DoMethod(obj, (OOP_Msg) &p);
    
}

/***************************************************************/
HIDDT_ModeID HIDD_Gfx_NextModeID(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object **syncPtr, OOP_Object **pixFmtPtr)
{
    STATIC_MID;
    struct pHidd_Gfx_NextModeID p;

    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NextModeID);
        
    p.mID = mid;
    
    p.modeID	= modeID;
    p.syncPtr	= syncPtr;
    p.pixFmtPtr	= pixFmtPtr;

    return (HIDDT_ModeID)OOP_DoMethod(obj, (OOP_Msg) &p);
    
}

/***************************************************************/

BOOL HIDD_Gfx_SetCursorShape(OOP_Object *obj, OOP_Object *shape)
{
    STATIC_MID;
    struct pHidd_Gfx_SetCursorShape p;

    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetCursorShape);
        
    p.mID = mid;
    p.shape = shape;

    return (BOOL)OOP_DoMethod(obj, (OOP_Msg) &p);
    
}

/***************************************************************/

BOOL HIDD_Gfx_SetCursorPos(OOP_Object *obj, LONG x, LONG y)
{
    STATIC_MID;
    struct pHidd_Gfx_SetCursorPos p;

    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetCursorPos);
        
    p.mID = mid;
    
    p.x = x;
    p.y = y;
    
    return (BOOL)OOP_DoMethod(obj, (OOP_Msg) &p);
    
}

/***************************************************************/

VOID HIDD_Gfx_SetCursorVisible(OOP_Object *obj, BOOL visible)
{
    STATIC_MID;
    struct pHidd_Gfx_SetCursorVisible p;

    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetCursorVisible);
        
    p.mID = mid;
    
    p.visible = visible;

    OOP_DoMethod(obj, (OOP_Msg) &p);
    
    return;
    
}

/***************************************************************/

BOOL HIDD_Gfx_SetMode(OOP_Object *obj, HIDDT_ModeID modeID)
{
    STATIC_MID;
    struct pHidd_Gfx_SetMode p;

    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetMode);
        
    p.mID = mid;
    
    p.modeID = modeID;

    return (BOOL)OOP_DoMethod(obj, (OOP_Msg) &p);
    
}


/***************************************************************/

OOP_Object *HIDD_Gfx_Show(OOP_Object *obj, OOP_Object *bitMap, ULONG flags)
{
    STATIC_MID;
    struct pHidd_Gfx_Show p;

    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_Show);
        
    p.mID = mid;
    
    p.bitMap	= bitMap;
    p.flags	= flags;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) &p);
    
}

/***************************************************************/
VOID HIDD_Gfx_CopyBox(OOP_Object *obj, OOP_Object *src, WORD srcX, WORD srcY, OOP_Object *dest, WORD destX, WORD destY, UWORD width, UWORD height, OOP_Object *gc)
{
    STATIC_MID;
    struct pHidd_Gfx_CopyBox p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_Gfx_CopyBox);
        
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

    OOP_DoMethod(obj, (OOP_Msg) &p);
}


/***************************************************************/

BOOL HIDD_BM_SetColors (OOP_Object *obj, HIDDT_Color *colors, ULONG firstColor, ULONG numColors)
{
    STATIC_MID;
    struct pHidd_BitMap_SetColors p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_SetColors);
        
    p.mID        = mid;
    p.colors     = colors;
    p.firstColor = firstColor;
    p.numColors  = numColors;

    return OOP_DoMethod(obj, (OOP_Msg) &p);
}


/***************************************************************/

ULONG HIDD_BM_PutPixel(OOP_Object *obj, WORD x, WORD y, ULONG val)
{
    STATIC_MID;
    struct pHidd_BitMap_PutPixel p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutPixel);
        
    p.mID  = mid;
    p.x    = x;
    p.y    = y;
    p.pixel  = val;

    return(OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

ULONG HIDD_BM_DrawPixel(OOP_Object *obj, OOP_Object *gc, WORD x, WORD y)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawPixel p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPixel);
        
    p.mID  = mid;
    p.gc   = gc;
    p.x    = x;
    p.y    = y;

    return(OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

ULONG HIDD_BM_GetPixel(OOP_Object *obj, WORD x, WORD y)
{
    STATIC_MID;
    struct pHidd_BitMap_GetPixel p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetPixel);
        
    p.mID  = mid;
    p.x    = x;
    p.y    = y;

    return(OOP_DoMethod(obj, (OOP_Msg) &p));
}
/***************************************************************/

VOID HIDD_BM_DrawLine(OOP_Object *obj, OOP_Object *gc, WORD x1, WORD y1, WORD x2, WORD y2)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawLine p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawLine);
        
    p.mID   = mid;
    p.gc    = gc;
    p.x1    = x1;
    p.y1    = y1;
    p.x2    = x2;
    p.y2    = y2;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/


VOID HIDD_BM_DrawRect (OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawRect p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawRect);
        
    p.mID    = mid;
    p.gc     = gc;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillRect (OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawRect p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillRect);
        
    p.mID    = mid;
    p.gc     = gc;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawEllipse (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, WORD rx, WORD ry)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawEllipse p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawEllipse);
        
    p.mID    = mid;
    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillEllipse (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, WORD ry, WORD rx)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawEllipse p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillEllipse);
        
    p.mID    = mid;
    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawPolygon (OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawPolygon p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawPolygon);
        
    p.mID    = mid;
    p.gc     = gc;
    p.n      = n;
    p.coords = coords;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillPolygon (OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawPolygon p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillPolygon);
        
    p.mID    = mid;
    p.gc     = gc;
    p.n      = n;
    p.coords = coords;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_DrawText (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, STRPTR text, UWORD length)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawText p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_DrawText);
        
    p.mID    = mid;
    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_FillText (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, STRPTR text, UWORD length)
{
    STATIC_MID;
    struct pHidd_BitMap_DrawText p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillText);
        
    p.mID    = mid;
    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
/***************************************************************/

VOID HIDD_BM_Clear (OOP_Object *obj, OOP_Object *gc)
{
    STATIC_MID;
    struct pHidd_BitMap_Clear p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_Clear);
        
    p.mID    = mid;
    p.gc     = gc;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_GetImage  (OOP_Object *obj
	, UBYTE *pixels
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height
	, HIDDT_StdPixFmt pixFmt)
{
    STATIC_MID;
    struct pHidd_BitMap_GetImage p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetImage);
        
    p.mID    = mid;
    p.pixels = pixels;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    
    p.pixFmt = pixFmt;
    
    

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_PutImage  (OOP_Object *obj
	, OOP_Object *gc
	, UBYTE *pixels
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height
	, HIDDT_StdPixFmt pixFmt)
{
    STATIC_MID;
    struct pHidd_BitMap_PutImage p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutImage);
        
    p.mID    = mid;
    p.gc     = gc;
    p.pixels = pixels;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    p.pixFmt = pixFmt;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_PutAlphaImage  (OOP_Object *obj
	, OOP_Object *gc
	, UBYTE *pixels
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height)
{
    STATIC_MID;
    struct pHidd_BitMap_PutAlphaImage p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutAlphaImage);
        
    p.mID    = mid;
    p.gc     = gc;
    p.pixels = pixels;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_PutTemplate  (OOP_Object *obj
	, OOP_Object *gc
	, UBYTE *template
	, ULONG modulo
	, WORD srcx
	, WORD x, WORD y
	, WORD width, WORD height
	, BOOL inverttemplate)
{
    STATIC_MID;
    struct pHidd_BitMap_PutTemplate p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutTemplate);
        
    p.mID    	= mid;
    p.gc     	= gc;
    p.template  = template;
    p.modulo 	= modulo;
    p.srcx   	= srcx;
    p.x      	= x;
    p.y      	= y;
    p.width  	= width;
    p.height 	= height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID     HIDD_BM_PutAlphaTemplate  (OOP_Object *obj
	, OOP_Object *gc
	, UBYTE *alpha
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height
	, BOOL invertalpha)
{
    STATIC_MID;
    struct pHidd_BitMap_PutAlphaTemplate p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutAlphaTemplate);
        
    p.mID    = mid;
    p.gc     = gc;
    p.alpha  = alpha;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    p.invertalpha = invertalpha;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID HIDD_BM_PutPattern(OOP_Object *obj, OOP_Object *gc, UBYTE *pattern,
    	    	    	WORD patternsrcx, WORD patternsrcy, WORD patternheight,
			WORD patterndepth, HIDDT_PixelLUT *patternlut,
			BOOL invertpattern, UBYTE *mask, ULONG maskmodulo,
			WORD masksrcx, WORD x, WORD y, WORD width, WORD height)
{
    STATIC_MID;
    struct pHidd_BitMap_PutPattern p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutPattern);
        
    p.mID    	    = mid;
    p.gc     	    = gc;
    p.pattern	    = pattern;
    p.patternsrcx   = patternsrcx;
    p.patternsrcy   = patternsrcy;
    p.patternheight = patternheight;
    p.patterndepth  = patterndepth;
    p.patternlut    = patternlut;
    p.invertpattern = invertpattern;
    p.mask  	    = mask;
    p.maskmodulo    = maskmodulo;
    p.masksrcx      = masksrcx;
    p.x      	    = x;
    p.y      	    = y;
    p.width  	    = width;
    p.height 	    = height;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID	 HIDD_BM_BlitColorExpansion	 (OOP_Object *obj, OOP_Object *gc, OOP_Object *srcBitMap, WORD srcX, WORD srcY, WORD destX, WORD destY,  UWORD width, UWORD height)
{
    STATIC_MID;
    struct pHidd_BitMap_BlitColorExpansion p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_BlitColorExpansion);
        
    p.mID	= mid;
    p.gc	= gc;
    p.srcBitMap	= srcBitMap;
    p.srcX	= srcX;
    p.srcY	= srcY;
    p.destX 	= destX;
    p.destY	= destY;
    p.width	= width;
    p.height	= height;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

HIDDT_Pixel HIDD_BM_MapColor(OOP_Object *obj, HIDDT_Color *color)
{
    STATIC_MID;
    struct pHidd_BitMap_MapColor p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_MapColor);
    
    p.mID	= mid;
    p.color	= color;
 
    
    return OOP_DoMethod(obj, (OOP_Msg)&p);
}

/***************************************************************/

VOID HIDD_BM_UnmapPixel(OOP_Object *obj, HIDDT_Pixel pixel, HIDDT_Color *color)
{
    STATIC_MID;
    struct pHidd_BitMap_UnmapPixel p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_UnmapPixel);
    
    p.mID	= mid;
    p.pixel	= pixel;
    p.color	= color;

    OOP_DoMethod(obj, (OOP_Msg)&p);
}


/***************************************************************/

VOID     HIDD_BM_PutImageLUT  (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    STATIC_MID;
    struct pHidd_BitMap_PutImageLUT p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutImageLUT);
        
    p.mID	= mid;
    p.gc	= gc;
    p.pixels	= pixels;
    p.modulo	= modulo;
    p.x		= x;
    p.y		= y;
    p.width	= width;
    p.height	= height;
    p.pixlut	= pixlut;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/***************************************************************/

VOID HIDD_BM_PutTranspImageLUT  (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels,
    	    	    	    	 ULONG modulo, WORD x, WORD y, WORD width, WORD height,
				 HIDDT_PixelLUT *pixlut, UBYTE transparent)
{
    STATIC_MID;
    struct pHidd_BitMap_PutTranspImageLUT p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutTranspImageLUT);
        
    p.mID	  = mid;
    p.gc	  = gc;
    p.pixels	  = pixels;
    p.modulo	  = modulo;
    p.x		  = x;
    p.y		  = y;
    p.width	  = width;
    p.height	  = height;
    p.pixlut	  = pixlut;
    p.transparent = transparent;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}


/***************************************************************/

VOID     HIDD_BM_GetImageLUT  (OOP_Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    STATIC_MID;
    struct pHidd_BitMap_GetImageLUT p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetImageLUT);
        
    p.mID	= mid;
    p.pixels	= pixels;
    p.modulo	= modulo;
    p.x		= x;
    p.y		= y;
    p.width	= width;
    p.height	= height;
    p.pixlut	= pixlut;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}


ULONG HIDD_BM_BytesPerLine(OOP_Object *obj, HIDDT_StdPixFmt pixFmt, ULONG width)
{
    STATIC_MID;
    struct pHidd_BitMap_BytesPerLine p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_BytesPerLine);

    p.mID = mid;
    p.pixFmt	= pixFmt;
    p.width	= width;

    return OOP_DoMethod(obj, (OOP_Msg) &p);
     
}

/***************************************************************/


VOID     HIDD_BM_ConvertPixels  (OOP_Object *obj
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
    STATIC_MID;
    struct pHidd_BitMap_ConvertPixels p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_ConvertPixels);
        
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
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_FillMemRect8 (OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
	, UBYTE fill
)
{
    STATIC_MID;
    struct pHidd_BitMap_FillMemRect8 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillMemRect8);
        
    p.mID = mid;
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_FillMemRect16(OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
	, UWORD fill
)
{
    STATIC_MID;
    struct pHidd_BitMap_FillMemRect16 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillMemRect16);
        
    p.mID = mid;
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_FillMemRect24 (OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
	, ULONG fill
)
{
    STATIC_MID;
    struct pHidd_BitMap_FillMemRect24 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillMemRect24);
        
    p.mID = mid;
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_FillMemRect32 (OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
	, ULONG fill
)
{
    STATIC_MID;
    struct pHidd_BitMap_FillMemRect32 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_FillMemRect32);
        
    p.mID = mid;
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}


VOID	HIDD_BM_InvertMemRect(OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
)
{
    STATIC_MID;
    struct pHidd_BitMap_InvertMemRect p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_InvertMemRect);
        
    p.mID = mid;
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}


VOID	HIDD_BM_CopyMemBox8(OOP_Object *obj
    	, APTR src
	, WORD srcX
	, WORD srcY
	, APTR dst
	, WORD dstX
	, WORD dstY
	, UWORD width
	, UWORD height
	, ULONG srcMod
	, ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_CopyMemBox8 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox8);
        
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;  
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_CopyMemBox16(OOP_Object *obj
    	, APTR src
	, WORD srcX
	, WORD srcY
	, APTR dst
	, WORD dstX
	, WORD dstY
	, UWORD width
	, UWORD height
	, ULONG srcMod
	, ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_CopyMemBox16 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox16);
        
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;   
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_CopyMemBox24(OOP_Object *obj
    	, APTR src
	, WORD srcX
	, WORD srcY
	, APTR dst
	, WORD dstX
	, WORD dstY
	, UWORD width
	, UWORD height
	, ULONG srcMod
	, ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_CopyMemBox24 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox24);
        
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;  
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_CopyMemBox32(OOP_Object *obj
    	, APTR src
	, WORD srcX
	, WORD srcY
	, APTR dst
	, WORD dstX
	, WORD dstY
	, UWORD width
	, UWORD height
	, ULONG srcMod
	, ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_CopyMemBox32 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyMemBox32);
        
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;  
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_CopyLUTMemBox16(OOP_Object *obj
    	, APTR src
	, WORD srcX
	, WORD srcY
	, APTR dst
	, WORD dstX
	, WORD dstY
	, UWORD width
	, UWORD height
	, ULONG srcMod
	, ULONG dstMod
	, HIDDT_PixelLUT *pixlut)
{
    STATIC_MID;
    struct pHidd_BitMap_CopyLUTMemBox16 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyLUTMemBox16);
        
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;   
    p.dstMod = dstMod;
    p.pixlut = pixlut;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_CopyLUTMemBox24(OOP_Object *obj
    	, APTR src
	, WORD srcX
	, WORD srcY
	, APTR dst
	, WORD dstX
	, WORD dstY
	, UWORD width
	, UWORD height
	, ULONG srcMod
	, ULONG dstMod
	, HIDDT_PixelLUT *pixlut)
{
    STATIC_MID;
    struct pHidd_BitMap_CopyLUTMemBox24 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyLUTMemBox24);
        
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;   
    p.dstMod = dstMod;
    p.pixlut = pixlut;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_CopyLUTMemBox32(OOP_Object *obj
    	, APTR src
	, WORD srcX
	, WORD srcY
	, APTR dst
	, WORD dstX
	, WORD dstY
	, UWORD width
	, UWORD height
	, ULONG srcMod
	, ULONG dstMod
	, HIDDT_PixelLUT *pixlut)
{
    STATIC_MID;
    struct pHidd_BitMap_CopyLUTMemBox32 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_CopyLUTMemBox32);
        
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;   
    p.dstMod = dstMod;
    p.pixlut = pixlut;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_PutMem32Image8(OOP_Object *obj,
    	    	    	       APTR src,
			       APTR dst,
			       WORD dstX,
			       WORD dstY,
			       UWORD width,
			       UWORD height,
			       ULONG srcMod,
			       ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMem32Image8 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image8);
    
    p.mID = mid;
    p.src = src;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_PutMem32Image16(OOP_Object *obj,
    	    	    	        APTR src,
			        APTR dst,
			        WORD dstX,
			        WORD dstY,
			        UWORD width,
			        UWORD height,
			        ULONG srcMod,
			        ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMem32Image16 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image16);
    
    p.mID = mid;
    p.src = src;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_PutMem32Image24(OOP_Object *obj,
    	    	    	        APTR src,
			        APTR dst,
			        WORD dstX,
			        WORD dstY,
			        UWORD width,
			        UWORD height,
			        ULONG srcMod,
			        ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMem32Image24 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMem32Image24);
    
    p.mID = mid;
    p.src = src;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_GetMem32Image8(OOP_Object *obj,
    	    	    	       APTR src,
			       WORD srcX,
			       WORD srcY,
			       APTR dst,
			       UWORD width,
			       UWORD height,
			       ULONG srcMod,
			       ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_GetMem32Image8 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image8);
    
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_GetMem32Image16(OOP_Object *obj,
    	    	    		APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_GetMem32Image16 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image16);
    
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_GetMem32Image24(OOP_Object *obj,
    	    	    		APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod)
{
    STATIC_MID;
    struct pHidd_BitMap_GetMem32Image24 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetMem32Image24);
    
    p.mID = mid;
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_PutMemTemplate8	(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *template,
				 ULONG modulo,
				 WORD srcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height,
				 BOOL inverttemplate)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMemTemplate8 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate8);
    
    p.mID = mid;
    p.gc = gc;
    p.template = template;
    p.modulo = modulo;
    p.srcx = srcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_PutMemTemplate16(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *template,
				 ULONG modulo,
				 WORD srcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height,
				 BOOL inverttemplate)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMemTemplate16 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate16);
    
    p.mID = mid;
    p.gc = gc;
    p.template = template;
    p.modulo = modulo;
    p.srcx = srcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_PutMemTemplate24(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *template,
				 ULONG modulo,
				 WORD srcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height,
				 BOOL inverttemplate)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMemTemplate24 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate24);
    
    p.mID = mid;
    p.gc = gc;
    p.template = template;
    p.modulo = modulo;
    p.srcx = srcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID	HIDD_BM_PutMemTemplate32(OOP_Object *obj,
    	    	    	    	 OOP_Object *gc,
				 UBYTE *template,
				 ULONG modulo,
				 WORD srcx,
				 APTR dst,
				 ULONG dstMod,
				 WORD x,
				 WORD y,
				 WORD width,
				 WORD height,
				 BOOL inverttemplate)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMemTemplate32 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate32);
    
    p.mID = mid;
    p.gc = gc;
    p.template = template;
    p.modulo = modulo;
    p.srcx = srcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);
}

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
				 WORD height)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMemPattern8 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern8);
    
    p.mID = mid;
    p.gc = gc;
    p.pattern = pattern;
    p.patternsrcx = patternsrcx;
    p.patternsrcy = patternsrcy;
    p.patternheight = patternheight;
    p.patterndepth = patterndepth;
    p.patternlut = patternlut;
    p.invertpattern = invertpattern;
    p.mask = mask;
    p.maskmodulo = maskmodulo;
    p.masksrcx = masksrcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);    
}
				 
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
				 WORD height)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMemPattern16 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern16);
    
    p.mID = mid;
    p.gc = gc;
    p.pattern = pattern;
    p.patternsrcx = patternsrcx;
    p.patternsrcy = patternsrcy;
    p.patternheight = patternheight;
    p.patterndepth = patterndepth;
    p.patternlut = patternlut;
    p.invertpattern = invertpattern;
    p.mask = mask;
    p.maskmodulo = maskmodulo;
    p.masksrcx = masksrcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);    
}

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
				 WORD height)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMemPattern24 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern24);
    
    p.mID = mid;
    p.gc = gc;
    p.pattern = pattern;
    p.patternsrcx = patternsrcx;
    p.patternsrcy = patternsrcy;
    p.patternheight = patternheight;
    p.patterndepth = patterndepth;
    p.patternlut = patternlut;
    p.invertpattern = invertpattern;
    p.mask = mask;
    p.maskmodulo = maskmodulo;
    p.masksrcx = masksrcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);    
}
	
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
				 WORD height)
{
    STATIC_MID;
    struct pHidd_BitMap_PutMemPattern32 p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutMemPattern32);
    
    p.mID = mid;
    p.gc = gc;
    p.pattern = pattern;
    p.patternsrcx = patternsrcx;
    p.patternsrcy = patternsrcy;
    p.patternheight = patternheight;
    p.patterndepth = patterndepth;
    p.patternlut = patternlut;
    p.invertpattern = invertpattern;
    p.mask = mask;
    p.maskmodulo = maskmodulo;
    p.masksrcx = masksrcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    
    OOP_DoMethod(obj, (OOP_Msg) &p);    
}
			
OOP_Object * HIDD_BM_SetColorMap(OOP_Object *obj, OOP_Object *colorMap)
{
    STATIC_MID;
    struct pHidd_BitMap_SetColorMap p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_SetColorMap);
        
    p.mID = mid;
    p.colorMap = colorMap;
    
    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg)&p);
}


BOOL HIDD_BM_ObtainDirectAccess(OOP_Object *obj
	, UBYTE **addressReturn
	, ULONG *widthReturn
	, ULONG *heightReturn
	, ULONG *bankSizeReturn
	, ULONG *memSizeReturn )
{
    STATIC_MID;
    struct pHidd_BitMap_ObtainDirectAccess p;

    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_ObtainDirectAccess);
        
    p.mID = mid;
    p.addressReturn	= addressReturn;
    p.widthReturn	= widthReturn;
    p.heightReturn	= heightReturn;
    p.bankSizeReturn	= bankSizeReturn;
    p.memSizeReturn	= memSizeReturn;
    
    /* Clear this by default */
    *addressReturn = NULL;
    
    return (BOOL)OOP_DoMethod(obj, (OOP_Msg)&p);
}

VOID HIDD_BM_ReleaseDirectAccess(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_BitMap_ReleaseDirectAccess p;

    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_ReleaseDirectAccess);
        
    p.mID = mid;
    
    OOP_DoMethod(obj, (OOP_Msg)&p);
	
    return;
}

/***************************************************************/
VOID HIDD_BM_BitMapScale(OOP_Object *obj, OOP_Object *src, OOP_Object *dest, struct BitScaleArgs * bsa, OOP_Object *gc)
{
    STATIC_MID;
    struct pHidd_BitMap_BitMapScale p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_BitMapScale);
        
    p.mID    = mid;
    p.src    = src;
    p.dst    = dest;
    p.bsa    = bsa;
    p.gc     = gc;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

/********* GC *****************************************/
VOID HIDD_GC_SetClipRect(OOP_Object *obj, LONG x1, LONG y1, LONG x2, LONG y2)
{
    STATIC_MID;
    struct pHidd_GC_SetClipRect p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_GC, moHidd_GC_SetClipRect);
        
    p.mID	= mid;
    p.x1	= x1;
    p.y1	= y1;
    p.x2	= x2;
    p.y2	= y2;
    
    OOP_DoMethod(obj, (OOP_Msg)&p);
    
}

VOID HIDD_GC_UnsetClipRect(OOP_Object *obj)
{
    STATIC_MID;
    struct pHidd_GC_UnsetClipRect p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_GC, moHidd_GC_UnsetClipRect);
        
    p.mID	= mid;
    
    OOP_DoMethod(obj, (OOP_Msg)&p);
}

/********* PlanarBM **********************************/
BOOL HIDD_PlanarBM_SetBitMap(OOP_Object *obj, struct BitMap *bitMap)
{
    STATIC_MID;
    struct pHidd_PlanarBM_SetBitMap p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_PlanarBM, moHidd_PlanarBM_SetBitMap);
        
    p.mID = mid;
    p.bitMap = bitMap;
    
    return (BOOL)OOP_DoMethod(obj, (OOP_Msg)&p);
}


/********* ColorMap *********************************/

BOOL HIDD_CM_SetColors(OOP_Object *obj, HIDDT_Color *colors, ULONG firstColor, ULONG numColors, OOP_Object *pixFmt)
{
    STATIC_MID;
    struct pHidd_ColorMap_SetColors p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_ColorMap, moHidd_ColorMap_SetColors);
        
    p.mID = mid;
    p.colors	 = colors;
    p.firstColor = firstColor;
    p.numColors	 = numColors;
    p.pixFmt	 = pixFmt;
    
    return OOP_DoMethod(obj, (OOP_Msg)&p);
}

HIDDT_Pixel HIDD_CM_GetPixel(OOP_Object *obj, ULONG pixelNo) /* Starts at 0 */
{
    STATIC_MID;
    struct pHidd_ColorMap_GetPixel p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_ColorMap, moHidd_ColorMap_GetPixel);
        
    p.mID = mid;
    p.pixelNo = pixelNo;
    
    return (HIDDT_Pixel)OOP_DoMethod(obj, (OOP_Msg)&p);
}


BOOL HIDD_CM_GetColor(OOP_Object *obj, ULONG colorNo, HIDDT_Color *colorReturn) /* Starts at 0 */
{
    STATIC_MID;
    struct pHidd_ColorMap_GetColor p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_ColorMap, moHidd_ColorMap_GetColor);
        
    p.mID = mid;
    p.colorNo	  = colorNo;
    p.colorReturn = colorReturn;
    
    return (BOOL)OOP_DoMethod(obj, (OOP_Msg)&p);
}
