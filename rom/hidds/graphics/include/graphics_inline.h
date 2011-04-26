/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Inline stubs for graphics, bitmap, gc and colormap hidd class
    Lang: english
*/

#ifndef HIDD_GRAPHICS_INLINE_H
#define HIDD_GRAPHICS_INLINE_H

#include <exec/libraries.h>
#include <graphics/view.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/oop.h>

/* Compatibility hack. In C++ template is a reserved keyword, so we
   can't use it as variable name */
#ifndef __cplusplus
#define template Template
#endif

/***************************************************************/

#ifndef HiddGfxBase
#define HiddGfxBase HIDD_Gfx_GetMethodBase(OOPBase)

static inline OOP_MethodID HIDD_Gfx_GetMethodBase(struct Library *OOPBase)
{
    static OOP_MethodID base;

    if (!base)
	base = OOP_GetMethodID(IID_Hidd_Gfx, 0);

    return base;
}

#endif

static inline OOP_Object * HIDD_Gfx_NewGC(OOP_Object *obj, struct TagItem *tagList)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_NewGC p;

    p.mID      = HiddGfxBase + moHidd_Gfx_NewGC;
    p.attrList = tagList;

    return (OOP_Object *)OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline void HIDD_Gfx_DisposeGC(OOP_Object *obj, OOP_Object *gc)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_DisposeGC p;
    
    p.mID = HiddGfxBase + moHidd_Gfx_DisposeGC;
    p.gc  = gc;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline OOP_Object * HIDD_Gfx_NewBitMap(OOP_Object *obj, struct TagItem *tagList)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_NewBitMap p;

    p.mID      = HiddGfxBase + moHidd_Gfx_NewBitMap;
    p.attrList = tagList;

    return (OOP_Object *)OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline void HIDD_Gfx_DisposeBitMap(OOP_Object *obj, OOP_Object *bitMap)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_DisposeBitMap p;
    
    p.mID    = HiddGfxBase + moHidd_Gfx_DisposeBitMap;
    p.bitMap = bitMap;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline OOP_Object * HIDD_Gfx_NewOverlay(OOP_Object *obj, struct TagItem *tagList)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_NewOverlay p;

    p.mID      = HiddGfxBase + moHidd_Gfx_NewOverlay;
    p.attrList = tagList;

    return(OOP_Object *)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline void HIDD_Gfx_DisposeOverlay(OOP_Object *obj, OOP_Object *Overlay)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_DisposeOverlay p;
    
    p.mID     = HiddGfxBase + moHidd_Gfx_DisposeOverlay;
    p.Overlay = Overlay;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

/***************************************************************/

static inline HIDDT_ModeID * HIDD_Gfx_QueryModeIDs(OOP_Object *obj, struct TagItem *queryTags)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_QueryModeIDs p;
    
    p.mID       = HiddGfxBase + moHidd_Gfx_QueryModeIDs;
    p.queryTags	= queryTags;

    return (HIDDT_ModeID *)OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_Gfx_ReleaseModeIDs(OOP_Object *obj, HIDDT_ModeID *modeIDs)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_ReleaseModeIDs p;
    
    p.mID     = HiddGfxBase + moHidd_Gfx_ReleaseModeIDs;
    p.modeIDs = modeIDs;

    OOP_DoMethod(obj, &p.mID);
}


/***************************************************************/
static inline OOP_Object *    HIDD_Gfx_GetPixFmt  (OOP_Object *obj, HIDDT_StdPixFmt stdPixFmt)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_GetPixFmt p;
    
    p.mID       = HiddGfxBase + moHidd_Gfx_GetPixFmt;
    p.stdPixFmt	= stdPixFmt;
    
    return (OOP_Object *)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline BOOL HIDD_Gfx_CheckMode(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object *sync, OOP_Object *pixFmt)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_CheckMode p;

    
    p.mID    = HiddGfxBase + moHidd_Gfx_CheckMode;
    p.modeID = modeID;
    p.sync   = sync;
    p.pixFmt = pixFmt;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/
static inline BOOL HIDD_Gfx_GetMode(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object **syncPtr, OOP_Object **pixFmtPtr)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_GetMode p;

    p.mID       = HiddGfxBase + moHidd_Gfx_GetMode;
    p.modeID	= modeID;
    p.syncPtr	= syncPtr;
    p.pixFmtPtr	= pixFmtPtr;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/
static inline HIDDT_ModeID HIDD_Gfx_NextModeID(OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object **syncPtr, OOP_Object **pixFmtPtr)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_NextModeID p;

    p.mID       = HiddGfxBase + moHidd_Gfx_NextModeID;
    p.modeID	= modeID;
    p.syncPtr	= syncPtr;
    p.pixFmtPtr	= pixFmtPtr;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline BOOL HIDD_Gfx_SetCursorShape(OOP_Object *obj, OOP_Object *shape, LONG xoffset, LONG yoffset)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_SetCursorShape p;

    p.mID     = HiddGfxBase + moHidd_Gfx_SetCursorShape;
    p.shape   = shape;
    p.xoffset = xoffset;
    p.yoffset = yoffset;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline BOOL HIDD_Gfx_SetCursorPos(OOP_Object *obj, LONG x, LONG y)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_SetCursorPos p;

    p.mID = HiddGfxBase + moHidd_Gfx_SetCursorPos;
        
    
    
    p.x = x;
    p.y = y;
    
    return (BOOL)OOP_DoMethod(obj, &p.mID);
    
}

/***************************************************************/

static inline VOID HIDD_Gfx_SetCursorVisible(OOP_Object *obj, BOOL visible)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_SetCursorVisible p;

    p.mID = HiddGfxBase + moHidd_Gfx_SetCursorVisible;
        
    
    
    p.visible = visible;

    OOP_DoMethod(obj, &p.mID);
    
    return;
    
}

/***************************************************************/

static inline BOOL HIDD_Gfx_SetMode(OOP_Object *obj, OOP_Object *sync)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_SetMode p;

    p.mID = HiddGfxBase + moHidd_Gfx_SetMode;
        
    
    p.Sync = sync;

    return (BOOL)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline OOP_Object *HIDD_Gfx_Show(OOP_Object *obj, OOP_Object *bitMap, ULONG flags)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_Show p;

    p.mID = HiddGfxBase + moHidd_Gfx_Show;
        
    
    
    p.bitMap	= bitMap;
    p.flags	= flags;

    return (OOP_Object *)OOP_DoMethod(obj, &p.mID);
    
}

/***************************************************************/
static inline VOID HIDD_Gfx_CopyBox(OOP_Object *obj, OOP_Object *src, WORD srcX, WORD srcY, OOP_Object *dest, WORD destX, WORD destY, UWORD width, UWORD height, OOP_Object *gc)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_CopyBox p;
    
    p.mID = HiddGfxBase + moHidd_Gfx_CopyBox;
        

    p.src    = src;
    p.srcX   = srcX;
    p.srcY   = srcY;
    p.dest   = dest;
    p.destX  = destX;
    p.destY  = destY;
    p.width  = width;
    p.height = height;
    p.gc     = gc;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline ULONG HIDD_Gfx_ModeProperties(OOP_Object *obj, HIDDT_ModeID modeID, struct HIDD_ModeProperties *props, ULONG propsLen)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_ModeProperties p;
    
    p.mID = HiddGfxBase + moHidd_Gfx_ModeProperties;
    

    p.modeID   = modeID;
    p.props    = props;
    p.propsLen = propsLen;

    return OOP_DoMethod(obj, &p.mID);   
}

/***************************************************************/

static inline ULONG HIDD_Gfx_ShowViewPorts(OOP_Object *obj, struct HIDD_ViewPortData *data)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_ShowViewPorts p;

    p.mID = HiddGfxBase + moHidd_Gfx_ShowViewPorts;


    p.Data = data;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline OOP_Object *HIDD_Gfx_GetSync(OOP_Object *obj, ULONG num)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_GetSync p;

    p.mID = HiddGfxBase + moHidd_Gfx_GetSync;

    
    p.num = num;

    return (OOP_Object *)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline BOOL HIDD_Gfx_GetGamma(OOP_Object *obj, UBYTE *Red, UBYTE *Green, UBYTE *Blue)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_Gamma p;

    p.mID = HiddGfxBase + moHidd_Gfx_GetGamma;


    p.Red   = Red;
    p.Green = Green;
    p.Blue  = Blue;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline BOOL HIDD_Gfx_SetGamma(OOP_Object *obj, UBYTE *Red, UBYTE *Green, UBYTE *Blue)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_Gamma p;

    p.mID = HiddGfxBase + moHidd_Gfx_SetGamma;


    p.Red   = Red;
    p.Green = Green;
    p.Blue  = Blue;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline BOOL HIDD_Gfx_QueryHardware3D(OOP_Object *obj, OOP_Object *pixFmt)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_QueryHardware3D p;

    p.mID = HiddGfxBase + moHidd_Gfx_QueryHardware3D;


    p.pixFmt = pixFmt;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline BOOL HIDD_Gfx_GetMaxSpriteSize(OOP_Object *obj, ULONG Type, ULONG *Width, ULONG *Height)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_GetMaxSpriteSize p;

    p.mID = HiddGfxBase + moHidd_Gfx_GetMaxSpriteSize;


    p.Type   = Type;
    p.Width  = Width;
    p.Height = Height;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline ULONG HIDD_Gfx_MakeViewPort(OOP_Object *obj, struct HIDD_ViewPortData *data)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_MakeViewPort p;

    p.mID = HiddGfxBase + moHidd_Gfx_MakeViewPort;


    p.Data = data;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline void HIDD_Gfx_CleanViewPort(OOP_Object *obj, struct HIDD_ViewPortData *data)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_CleanViewPort p;

    p.mID = HiddGfxBase + moHidd_Gfx_CleanViewPort;


    p.Data = data;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline ULONG HIDD_Gfx_PrepareViewPorts(OOP_Object *obj, struct HIDD_ViewPortData *data, struct View *view)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_Gfx_PrepareViewPorts p;

    p.mID = HiddGfxBase + moHidd_Gfx_PrepareViewPorts;


    p.Data = data;
    p.view = view;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#ifndef HiddBitMapBase
#define HiddBitMapBase HIDD_BitMap_GetMethodBase(OOPBase)

static inline OOP_MethodID HIDD_BitMap_GetMethodBase(struct Library *OOPBase)
{
    static OOP_MethodID base;

    if (!base)
	base = OOP_GetMethodID(IID_Hidd_BitMap, 0);

    return base;
}

#endif


static inline BOOL HIDD_BM_SetColors (OOP_Object *obj, HIDDT_Color *colors, ULONG firstColor, ULONG numColors)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_SetColors p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_SetColors;

    p.colors     = colors;
    p.firstColor = firstColor;
    p.numColors  = numColors;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline ULONG HIDD_BM_PutPixel(OOP_Object *obj, WORD x, WORD y, HIDDT_Pixel val)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutPixel p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutPixel;
        

    p.x    = x;
    p.y    = y;
    p.pixel  = val;

    return OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline ULONG HIDD_BM_DrawPixel(OOP_Object *obj, OOP_Object *gc, WORD x, WORD y)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawPixel p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_DrawPixel;
        

    p.gc   = gc;
    p.x    = x;
    p.y    = y;

    return OOP_DoMethod(obj, &p.mID) ;
}
/***************************************************************/

static inline HIDDT_Pixel HIDD_BM_GetPixel(OOP_Object *obj, WORD x, WORD y)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_GetPixel p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_GetPixel;
        

    p.x    = x;
    p.y    = y;

    return OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_DrawLine(OOP_Object *obj, OOP_Object *gc, WORD x1, WORD y1, WORD x2, WORD y2)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawLine p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_DrawLine;
        

    p.gc    = gc;
    p.x1    = x1;
    p.y1    = y1;
    p.x2    = x2;
    p.y2    = y2;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/


static inline VOID HIDD_BM_DrawRect (OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawRect p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_DrawRect;
        

    p.gc     = gc;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_FillRect (OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawRect p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_FillRect;
        

    p.gc     = gc;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_DrawEllipse (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, WORD rx, WORD ry)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawEllipse p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_DrawEllipse;
        

    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_FillEllipse (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, WORD ry, WORD rx)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawEllipse p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_FillEllipse;
        

    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_DrawPolygon (OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawPolygon p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_DrawPolygon;
        

    p.gc     = gc;
    p.n      = n;
    p.coords = coords;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_FillPolygon (OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawPolygon p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_FillPolygon;
        

    p.gc     = gc;
    p.n      = n;
    p.coords = coords;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_DrawText (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, STRPTR text, UWORD length)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawText p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_DrawText;
        

    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_FillText (OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, STRPTR text, UWORD length)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_DrawText p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_FillText;
        

    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

static inline VOID HIDD_BM_Clear (OOP_Object *obj, OOP_Object *gc)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_Clear p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_Clear;
        

    p.gc     = gc;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID     HIDD_BM_GetImage  (OOP_Object *obj
	, UBYTE *pixels
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height
	, HIDDT_StdPixFmt pixFmt)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_GetImage p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_GetImage;
        

    p.pixels = pixels;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    
    p.pixFmt = pixFmt;
    
    

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID     HIDD_BM_PutImage  (OOP_Object *obj
	, OOP_Object *gc
	, UBYTE *pixels
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height
	, HIDDT_StdPixFmt pixFmt)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutImage p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutImage;
        

    p.gc     = gc;
    p.pixels = pixels;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    p.pixFmt = pixFmt;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID     HIDD_BM_PutAlphaImage  (OOP_Object *obj
	, OOP_Object *gc
	, UBYTE *pixels
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutAlphaImage p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutAlphaImage;
        

    p.gc     = gc;
    p.pixels = pixels;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID     HIDD_BM_PutTemplate  (OOP_Object *obj
	, OOP_Object *gc
	, UBYTE *Template
	, ULONG modulo
	, WORD srcx
	, WORD x, WORD y
	, WORD width, WORD height
	, BOOL inverttemplate)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutTemplate p;
    
    p.mID            = HiddBitMapBase + moHidd_BitMap_PutTemplate;
    p.gc     	     = gc;
    p.Template       = Template;
    p.modulo 	     = modulo;
    p.srcx   	     = srcx;
    p.x      	     = x;
    p.y      	     = y;
    p.width  	     = width;
    p.height 	     = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID     HIDD_BM_PutAlphaTemplate  (OOP_Object *obj
	, OOP_Object *gc
	, UBYTE *alpha
	, ULONG modulo
	, WORD x, WORD y
	, WORD width, WORD height
	, BOOL invertalpha)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutAlphaTemplate p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutAlphaTemplate;
        

    p.gc     = gc;
    p.alpha  = alpha;
    p.modulo = modulo;
    p.x = x;
    p.y = y;
    p.width  = width;
    p.height = height;
    p.invertalpha = invertalpha;
    
    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID HIDD_BM_PutPattern(OOP_Object *obj, OOP_Object *gc, UBYTE *pattern,
    	    	    	WORD patternsrcx, WORD patternsrcy, WORD patternheight,
			WORD patterndepth, HIDDT_PixelLUT *patternlut,
			BOOL invertpattern, UBYTE *mask, ULONG maskmodulo,
			WORD masksrcx, WORD x, WORD y, WORD width, WORD height)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutPattern p;
    
    p.mID           = HiddBitMapBase + moHidd_BitMap_PutPattern;
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
    
    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID	 HIDD_BM_BlitColorExpansion	 (OOP_Object *obj, OOP_Object *gc, OOP_Object *srcBitMap, WORD srcX, WORD srcY, WORD destX, WORD destY,  UWORD width, UWORD height)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_BlitColorExpansion p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_BlitColorExpansion;
        

    p.gc	= gc;
    p.srcBitMap	= srcBitMap;
    p.srcX	= srcX;
    p.srcY	= srcY;
    p.destX 	= destX;
    p.destY	= destY;
    p.width	= width;
    p.height	= height;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline HIDDT_Pixel HIDD_BM_MapColor(OOP_Object *obj, HIDDT_Color *color)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_MapColor p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_MapColor;
    

    p.color	= color;
 
    
    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID HIDD_BM_UnmapPixel(OOP_Object *obj, HIDDT_Pixel pixel, HIDDT_Color *color)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_UnmapPixel p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_UnmapPixel;
    

    p.pixel	= pixel;
    p.color	= color;

    OOP_DoMethod(obj, &p.mID);
}


/***************************************************************/

static inline VOID     HIDD_BM_PutImageLUT  (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutImageLUT p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutImageLUT;
        

    p.gc	= gc;
    p.pixels	= pixels;
    p.modulo	= modulo;
    p.x		= x;
    p.y		= y;
    p.width	= width;
    p.height	= height;
    p.pixlut	= pixlut;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

static inline VOID HIDD_BM_PutTranspImageLUT  (OOP_Object *obj, OOP_Object *gc, UBYTE *pixels,
    	    	    	    	 ULONG modulo, WORD x, WORD y, WORD width, WORD height,
				 HIDDT_PixelLUT *pixlut, UBYTE transparent)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutTranspImageLUT p;
    
    p.mID         = HiddBitMapBase + moHidd_BitMap_PutTranspImageLUT;
    p.gc	  = gc;
    p.pixels	  = pixels;
    p.modulo	  = modulo;
    p.x		  = x;
    p.y		  = y;
    p.width	  = width;
    p.height	  = height;
    p.pixlut	  = pixlut;
    p.transparent = transparent;
    
    OOP_DoMethod(obj, &p.mID);
}


/***************************************************************/

static inline VOID     HIDD_BM_GetImageLUT  (OOP_Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_GetImageLUT p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_GetImageLUT;
        

    p.pixels	= pixels;
    p.modulo	= modulo;
    p.x		= x;
    p.y		= y;
    p.width	= width;
    p.height	= height;
    p.pixlut	= pixlut;

    OOP_DoMethod(obj, &p.mID);
}


static inline ULONG HIDD_BM_BytesPerLine(OOP_Object *obj, HIDDT_StdPixFmt pixFmt, ULONG width)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_BytesPerLine p;

    p.mID = HiddBitMapBase + moHidd_BitMap_BytesPerLine;

    
    p.pixFmt	= pixFmt;
    p.width	= width;

    return OOP_DoMethod(obj, &p.mID);
     
}

/***************************************************************/


static inline VOID     HIDD_BM_ConvertPixels  (OOP_Object *obj
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_ConvertPixels p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_ConvertPixels;
        
    
    p.srcPixFmt = srcPixFmt;
    p.srcPixels = srcPixels;
    
    p.srcMod	= srcMod;
    
    p.dstBuf	= dstBuf;
    p.dstPixFmt	= dstPixFmt;
    
    p.dstMod	= dstMod;
    
    p.width	= width;
    p.height	= height;
    
    p.pixlut	= pixlut;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_FillMemRect8 (OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
	, UBYTE fill
)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_FillMemRect8 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_FillMemRect8;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_FillMemRect16(OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
	, UWORD fill
)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_FillMemRect16 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_FillMemRect16;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_FillMemRect24 (OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
	, ULONG fill
)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_FillMemRect24 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_FillMemRect24;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_FillMemRect32 (OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
	, ULONG fill
)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_FillMemRect32 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_FillMemRect32;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, &p.mID);
}


static inline VOID	HIDD_BM_InvertMemRect(OOP_Object *obj
    	, APTR dstBuf
	, WORD minX
	, WORD minY
	, WORD maxX
	, WORD maxY
	, ULONG dstMod
)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_InvertMemRect p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_InvertMemRect;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, &p.mID);
}


static inline VOID	HIDD_BM_CopyMemBox8(OOP_Object *obj
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_CopyMemBox8 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_CopyMemBox8;
        
    
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
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_CopyMemBox16(OOP_Object *obj
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_CopyMemBox16 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_CopyMemBox16;
        
    
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
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_CopyMemBox24(OOP_Object *obj
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_CopyMemBox24 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_CopyMemBox24;
        
    
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
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_CopyMemBox32(OOP_Object *obj
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_CopyMemBox32 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_CopyMemBox32;
        
    
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
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_CopyLUTMemBox16(OOP_Object *obj
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_CopyLUTMemBox16 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_CopyLUTMemBox16;
        
    
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
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_CopyLUTMemBox24(OOP_Object *obj
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_CopyLUTMemBox24 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_CopyLUTMemBox24;
        
    
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
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_CopyLUTMemBox32(OOP_Object *obj
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_CopyLUTMemBox32 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_CopyLUTMemBox32;
        
    
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
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_PutMem32Image8(OOP_Object *obj,
    	    	    	       APTR src,
			       APTR dst,
			       WORD dstX,
			       WORD dstY,
			       UWORD width,
			       UWORD height,
			       ULONG srcMod,
			       ULONG dstMod)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMem32Image8 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMem32Image8;
    
    
    p.src = src;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_PutMem32Image16(OOP_Object *obj,
    	    	    	        APTR src,
			        APTR dst,
			        WORD dstX,
			        WORD dstY,
			        UWORD width,
			        UWORD height,
			        ULONG srcMod,
			        ULONG dstMod)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMem32Image16 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMem32Image16;
    
    
    p.src = src;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_PutMem32Image24(OOP_Object *obj,
    	    	    	        APTR src,
			        APTR dst,
			        WORD dstX,
			        WORD dstY,
			        UWORD width,
			        UWORD height,
			        ULONG srcMod,
			        ULONG dstMod)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMem32Image24 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMem32Image24;
    
    
    p.src = src;
    p.dst = dst;
    p.dstX = dstX;
    p.dstY = dstY;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_GetMem32Image8(OOP_Object *obj,
    	    	    	       APTR src,
			       WORD srcX,
			       WORD srcY,
			       APTR dst,
			       UWORD width,
			       UWORD height,
			       ULONG srcMod,
			       ULONG dstMod)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_GetMem32Image8 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_GetMem32Image8;
    
    
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_GetMem32Image16(OOP_Object *obj,
    	    	    		APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_GetMem32Image16 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_GetMem32Image16;
    
    
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_GetMem32Image24(OOP_Object *obj,
    	    	    		APTR src,
				WORD srcX,
				WORD srcY,
				APTR dst,
				UWORD width,
				UWORD height,
				ULONG srcMod,
				ULONG dstMod)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_GetMem32Image24 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_GetMem32Image24;
    
    
    p.src = src;
    p.srcX = srcX;
    p.srcY = srcY;
    p.dst = dst;
    p.width = width;
    p.height = height;
    p.srcMod = srcMod;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_PutMemTemplate8	(OOP_Object *obj,
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
				 BOOL inverttemplate)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMemTemplate8 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMemTemplate8;
    
    
    p.gc = gc;
    p.Template = Template;
    p.modulo = modulo;
    p.srcx = srcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_PutMemTemplate16(OOP_Object *obj,
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
				 BOOL inverttemplate)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMemTemplate16 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMemTemplate16;
    
    
    p.gc = gc;
    p.Template = Template;
    p.modulo = modulo;
    p.srcx = srcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_PutMemTemplate24(OOP_Object *obj,
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
				 BOOL inverttemplate)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMemTemplate24 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMemTemplate24;
    
    
    p.gc = gc;
    p.Template = Template;
    p.modulo = modulo;
    p.srcx = srcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_PutMemTemplate32(OOP_Object *obj,
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
				 BOOL inverttemplate)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMemTemplate32 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMemTemplate32;
    
    
    p.gc = gc;
    p.Template = Template;
    p.modulo = modulo;
    p.srcx = srcx;
    p.dst = dst;
    p.dstMod = dstMod;
    p.x = x;
    p.y = y;
    p.width = width;
    p.height = height;
    p.inverttemplate = inverttemplate;
    
    OOP_DoMethod(obj, &p.mID);
}

static inline VOID	HIDD_BM_PutMemPattern8	(OOP_Object *obj,
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMemPattern8 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMemPattern8;
    
    
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
    
    OOP_DoMethod(obj, &p.mID);    
}
				 
static inline VOID	HIDD_BM_PutMemPattern16	(OOP_Object *obj,
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMemPattern16 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMemPattern16;
    
    
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
    
    OOP_DoMethod(obj, &p.mID);    
}

static inline VOID	HIDD_BM_PutMemPattern24	(OOP_Object *obj,
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMemPattern24 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMemPattern24;
    
    
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
    
    OOP_DoMethod(obj, &p.mID);    
}
	
static inline VOID	HIDD_BM_PutMemPattern32	(OOP_Object *obj,
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
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_PutMemPattern32 p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_PutMemPattern32;
    
    
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
    
    OOP_DoMethod(obj, &p.mID);    
}
			
static inline OOP_Object * HIDD_BM_SetColorMap(OOP_Object *obj, OOP_Object *colorMap)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_SetColorMap p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_SetColorMap;
        
    
    p.colorMap = colorMap;
    
    return (OOP_Object *)OOP_DoMethod(obj, &p.mID);
}


static inline BOOL HIDD_BM_ObtainDirectAccess(OOP_Object *obj
	, UBYTE **addressReturn
	, ULONG *widthReturn
	, ULONG *heightReturn
	, ULONG *bankSizeReturn
	, ULONG *memSizeReturn )
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_ObtainDirectAccess p;

    p.mID            = HiddBitMapBase + moHidd_BitMap_ObtainDirectAccess;        
    p.addressReturn  = addressReturn;
    p.widthReturn    = widthReturn;
    p.heightReturn   = heightReturn;
    p.bankSizeReturn = bankSizeReturn;
    p.memSizeReturn  = memSizeReturn;

    /* Clear this by default */
    *addressReturn = NULL;

    return OOP_DoMethod(obj, &p.mID);
}

static inline VOID HIDD_BM_ReleaseDirectAccess(OOP_Object *obj)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_ReleaseDirectAccess p;

    p.mID = HiddBitMapBase + moHidd_BitMap_ReleaseDirectAccess;

    OOP_DoMethod(obj, &p.mID);
}

static inline VOID HIDD_BM_BitMapScale(OOP_Object *obj, OOP_Object *src, OOP_Object *dest, struct BitScaleArgs * bsa, OOP_Object *gc)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_BitMapScale p;
    
    p.mID = HiddBitMapBase + moHidd_BitMap_BitMapScale;
    p.src = src;
    p.dst = dest;
    p.bsa = bsa;
    p.gc  = gc;

    OOP_DoMethod(obj, &p.mID);
}

static inline HIDDT_RGBConversionFunction HIDD_BM_SetRGBConversionFunction(OOP_Object *obj, HIDDT_StdPixFmt srcPixFmt, HIDDT_StdPixFmt dstPixFmt,
				    	    	    	     HIDDT_RGBConversionFunction function)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_SetRGBConversionFunction p;

    p.mID       = HiddBitMapBase + moHidd_BitMap_SetRGBConversionFunction;    
    p.srcPixFmt = srcPixFmt;
    p.dstPixFmt = dstPixFmt;
    p.function  = function;
    
    return (HIDDT_RGBConversionFunction) OOP_DoMethod(obj, &p.mID);    
}

static inline VOID HIDD_BM_UpdateRect(OOP_Object *obj, WORD x, WORD y, WORD width, WORD height)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_BitMap_UpdateRect p;

    p.mID    = HiddBitMapBase + moHidd_BitMap_UpdateRect;
    p.x      = x;
    p.y      = y;
    p.width  = width;
    p.height = height;

    OOP_DoMethod(obj, &p.mID);    
}

#ifndef HiddGCBase
#define HiddGCBase HIDD_GC_GetMethodBase(OOPBase)

static inline OOP_MethodID HIDD_GC_GetMethodBase(struct Library *OOPBase)
{
    static OOP_MethodID base;

    if (!base)
	base = OOP_GetMethodID(IID_Hidd_GC, 0);

    return base;
}
#endif

/********* GC *****************************************/
static inline VOID HIDD_GC_SetClipRect(OOP_Object *obj, LONG x1, LONG y1, LONG x2, LONG y2)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_GC_SetClipRect p;

    p.mID = HiddGCBase + moHidd_GC_SetClipRect;        
    p.x1  = x1;
    p.y1  = y1;
    p.x2  = x2;
    p.y2  = y2;

    OOP_DoMethod(obj, &p.mID);    
}

static inline VOID HIDD_GC_UnsetClipRect(OOP_Object *obj)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_GC_UnsetClipRect p;
    
    p.mID = HiddGCBase + moHidd_GC_UnsetClipRect;
    
    OOP_DoMethod(obj, &p.mID);
}

#ifndef HiddPlanarBMBase
#define HiddPlanarBMBase HIDD_PlanarBM_GetMethodBase(OOPBase)

static inline OOP_MethodID HIDD_PlanarBM_GetMethodBase(struct Library *OOPBase)
{
    static OOP_MethodID base;

    if (!base)
	base = OOP_GetMethodID(IID_Hidd_PlanarBM, 0);

    return base;
}
#endif

/********* PlanarBM **********************************/
static inline BOOL HIDD_PlanarBM_SetBitMap(OOP_Object *obj, struct BitMap *bitMap)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_PlanarBM_SetBitMap p;
    
    p.mID    = HiddPlanarBMBase + moHidd_PlanarBM_SetBitMap;
    p.bitMap = bitMap;

    return OOP_DoMethod(obj, &p.mID);
}

static inline BOOL HIDD_PlanarBM_GetBitMap(OOP_Object *obj, struct BitMap *bitMap)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_PlanarBM_GetBitMap p;

    p.mID    = HiddPlanarBMBase + moHidd_PlanarBM_GetBitMap;
    p.bitMap = bitMap;

    return OOP_DoMethod(obj, &p.mID);
}

/********* ColorMap *********************************/

#ifndef HiddColorMapBase
#define HiddColorMapBase HIDD_ColorMap_GetMethodBase(OOPBase)

static inline OOP_MethodID HIDD_ColorMap_GetMethodBase(struct Library *OOPBase)
{
    static OOP_MethodID base;

    if (!base)
	base = OOP_GetMethodID(IID_Hidd_ColorMap, 0);

    return base;
}
#endif

static inline BOOL HIDD_CM_SetColors(OOP_Object *obj, HIDDT_Color *colors, ULONG firstColor, ULONG numColors, OOP_Object *pixFmt)
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_ColorMap_SetColors p;
    
    p.mID        = HiddColorMapBase + moHidd_ColorMap_SetColors;
    p.colors	 = colors;
    p.firstColor = firstColor;
    p.numColors	 = numColors;
    p.pixFmt	 = pixFmt;
    
    return OOP_DoMethod(obj, &p.mID);
}

static inline HIDDT_Pixel HIDD_CM_GetPixel(OOP_Object *obj, ULONG pixelNo) /* Starts at 0 */
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_ColorMap_GetPixel p;
    
    p.mID     = HiddColorMapBase + moHidd_ColorMap_GetPixel;
    p.pixelNo = pixelNo;
    
    return OOP_DoMethod(obj, &p.mID);
}


static inline BOOL HIDD_CM_GetColor(OOP_Object *obj, ULONG colorNo, HIDDT_Color *colorReturn) /* Starts at 0 */
{
    struct Library *OOPBase = OOP_OCLASS(obj)->OOPBasePtr;
    struct pHidd_ColorMap_GetColor p;
    
    p.mID         = HiddColorMapBase + moHidd_ColorMap_GetColor;
    p.colorNo	  = colorNo;
    p.colorReturn = colorReturn;
    
    return OOP_DoMethod(obj, &p.mID);
}

#endif /* HIDD_GRAPHICS_INLINE_H */
