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

#if !defined(HiddGfxBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddGfxBase HIDD_Gfx_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_Gfx_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID GfxMethodBase;

    if (!GfxMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OCLASS(obj)->OOPBasePtr;

	GfxMethodBase = OOP_GetMethodID(IID_Hidd_Gfx, 0);
    }

    return GfxMethodBase;
}

#endif

#define HIDD_Gfx_NewGC(obj, tagList) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_NewGC_(HiddGfxBase, __obj, tagList); })

static inline OOP_Object * HIDD_Gfx_NewGC_(OOP_MethodID GfxBase, OOP_Object *obj, struct TagItem *tagList)
{
    struct pHidd_Gfx_NewGC p;

    p.mID      = GfxBase + moHidd_Gfx_NewGC;
    p.attrList = tagList;

    return(OOP_Object *)OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_Gfx_DisposeGC(obj, gc) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_DisposeGC_(HiddGfxBase, __obj, gc); })

static inline void HIDD_Gfx_DisposeGC_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *gc)
{
    struct pHidd_Gfx_DisposeGC p;
    
    p.mID = GfxBase + moHidd_Gfx_DisposeGC;
    p.gc  = gc;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_Gfx_NewBitMap(obj, tagList) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_NewBitMap_(HiddGfxBase, __obj, tagList); })

static inline OOP_Object * HIDD_Gfx_NewBitMap_(OOP_MethodID GfxBase, OOP_Object *obj, struct TagItem *tagList)
{
    struct pHidd_Gfx_NewBitMap p;

    p.mID      = GfxBase + moHidd_Gfx_NewBitMap;
    p.attrList = tagList;

    return(OOP_Object *)OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_Gfx_DisposeBitMap(obj, bitMap) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_DisposeBitMap_(HiddGfxBase, __obj, bitMap); })

static inline void HIDD_Gfx_DisposeBitMap_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *bitMap)
{
    struct pHidd_Gfx_DisposeBitMap p;
    
    p.mID    = GfxBase + moHidd_Gfx_DisposeBitMap;
    p.bitMap = bitMap;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_NewOverlay(obj, tagList) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_NewOverlay_(HiddGfxBase, __obj, tagList); })

static inline OOP_Object * HIDD_Gfx_NewOverlay_(OOP_MethodID GfxBase, OOP_Object *obj, struct TagItem *tagList)
{
    struct pHidd_Gfx_NewOverlay p;

    p.mID      = GfxBase + moHidd_Gfx_NewOverlay;
    p.attrList = tagList;

    return(OOP_Object *)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_DisposeOverlay(obj, Overlay) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_DisposeOverlay_(HiddGfxBase, __obj, Overlay); })

static inline void HIDD_Gfx_DisposeOverlay_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *Overlay)
{
    struct pHidd_Gfx_DisposeOverlay p;
    
    p.mID     = GfxBase + moHidd_Gfx_DisposeOverlay;
    p.Overlay = Overlay;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

/***************************************************************/

#define HIDD_Gfx_QueryModeIDs(obj, queryTags) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_QueryModeIDs_(HiddGfxBase, __obj, queryTags); })

static inline HIDDT_ModeID * HIDD_Gfx_QueryModeIDs_(OOP_MethodID GfxBase, OOP_Object *obj, struct TagItem *queryTags)
{
    struct pHidd_Gfx_QueryModeIDs p;
    
    p.mID       = GfxBase + moHidd_Gfx_QueryModeIDs;
    p.queryTags	= queryTags;

    return(HIDDT_ModeID *)OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_Gfx_ReleaseModeIDs(obj, modeIDs) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_ReleaseModeIDs_(HiddGfxBase, __obj, modeIDs); })

static inline VOID HIDD_Gfx_ReleaseModeIDs_(OOP_MethodID GfxBase, OOP_Object *obj, HIDDT_ModeID *modeIDs)
{
    struct pHidd_Gfx_ReleaseModeIDs p;
    
    p.mID     = GfxBase + moHidd_Gfx_ReleaseModeIDs;
    p.modeIDs = modeIDs;

    OOP_DoMethod(obj, &p.mID);
}


/***************************************************************/
#define HIDD_Gfx_GetPixFmt(obj, stdPixFmt) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_GetPixFmt_(HiddGfxBase, __obj, stdPixFmt); })

static inline OOP_Object *    HIDD_Gfx_GetPixFmt_(OOP_MethodID GfxBase, OOP_Object *obj, HIDDT_StdPixFmt stdPixFmt)
{
    struct pHidd_Gfx_GetPixFmt p;
    
    p.mID       = GfxBase + moHidd_Gfx_GetPixFmt;
    p.stdPixFmt	= stdPixFmt;
    
    return(OOP_Object *)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_CheckMode(obj, modeID, sync, pixFmt) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_CheckMode_(HiddGfxBase, __obj, modeID, sync, pixFmt); })

static inline BOOL HIDD_Gfx_CheckMode_(OOP_MethodID GfxBase, OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object *sync, OOP_Object *pixFmt)
{
    struct pHidd_Gfx_CheckMode p;

    
    p.mID    = GfxBase + moHidd_Gfx_CheckMode;
    p.modeID = modeID;
    p.sync   = sync;
    p.pixFmt = pixFmt;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/
#define HIDD_Gfx_GetMode(obj, modeID, syncPtr, pixFmtPtr) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_GetMode_(HiddGfxBase, __obj, modeID, syncPtr, pixFmtPtr); })

static inline BOOL HIDD_Gfx_GetMode_(OOP_MethodID GfxBase, OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object **syncPtr, OOP_Object **pixFmtPtr)
{
    struct pHidd_Gfx_GetMode p;

    p.mID       = GfxBase + moHidd_Gfx_GetMode;
    p.modeID	= modeID;
    p.syncPtr	= syncPtr;
    p.pixFmtPtr	= pixFmtPtr;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/
#define HIDD_Gfx_NextModeID(obj, modeID, syncPtr, pixFmtPtr) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_NextModeID_(HiddGfxBase, __obj, modeID, syncPtr, pixFmtPtr); })

static inline HIDDT_ModeID HIDD_Gfx_NextModeID_(OOP_MethodID GfxBase, OOP_Object *obj, HIDDT_ModeID modeID, OOP_Object **syncPtr, OOP_Object **pixFmtPtr)
{
    struct pHidd_Gfx_NextModeID p;

    p.mID       = GfxBase + moHidd_Gfx_NextModeID;
    p.modeID	= modeID;
    p.syncPtr	= syncPtr;
    p.pixFmtPtr	= pixFmtPtr;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_SetCursorShape(obj, shape, xoffset, yoffset) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_SetCursorShape_(HiddGfxBase, __obj, shape, xoffset, yoffset); })

static inline BOOL HIDD_Gfx_SetCursorShape_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *shape, WORD xoffset, WORD yoffset)
{
    struct pHidd_Gfx_SetCursorShape p;

    p.mID     = GfxBase + moHidd_Gfx_SetCursorShape;
    p.shape   = shape;
    p.xoffset = xoffset;
    p.yoffset = yoffset;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_SetCursorPos(obj, x, y) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_SetCursorPos_(HiddGfxBase, __obj, x, y); })

static inline BOOL HIDD_Gfx_SetCursorPos_(OOP_MethodID GfxBase, OOP_Object *obj, WORD x, WORD y)
{
    struct pHidd_Gfx_SetCursorPos p;

    p.mID = GfxBase + moHidd_Gfx_SetCursorPos;
        
    
    
    p.x = x;
    p.y = y;
    
    return(BOOL)OOP_DoMethod(obj, &p.mID);
    
}

/***************************************************************/

#define HIDD_Gfx_SetCursorVisible(obj, visible) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_SetCursorVisible_(HiddGfxBase, __obj, visible); })

static inline VOID HIDD_Gfx_SetCursorVisible_(OOP_MethodID GfxBase, OOP_Object *obj, BOOL visible)
{
    struct pHidd_Gfx_SetCursorVisible p;

    p.mID = GfxBase + moHidd_Gfx_SetCursorVisible;
        
    
    
    p.visible = visible;

    OOP_DoMethod(obj, &p.mID);
    
    return;
    
}

/***************************************************************/

#define HIDD_Gfx_SetMode(obj, sync) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_SetMode_(HiddGfxBase, __obj, sync); })

static inline BOOL HIDD_Gfx_SetMode_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *sync)
{
    struct pHidd_Gfx_SetMode p;

    p.mID = GfxBase + moHidd_Gfx_SetMode;
        
    
    p.Sync = sync;

    return(BOOL)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_Show(obj, bitMap, flags) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_Show_(HiddGfxBase, __obj, bitMap, flags); })

static inline OOP_Object *HIDD_Gfx_Show_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *bitMap, ULONG flags)
{
    struct pHidd_Gfx_Show p;

    p.mID = GfxBase + moHidd_Gfx_Show;
        
    
    
    p.bitMap	= bitMap;
    p.flags	= flags;

    return(OOP_Object *)OOP_DoMethod(obj, &p.mID);
    
}

/***************************************************************/
#define HIDD_Gfx_CopyBox(obj, src, srcX, srcY, dest, destX, destY, width, height, gc) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_CopyBox_(HiddGfxBase, __obj, src, srcX, srcY, dest, destX, destY, width, height, gc); })

static inline VOID HIDD_Gfx_CopyBox_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *src, WORD srcX, WORD srcY, OOP_Object *dest, WORD destX, WORD destY, UWORD width, UWORD height, OOP_Object *gc)
{
    struct pHidd_Gfx_CopyBox p;
    
    p.mID = GfxBase + moHidd_Gfx_CopyBox;
        

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
#define HIDD_Gfx_CopyBoxMasked(obj, src, srcX, srcY, dest, destX, destY, width, height, mask, gc) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_CopyBoxMasked_(HiddGfxBase, __obj, src, srcX, srcY, dest, destX, destY, width, height, mask, gc); })

static inline BOOL HIDD_Gfx_CopyBoxMasked_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *src, WORD srcX, WORD srcY, OOP_Object *dest, WORD destX, WORD destY, UWORD width, UWORD height, PLANEPTR mask, OOP_Object *gc)
{
    struct pHidd_Gfx_CopyBoxMasked p;
    
    p.mID = GfxBase + moHidd_Gfx_CopyBoxMasked;

    p.src    = src;
    p.srcX   = srcX;
    p.srcY   = srcY;
    p.dest   = dest;
    p.destX  = destX;
    p.destY  = destY;
    p.width  = width;
    p.height = height;
    p.mask   = mask;
    p.gc     = gc;

    return (BOOL)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_ModeProperties(obj, modeID, props, propsLen) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_ModeProperties_(HiddGfxBase, __obj, modeID, props, propsLen); })

static inline ULONG HIDD_Gfx_ModeProperties_(OOP_MethodID GfxBase, OOP_Object *obj, HIDDT_ModeID modeID, struct HIDD_ModeProperties *props, ULONG propsLen)
{
    struct pHidd_Gfx_ModeProperties p;
    
    p.mID = GfxBase + moHidd_Gfx_ModeProperties;
    

    p.modeID   = modeID;
    p.props    = props;
    p.propsLen = propsLen;

    return OOP_DoMethod(obj, &p.mID);   
}

/***************************************************************/

#define HIDD_Gfx_ShowViewPorts(obj, data) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_ShowViewPorts_(HiddGfxBase, __obj, data); })

static inline ULONG HIDD_Gfx_ShowViewPorts_(OOP_MethodID GfxBase, OOP_Object *obj, struct HIDD_ViewPortData *data)
{
    struct pHidd_Gfx_ShowViewPorts p;

    p.mID = GfxBase + moHidd_Gfx_ShowViewPorts;


    p.Data = data;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_GetSync(obj, num) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_GetSync_(HiddGfxBase, __obj, num); })

static inline OOP_Object *HIDD_Gfx_GetSync_(OOP_MethodID GfxBase, OOP_Object *obj, ULONG num)
{
    struct pHidd_Gfx_GetSync p;

    p.mID = GfxBase + moHidd_Gfx_GetSync;

    
    p.num = num;

    return(OOP_Object *)OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_GetGamma(obj, Red, Green, Blue) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_GetGamma_(HiddGfxBase, __obj, Red, Green, Blue); })

static inline BOOL HIDD_Gfx_GetGamma_(OOP_MethodID GfxBase, OOP_Object *obj, UBYTE *Red, UBYTE *Green, UBYTE *Blue)
{
    struct pHidd_Gfx_Gamma p;

    p.mID = GfxBase + moHidd_Gfx_GetGamma;


    p.Red   = Red;
    p.Green = Green;
    p.Blue  = Blue;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_SetGamma(obj, Red, Green, Blue) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_SetGamma_(HiddGfxBase, __obj, Red, Green, Blue); })

static inline BOOL HIDD_Gfx_SetGamma_(OOP_MethodID GfxBase, OOP_Object *obj, UBYTE *Red, UBYTE *Green, UBYTE *Blue)
{
    struct pHidd_Gfx_Gamma p;

    p.mID = GfxBase + moHidd_Gfx_SetGamma;


    p.Red   = Red;
    p.Green = Green;
    p.Blue  = Blue;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_QueryHardware3D(obj, pixFmt) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_QueryHardware3D_(HiddGfxBase, __obj, pixFmt); })

static inline BOOL HIDD_Gfx_QueryHardware3D_(OOP_MethodID GfxBase, OOP_Object *obj, OOP_Object *pixFmt)
{
    struct pHidd_Gfx_QueryHardware3D p;

    p.mID = GfxBase + moHidd_Gfx_QueryHardware3D;


    p.pixFmt = pixFmt;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_GetMaxSpriteSize(obj, Type, Width, Height) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_GetMaxSpriteSize_(HiddGfxBase, __obj, Type, Width, Height); })

static inline BOOL HIDD_Gfx_GetMaxSpriteSize_(OOP_MethodID GfxBase, OOP_Object *obj, ULONG Type, ULONG *Width, ULONG *Height)
{
    struct pHidd_Gfx_GetMaxSpriteSize p;

    p.mID = GfxBase + moHidd_Gfx_GetMaxSpriteSize;


    p.Type   = Type;
    p.Width  = Width;
    p.Height = Height;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_MakeViewPort(obj, data) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_MakeViewPort_(HiddGfxBase, __obj, data); })

static inline ULONG HIDD_Gfx_MakeViewPort_(OOP_MethodID GfxBase, OOP_Object *obj, struct HIDD_ViewPortData *data)
{
    struct pHidd_Gfx_MakeViewPort p;

    p.mID = GfxBase + moHidd_Gfx_MakeViewPort;


    p.Data = data;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_CleanViewPort(obj, data) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_CleanViewPort_(HiddGfxBase, __obj, data); })

static inline void HIDD_Gfx_CleanViewPort_(OOP_MethodID GfxBase, OOP_Object *obj, struct HIDD_ViewPortData *data)
{
    struct pHidd_Gfx_CleanViewPort p;

    p.mID = GfxBase + moHidd_Gfx_CleanViewPort;


    p.Data = data;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_Gfx_PrepareViewPorts(obj, data, view) \
	({OOP_Object *__obj = obj;\
	  HIDD_Gfx_PrepareViewPorts_(HiddGfxBase, __obj, data, view); })

static inline ULONG HIDD_Gfx_PrepareViewPorts_(OOP_MethodID GfxBase, OOP_Object *obj, struct HIDD_ViewPortData *data, struct View *view)
{
    struct pHidd_Gfx_PrepareViewPorts p;

    p.mID = GfxBase + moHidd_Gfx_PrepareViewPorts;


    p.Data = data;
    p.view = view;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#if !defined(HiddBitMapBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddBitMapBase HIDD_BitMap_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_BitMap_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID BitMapMethodBase;

    if (!BitMapMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OCLASS(obj)->OOPBasePtr;

	BitMapMethodBase = OOP_GetMethodID(IID_Hidd_BitMap, 0);
    }

    return BitMapMethodBase;
}

#endif


#define HIDD_BM_SetColors(obj, colors, firstColor, numColors) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_SetColors_(HiddBitMapBase, __obj, colors, firstColor, numColors); })

static inline BOOL HIDD_BM_SetColors_(OOP_MethodID BitMapBase, OOP_Object *obj, HIDDT_Color *colors, UWORD firstColor, UWORD numColors)
{
    struct pHidd_BitMap_SetColors p;
    
    p.mID = BitMapBase + moHidd_BitMap_SetColors;

    p.colors     = colors;
    p.firstColor = firstColor;
    p.numColors  = numColors;

    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_BM_PutPixel(obj, x, y, val) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutPixel_(HiddBitMapBase, __obj, x, y, val); })

static inline ULONG HIDD_BM_PutPixel_(OOP_MethodID BitMapBase, OOP_Object *obj, WORD x, WORD y, HIDDT_Pixel val)
{
    struct pHidd_BitMap_PutPixel p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutPixel;
        

    p.x    = x;
    p.y    = y;
    p.pixel  = val;

    return OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_DrawPixel(obj, gc, x, y) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_DrawPixel_(HiddBitMapBase, __obj, gc, x, y); })

static inline ULONG HIDD_BM_DrawPixel_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, WORD x, WORD y)
{
    struct pHidd_BitMap_DrawPixel p;
    
    p.mID = BitMapBase + moHidd_BitMap_DrawPixel;
        

    p.gc   = gc;
    p.x    = x;
    p.y    = y;

    return OOP_DoMethod(obj, &p.mID) ;
}
/***************************************************************/

#define HIDD_BM_GetPixel(obj, x, y) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_GetPixel_(HiddBitMapBase, __obj, x, y); })

static inline HIDDT_Pixel HIDD_BM_GetPixel_(OOP_MethodID BitMapBase, OOP_Object *obj, WORD x, WORD y)
{
    struct pHidd_BitMap_GetPixel p;
    
    p.mID = BitMapBase + moHidd_BitMap_GetPixel;
        

    p.x    = x;
    p.y    = y;

    return OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_DrawLine(obj, gc, x1, y1, x2, y2) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_DrawLine_(HiddBitMapBase, __obj, gc, x1, y1, x2, y2); })

static inline VOID HIDD_BM_DrawLine_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, WORD x1, WORD y1, WORD x2, WORD y2)
{
    struct pHidd_BitMap_DrawLine p;
    
    p.mID = BitMapBase + moHidd_BitMap_DrawLine;
        

    p.gc    = gc;
    p.x1    = x1;
    p.y1    = y1;
    p.x2    = x2;
    p.y2    = y2;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/


#define HIDD_BM_DrawRect(obj, gc, minX, minY, maxX, maxY) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_DrawRect_(HiddBitMapBase, __obj, gc, minX, minY, maxX, maxY); })

static inline VOID HIDD_BM_DrawRect_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    struct pHidd_BitMap_DrawRect p;
    
    p.mID = BitMapBase + moHidd_BitMap_DrawRect;
        

    p.gc     = gc;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_FillRect(obj, gc, minX, minY, maxX, maxY) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_FillRect_(HiddBitMapBase, __obj, gc, minX, minY, maxX, maxY); })

static inline VOID HIDD_BM_FillRect_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, WORD minX, WORD minY, WORD maxX, WORD maxY)
{
    struct pHidd_BitMap_DrawRect p;
    
    p.mID = BitMapBase + moHidd_BitMap_FillRect;
        

    p.gc     = gc;
    p.minX   = minX;
    p.minY   = minY;
    p.maxX   = maxX;
    p.maxY   = maxY;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_DrawEllipse(obj, gc, x, y, rx, ry) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_DrawEllipse_(HiddBitMapBase, __obj, gc, x, y, rx, ry); })

static inline VOID HIDD_BM_DrawEllipse_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, WORD rx, WORD ry)
{
    struct pHidd_BitMap_DrawEllipse p;
    
    p.mID = BitMapBase + moHidd_BitMap_DrawEllipse;
        

    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_FillEllipse(obj, gc, x, y, ry, rx) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_FillEllipse_(HiddBitMapBase, __obj, gc, x, y, ry, rx); })

static inline VOID HIDD_BM_FillEllipse_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, WORD ry, WORD rx)
{
    struct pHidd_BitMap_DrawEllipse p;
    
    p.mID = BitMapBase + moHidd_BitMap_FillEllipse;
        

    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.rx     = rx;
    p.ry     = ry;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_DrawPolygon(obj, gc, n, coords) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_DrawPolygon_(HiddBitMapBase, __obj, gc, n, coords); })

static inline VOID HIDD_BM_DrawPolygon_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords)
{
    struct pHidd_BitMap_DrawPolygon p;
    
    p.mID = BitMapBase + moHidd_BitMap_DrawPolygon;
        

    p.gc     = gc;
    p.n      = n;
    p.coords = coords;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_FillPolygon(obj, gc, n, coords) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_FillPolygon_(HiddBitMapBase, __obj, gc, n, coords); })

static inline VOID HIDD_BM_FillPolygon_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UWORD n, WORD *coords)
{
    struct pHidd_BitMap_DrawPolygon p;
    
    p.mID = BitMapBase + moHidd_BitMap_FillPolygon;
        

    p.gc     = gc;
    p.n      = n;
    p.coords = coords;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_DrawText(obj, gc, x, y, text, length) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_DrawText_(HiddBitMapBase, __obj, gc, x, y, text, length); })

static inline VOID HIDD_BM_DrawText_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, STRPTR text, UWORD length)
{
    struct pHidd_BitMap_DrawText p;
    
    p.mID = BitMapBase + moHidd_BitMap_DrawText;
        

    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_FillText(obj, gc, x, y, text, length) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_FillText_(HiddBitMapBase, __obj, gc, x, y, text, length); })

static inline VOID HIDD_BM_FillText_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, WORD x, WORD y, STRPTR text, UWORD length)
{
    struct pHidd_BitMap_DrawText p;
    
    p.mID = BitMapBase + moHidd_BitMap_FillText;
        

    p.gc     = gc;
    p.x      = x;
    p.y      = y;
    p.text   = text;
    p.length = length;

    OOP_DoMethod(obj, &p.mID);
}
/***************************************************************/

#define HIDD_BM_Clear(obj, gc) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_Clear_(HiddBitMapBase, __obj, gc); })

static inline VOID HIDD_BM_Clear_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc)
{
    struct pHidd_BitMap_Clear p;
    
    p.mID = BitMapBase + moHidd_BitMap_Clear;
        

    p.gc     = gc;

    OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_BM_GetImage(obj, pixels, modulo, x, y, width, height, pixFmt) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_GetImage_(HiddBitMapBase, __obj, pixels, modulo, x, y, width, height, pixFmt); })

static inline VOID HIDD_BM_GetImage_(OOP_MethodID BitMapBase, OOP_Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_StdPixFmt pixFmt)
{
    struct pHidd_BitMap_GetImage p;
    
    p.mID = BitMapBase + moHidd_BitMap_GetImage;
        

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

#define HIDD_BM_PutImage(obj, gc, pixels, modulo, x, y, width, height, pixFmt) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutImage_(HiddBitMapBase, __obj, gc, pixels, modulo, x, y, width, height, pixFmt); })

static inline VOID     HIDD_BM_PutImage_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_StdPixFmt pixFmt)
{
    struct pHidd_BitMap_PutImage p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutImage;
        

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

#define HIDD_BM_PutAlphaImage(obj, gc, pixels, modulo, x, y, width, height) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutAlphaImage_(HiddBitMapBase, __obj, gc, pixels, modulo, x, y, width, height); })

static inline VOID     HIDD_BM_PutAlphaImage_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height)
{
    struct pHidd_BitMap_PutAlphaImage p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutAlphaImage;
        

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

#define HIDD_BM_PutTemplate(obj, gc, Template, modulo, srcx, x, y, width, height, inverttemplate) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutTemplate_(HiddBitMapBase, __obj, gc, Template, modulo, srcx, x, y, width, height, inverttemplate); })

static inline VOID     HIDD_BM_PutTemplate_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *Template, ULONG modulo, WORD srcx, WORD x, WORD y, WORD width, WORD height, BOOL inverttemplate)
{
    struct pHidd_BitMap_PutTemplate p;
    
    p.mID            = BitMapBase + moHidd_BitMap_PutTemplate;
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

#define HIDD_BM_PutAlphaTemplate(obj, gc, alpha, modulo, x, y, width, height, invertalpha) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutAlphaTemplate_(HiddBitMapBase, __obj, gc, alpha, modulo, x, y, width, height, invertalpha); })

static inline VOID     HIDD_BM_PutAlphaTemplate_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *alpha, ULONG modulo, WORD x, WORD y, WORD width, WORD height, BOOL invertalpha)
{
    struct pHidd_BitMap_PutAlphaTemplate p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutAlphaTemplate;
        

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

#define HIDD_BM_PutPattern(obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, x, y, width, height) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutPattern_(HiddBitMapBase, __obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, x, y, width, height); })

static inline VOID HIDD_BM_PutPattern_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pattern, WORD patternsrcx, WORD patternsrcy, WORD patternheight, WORD patterndepth, HIDDT_PixelLUT *patternlut, BOOL invertpattern, UBYTE *mask, ULONG maskmodulo, WORD masksrcx, WORD x, WORD y, WORD width, WORD height)
{
    struct pHidd_BitMap_PutPattern p;
    
    p.mID           = BitMapBase + moHidd_BitMap_PutPattern;
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

#define HIDD_BM_BlitColorExpansion(obj, gc, srcBitMap, srcX, srcY, destX, destY, width, height) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_BlitColorExpansion_(HiddBitMapBase, __obj, gc, srcBitMap, srcX, srcY, destX, destY, width, height); })

static inline VOID	 HIDD_BM_BlitColorExpansion_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, OOP_Object *srcBitMap, WORD srcX, WORD srcY, WORD destX, WORD destY, UWORD width, UWORD height)
{
    struct pHidd_BitMap_BlitColorExpansion p;
    
    p.mID = BitMapBase + moHidd_BitMap_BlitColorExpansion;
        

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

#define HIDD_BM_MapColor(obj, color) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_MapColor_(HiddBitMapBase, __obj, color); })

static inline HIDDT_Pixel HIDD_BM_MapColor_(OOP_MethodID BitMapBase, OOP_Object *obj, HIDDT_Color *color)
{
    struct pHidd_BitMap_MapColor p;
    
    p.mID = BitMapBase + moHidd_BitMap_MapColor;
    

    p.color	= color;
 
    
    return OOP_DoMethod(obj, &p.mID);
}

/***************************************************************/

#define HIDD_BM_UnmapPixel(obj, pixel, color) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_UnmapPixel_(HiddBitMapBase, __obj, pixel, color); })

static inline VOID HIDD_BM_UnmapPixel_(OOP_MethodID BitMapBase, OOP_Object *obj, HIDDT_Pixel pixel, HIDDT_Color *color)
{
    struct pHidd_BitMap_UnmapPixel p;
    
    p.mID = BitMapBase + moHidd_BitMap_UnmapPixel;
    

    p.pixel	= pixel;
    p.color	= color;

    OOP_DoMethod(obj, &p.mID);
}


/***************************************************************/

#define HIDD_BM_PutImageLUT(obj, gc, pixels, modulo, x, y, width, height, pixlut) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutImageLUT_(HiddBitMapBase, __obj, gc, pixels, modulo, x, y, width, height, pixlut); })

static inline VOID     HIDD_BM_PutImageLUT_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    struct pHidd_BitMap_PutImageLUT p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutImageLUT;
        

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

#define HIDD_BM_PutTranspImageLUT(obj, gc, pixels, modulo, x, y, width, height, pixlut, transparent) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutTranspImageLUT_(HiddBitMapBase, __obj, gc, pixels, modulo, x, y, width, height, pixlut, transparent); })

static inline VOID HIDD_BM_PutTranspImageLUT_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut, UBYTE transparent)
{
    struct pHidd_BitMap_PutTranspImageLUT p;
    
    p.mID         = BitMapBase + moHidd_BitMap_PutTranspImageLUT;
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

#define HIDD_BM_GetImageLUT(obj, pixels, modulo, x, y, width, height, pixlut) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_GetImageLUT_(HiddBitMapBase, __obj, pixels, modulo, x, y, width, height, pixlut); })

static inline VOID     HIDD_BM_GetImageLUT_(OOP_MethodID BitMapBase, OOP_Object *obj, UBYTE *pixels, ULONG modulo, WORD x, WORD y, WORD width, WORD height, HIDDT_PixelLUT *pixlut)
{
    struct pHidd_BitMap_GetImageLUT p;
    
    p.mID = BitMapBase + moHidd_BitMap_GetImageLUT;
        

    p.pixels	= pixels;
    p.modulo	= modulo;
    p.x		= x;
    p.y		= y;
    p.width	= width;
    p.height	= height;
    p.pixlut	= pixlut;

    OOP_DoMethod(obj, &p.mID);
}


#define HIDD_BM_BytesPerLine(obj, pixFmt, width) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_BytesPerLine_(HiddBitMapBase, __obj, pixFmt, width); })

static inline ULONG HIDD_BM_BytesPerLine_(OOP_MethodID BitMapBase, OOP_Object *obj, HIDDT_StdPixFmt pixFmt, UWORD width)
{
    struct pHidd_BitMap_BytesPerLine p;

    p.mID = BitMapBase + moHidd_BitMap_BytesPerLine;

    
    p.pixFmt	= pixFmt;
    p.width	= width;

    return OOP_DoMethod(obj, &p.mID);
     
}

/***************************************************************/


#define HIDD_BM_ConvertPixels(obj, srcPixels, srcPixFmt, srcMod, dstBuf, dstPixFmt, dstMod, width, height, pixlut) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_ConvertPixels_(HiddBitMapBase, __obj, srcPixels, srcPixFmt, srcMod, dstBuf, dstPixFmt, dstMod, width, height, pixlut); })

static inline VOID     HIDD_BM_ConvertPixels_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR *srcPixels, HIDDT_PixelFormat *srcPixFmt, ULONG srcMod, APTR *dstBuf, HIDDT_PixelFormat *dstPixFmt, ULONG dstMod, UWORD width, UWORD height, HIDDT_PixelLUT *pixlut)
{
    struct pHidd_BitMap_ConvertPixels p;
    
    p.mID = BitMapBase + moHidd_BitMap_ConvertPixels;
        
    
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

#define HIDD_BM_FillMemRect8(obj, dstBuf, minX, minY, maxX, maxY, dstMod, fill) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_FillMemRect8_(HiddBitMapBase, __obj, dstBuf, minX, minY, maxX, maxY, dstMod, fill); })

static inline VOID	HIDD_BM_FillMemRect8_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR dstBuf, WORD minX, WORD minY, WORD maxX, WORD maxY, ULONG dstMod, UBYTE fill)
{
    struct pHidd_BitMap_FillMemRect8 p;
    
    p.mID = BitMapBase + moHidd_BitMap_FillMemRect8;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, &p.mID);
}

#define HIDD_BM_FillMemRect16(obj, dstBuf, minX, minY, maxX, maxY, dstMod, fill) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_FillMemRect16_(HiddBitMapBase, __obj, dstBuf, minX, minY, maxX, maxY, dstMod, fill); })

static inline VOID	HIDD_BM_FillMemRect16_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR dstBuf, WORD minX, WORD minY, WORD maxX, WORD maxY, ULONG dstMod, UWORD fill)
{
    struct pHidd_BitMap_FillMemRect16 p;
    
    p.mID = BitMapBase + moHidd_BitMap_FillMemRect16;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, &p.mID);
}

#define HIDD_BM_FillMemRect24(obj, dstBuf, minX, minY, maxX, maxY, dstMod, fill) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_FillMemRect24_(HiddBitMapBase, __obj, dstBuf, minX, minY, maxX, maxY, dstMod, fill); })

static inline VOID	HIDD_BM_FillMemRect24_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR dstBuf, WORD minX, WORD minY, WORD maxX, WORD maxY, ULONG dstMod, ULONG fill)
{
    struct pHidd_BitMap_FillMemRect24 p;
    
    p.mID = BitMapBase + moHidd_BitMap_FillMemRect24;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, &p.mID);
}

#define HIDD_BM_FillMemRect32(obj, dstBuf, minX, minY, maxX, maxY, dstMod, fill) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_FillMemRect32_(HiddBitMapBase, __obj, dstBuf, minX, minY, maxX, maxY, dstMod, fill); })

static inline VOID	HIDD_BM_FillMemRect32_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR dstBuf, WORD minX, WORD minY, WORD maxX, WORD maxY, ULONG dstMod, ULONG fill)
{
    struct pHidd_BitMap_FillMemRect32 p;
    
    p.mID = BitMapBase + moHidd_BitMap_FillMemRect32;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    p.fill = fill;
    
    OOP_DoMethod(obj, &p.mID);
}


#define HIDD_BM_InvertMemRect(obj, dstBuf, minX, minY, maxX, maxY, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_InvertMemRect_(HiddBitMapBase, __obj, dstBuf, minX, minY, maxX, maxY, dstMod); })

static inline VOID	HIDD_BM_InvertMemRect_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR dstBuf, WORD minX, WORD minY, WORD maxX, WORD maxY, ULONG dstMod)
{
    struct pHidd_BitMap_InvertMemRect p;
    
    p.mID = BitMapBase + moHidd_BitMap_InvertMemRect;
        
    
    p.dstBuf = dstBuf;
    p.minX = minX;
    p.minY = minY;
    p.maxX = maxX;
    p.maxY = maxY;
    p.dstMod = dstMod;
    
    OOP_DoMethod(obj, &p.mID);
}


#define HIDD_BM_CopyMemBox8(obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_CopyMemBox8_(HiddBitMapBase, __obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_CopyMemBox8_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_CopyMemBox8 p;
    
    p.mID = BitMapBase + moHidd_BitMap_CopyMemBox8;
        
    
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

#define HIDD_BM_CopyMemBox16(obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_CopyMemBox16_(HiddBitMapBase, __obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_CopyMemBox16_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_CopyMemBox16 p;
    
    p.mID = BitMapBase + moHidd_BitMap_CopyMemBox16;
        
    
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

#define HIDD_BM_CopyMemBox24(obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_CopyMemBox24_(HiddBitMapBase, __obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_CopyMemBox24_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_CopyMemBox24 p;
    
    p.mID = BitMapBase + moHidd_BitMap_CopyMemBox24;
        
    
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

#define HIDD_BM_CopyMemBox32(obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_CopyMemBox32_(HiddBitMapBase, __obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_CopyMemBox32_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_CopyMemBox32 p;
    
    p.mID = BitMapBase + moHidd_BitMap_CopyMemBox32;
        
    
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

#define HIDD_BM_CopyLUTMemBox16(obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod, pixlut) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_CopyLUTMemBox16_(HiddBitMapBase, __obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod, pixlut); })

static inline VOID	HIDD_BM_CopyLUTMemBox16_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod, HIDDT_PixelLUT *pixlut)
{
    struct pHidd_BitMap_CopyLUTMemBox16 p;
    
    p.mID = BitMapBase + moHidd_BitMap_CopyLUTMemBox16;
        
    
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

#define HIDD_BM_CopyLUTMemBox24(obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod, pixlut) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_CopyLUTMemBox24_(HiddBitMapBase, __obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod, pixlut); })

static inline VOID	HIDD_BM_CopyLUTMemBox24_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod, HIDDT_PixelLUT *pixlut)
{
    struct pHidd_BitMap_CopyLUTMemBox24 p;
    
    p.mID = BitMapBase + moHidd_BitMap_CopyLUTMemBox24;
        
    
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

#define HIDD_BM_CopyLUTMemBox32(obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod, pixlut) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_CopyLUTMemBox32_(HiddBitMapBase, __obj, src, srcX, srcY, dst, dstX, dstY, width, height, srcMod, dstMod, pixlut); })

static inline VOID	HIDD_BM_CopyLUTMemBox32_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod, HIDDT_PixelLUT *pixlut)
{
    struct pHidd_BitMap_CopyLUTMemBox32 p;
    
    p.mID = BitMapBase + moHidd_BitMap_CopyLUTMemBox32;
        
    
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

#define HIDD_BM_PutMem32Image8(obj, src, dst, dstX, dstY, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMem32Image8_(HiddBitMapBase, __obj, src, dst, dstX, dstY, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_PutMem32Image8_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_PutMem32Image8 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMem32Image8;
    
    
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

#define HIDD_BM_PutMem32Image16(obj, src, dst, dstX, dstY, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMem32Image16_(HiddBitMapBase, __obj, src, dst, dstX, dstY, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_PutMem32Image16_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_PutMem32Image16 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMem32Image16;
    
    
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

#define HIDD_BM_PutMem32Image24(obj, src, dst, dstX, dstY, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMem32Image24_(HiddBitMapBase, __obj, src, dst, dstX, dstY, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_PutMem32Image24_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, APTR dst, WORD dstX, WORD dstY, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_PutMem32Image24 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMem32Image24;
    
    
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

#define HIDD_BM_GetMem32Image8(obj, src, srcX, srcY, dst, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_GetMem32Image8_(HiddBitMapBase, __obj, src, srcX, srcY, dst, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_GetMem32Image8_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_GetMem32Image8 p;
    
    p.mID = BitMapBase + moHidd_BitMap_GetMem32Image8;
    
    
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

#define HIDD_BM_GetMem32Image16(obj, src, srcX, srcY, dst, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_GetMem32Image16_(HiddBitMapBase, __obj, src, srcX, srcY, dst, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_GetMem32Image16_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_GetMem32Image16 p;
    
    p.mID = BitMapBase + moHidd_BitMap_GetMem32Image16;
    
    
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

#define HIDD_BM_GetMem32Image24(obj, src, srcX, srcY, dst, width, height, srcMod, dstMod) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_GetMem32Image24_(HiddBitMapBase, __obj, src, srcX, srcY, dst, width, height, srcMod, dstMod); })

static inline VOID	HIDD_BM_GetMem32Image24_(OOP_MethodID BitMapBase, OOP_Object *obj, APTR src, WORD srcX, WORD srcY, APTR dst, UWORD width, UWORD height, ULONG srcMod, ULONG dstMod)
{
    struct pHidd_BitMap_GetMem32Image24 p;
    
    p.mID = BitMapBase + moHidd_BitMap_GetMem32Image24;
    
    
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

#define HIDD_BM_PutMemTemplate8(obj, gc, Template, modulo, srcx, dst, dstMod, x, y, width, height, inverttemplate) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMemTemplate8_(HiddBitMapBase, __obj, gc, Template, modulo, srcx, dst, dstMod, x, y, width, height, inverttemplate); })

static inline VOID	HIDD_BM_PutMemTemplate8_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *Template, ULONG modulo, WORD srcx, APTR dst, ULONG dstMod, WORD x, WORD y, WORD width, WORD height, BOOL inverttemplate)
{
    struct pHidd_BitMap_PutMemTemplate8 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMemTemplate8;
    
    
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

#define HIDD_BM_PutMemTemplate16(obj, gc, Template, modulo, srcx, dst, dstMod, x, y, width, height, inverttemplate) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMemTemplate16_(HiddBitMapBase, __obj, gc, Template, modulo, srcx, dst, dstMod, x, y, width, height, inverttemplate); })

static inline VOID	HIDD_BM_PutMemTemplate16_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *Template, ULONG modulo, WORD srcx, APTR dst, ULONG dstMod, WORD x, WORD y, WORD width, WORD height, BOOL inverttemplate)
{
    struct pHidd_BitMap_PutMemTemplate16 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMemTemplate16;
    
    
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

#define HIDD_BM_PutMemTemplate24(obj, gc, Template, modulo, srcx, dst, dstMod, x, y, width, height, inverttemplate) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMemTemplate24_(HiddBitMapBase, __obj, gc, Template, modulo, srcx, dst, dstMod, x, y, width, height, inverttemplate); })

static inline VOID	HIDD_BM_PutMemTemplate24_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *Template, ULONG modulo, WORD srcx, APTR dst, ULONG dstMod, WORD x, WORD y, WORD width, WORD height, BOOL inverttemplate)
{
    struct pHidd_BitMap_PutMemTemplate24 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMemTemplate24;
    
    
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

#define HIDD_BM_PutMemTemplate32(obj, gc, Template, modulo, srcx, dst, dstMod, x, y, width, height, inverttemplate) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMemTemplate32_(HiddBitMapBase, __obj, gc, Template, modulo, srcx, dst, dstMod, x, y, width, height, inverttemplate); })

static inline VOID	HIDD_BM_PutMemTemplate32_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *Template, ULONG modulo, WORD srcx, APTR dst, ULONG dstMod, WORD x, WORD y, WORD width, WORD height, BOOL inverttemplate)
{
    struct pHidd_BitMap_PutMemTemplate32 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMemTemplate32;
    
    
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

#define HIDD_BM_PutMemPattern8(obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, dst, dstMod, x, y, width, height) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMemPattern8_(HiddBitMapBase, __obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, dst, dstMod, x, y, width, height); })

static inline VOID	HIDD_BM_PutMemPattern8_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pattern, WORD patternsrcx, WORD patternsrcy, WORD patternheight, WORD patterndepth, HIDDT_PixelLUT *patternlut, BOOL invertpattern, UBYTE *mask, ULONG maskmodulo, WORD masksrcx, APTR dst, ULONG dstMod, WORD x, WORD y, WORD width, WORD height)
{
    struct pHidd_BitMap_PutMemPattern8 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMemPattern8;
    
    
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
				 
#define HIDD_BM_PutMemPattern16(obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, dst, dstMod, x, y, width, height) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMemPattern16_(HiddBitMapBase, __obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, dst, dstMod, x, y, width, height); })

static inline VOID	HIDD_BM_PutMemPattern16_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pattern, WORD patternsrcx, WORD patternsrcy, WORD patternheight, WORD patterndepth, HIDDT_PixelLUT *patternlut, BOOL invertpattern, UBYTE *mask, ULONG maskmodulo, WORD masksrcx, APTR dst, ULONG dstMod, WORD x, WORD y, WORD width, WORD height)
{
    struct pHidd_BitMap_PutMemPattern16 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMemPattern16;
    
    
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

#define HIDD_BM_PutMemPattern24(obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, dst, dstMod, x, y, width, height) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMemPattern24_(HiddBitMapBase, __obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, dst, dstMod, x, y, width, height); })

static inline VOID	HIDD_BM_PutMemPattern24_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pattern, WORD patternsrcx, WORD patternsrcy, WORD patternheight, WORD patterndepth, HIDDT_PixelLUT *patternlut, BOOL invertpattern, UBYTE *mask, ULONG maskmodulo, WORD masksrcx, APTR dst, ULONG dstMod, WORD x, WORD y, WORD width, WORD height)
{
    struct pHidd_BitMap_PutMemPattern24 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMemPattern24;
    
    
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
	
#define HIDD_BM_PutMemPattern32(obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, dst, dstMod, x, y, width, height) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_PutMemPattern32_(HiddBitMapBase, __obj, gc, pattern, patternsrcx, patternsrcy, patternheight, patterndepth, patternlut, invertpattern, mask, maskmodulo, masksrcx, dst, dstMod, x, y, width, height); })

static inline VOID	HIDD_BM_PutMemPattern32_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *gc, UBYTE *pattern, WORD patternsrcx, WORD patternsrcy, WORD patternheight, WORD patterndepth, HIDDT_PixelLUT *patternlut, BOOL invertpattern, UBYTE *mask, ULONG maskmodulo, WORD masksrcx, APTR dst, ULONG dstMod, WORD x, WORD y, WORD width, WORD height)
{
    struct pHidd_BitMap_PutMemPattern32 p;
    
    p.mID = BitMapBase + moHidd_BitMap_PutMemPattern32;
    
    
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
			
#define HIDD_BM_SetColorMap(obj, colorMap) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_SetColorMap_(HiddBitMapBase, __obj, colorMap); })

static inline OOP_Object * HIDD_BM_SetColorMap_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *colorMap)
{
    struct pHidd_BitMap_SetColorMap p;
    
    p.mID = BitMapBase + moHidd_BitMap_SetColorMap;
        
    
    p.colorMap = colorMap;
    
    return(OOP_Object *)OOP_DoMethod(obj, &p.mID);
}


#define HIDD_BM_ObtainDirectAccess(obj, addressReturn, widthReturn, heightReturn, bankSizeReturn, memSizeReturn) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_ObtainDirectAccess_(HiddBitMapBase, __obj, addressReturn, widthReturn, heightReturn, bankSizeReturn, memSizeReturn); })

static inline BOOL HIDD_BM_ObtainDirectAccess_(OOP_MethodID BitMapBase, OOP_Object *obj, UBYTE **addressReturn, ULONG *widthReturn, ULONG *heightReturn, ULONG *bankSizeReturn, ULONG *memSizeReturn)
{
    struct pHidd_BitMap_ObtainDirectAccess p;

    p.mID            = BitMapBase + moHidd_BitMap_ObtainDirectAccess;        
    p.addressReturn  = addressReturn;
    p.widthReturn    = widthReturn;
    p.heightReturn   = heightReturn;
    p.bankSizeReturn = bankSizeReturn;
    p.memSizeReturn  = memSizeReturn;

    /* Clear this by default */
    *addressReturn = NULL;

    return OOP_DoMethod(obj, &p.mID);
}

#define HIDD_BM_ReleaseDirectAccess(obj) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_ReleaseDirectAccess_(HiddBitMapBase, __obj); })

static inline VOID HIDD_BM_ReleaseDirectAccess_(OOP_MethodID BitMapBase, OOP_Object *obj)
{
    struct pHidd_BitMap_ReleaseDirectAccess p;

    p.mID = BitMapBase + moHidd_BitMap_ReleaseDirectAccess;

    OOP_DoMethod(obj, &p.mID);
}

#define HIDD_BM_BitMapScale(obj, src, dest, bsa, gc) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_BitMapScale_(HiddBitMapBase, __obj, src, dest, bsa, gc); })

static inline VOID HIDD_BM_BitMapScale_(OOP_MethodID BitMapBase, OOP_Object *obj, OOP_Object *src, OOP_Object *dest, struct BitScaleArgs * bsa, OOP_Object *gc)
{
    struct pHidd_BitMap_BitMapScale p;
    
    p.mID = BitMapBase + moHidd_BitMap_BitMapScale;
    p.src = src;
    p.dst = dest;
    p.bsa = bsa;
    p.gc  = gc;

    OOP_DoMethod(obj, &p.mID);
}

#define HIDD_BM_SetRGBConversionFunction(obj, srcPixFmt, dstPixFmt, function) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_SetRGBConversionFunction_(HiddBitMapBase, __obj, srcPixFmt, dstPixFmt, function); })

static inline HIDDT_RGBConversionFunction HIDD_BM_SetRGBConversionFunction_(OOP_MethodID BitMapBase, OOP_Object *obj, HIDDT_StdPixFmt srcPixFmt, HIDDT_StdPixFmt dstPixFmt, HIDDT_RGBConversionFunction function)
{
    struct pHidd_BitMap_SetRGBConversionFunction p;

    p.mID       = BitMapBase + moHidd_BitMap_SetRGBConversionFunction;    
    p.srcPixFmt = srcPixFmt;
    p.dstPixFmt = dstPixFmt;
    p.function  = function;
    
    return(HIDDT_RGBConversionFunction) OOP_DoMethod(obj, &p.mID);    
}

#define HIDD_BM_UpdateRect(obj, x, y, width, height) \
	({OOP_Object *__obj = obj;\
	  HIDD_BM_UpdateRect_(HiddBitMapBase, __obj, x, y, width, height); })

static inline VOID HIDD_BM_UpdateRect_(OOP_MethodID BitMapBase, OOP_Object *obj, WORD x, WORD y, WORD width, WORD height)
{
    struct pHidd_BitMap_UpdateRect p;

    p.mID    = BitMapBase + moHidd_BitMap_UpdateRect;
    p.x      = x;
    p.y      = y;
    p.width  = width;
    p.height = height;

    OOP_DoMethod(obj, &p.mID);    
}

#if !defined(HiddGCBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddGCBase HIDD_GC_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_GC_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID GCMethodBase;

    if (!GCMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OCLASS(obj)->OOPBasePtr;

	GCMethodBase = OOP_GetMethodID(IID_Hidd_GC, 0);
    }

    return GCMethodBase;
}
#endif

/********* GC *****************************************/
#define HIDD_GC_SetClipRect(obj, x1, y1, x2, y2) \
	({OOP_Object *__obj = obj;\
	  HIDD_GC_SetClipRect_(HiddGCBase, __obj, x1, y1, x2, y2); })

static inline VOID HIDD_GC_SetClipRect_(OOP_MethodID GCBase, OOP_Object *obj, WORD x1, WORD y1, WORD x2, WORD y2)
{
    struct pHidd_GC_SetClipRect p;

    p.mID = GCBase + moHidd_GC_SetClipRect;        
    p.x1  = x1;
    p.y1  = y1;
    p.x2  = x2;
    p.y2  = y2;

    OOP_DoMethod(obj, &p.mID);    
}

#define HIDD_GC_UnsetClipRect(obj) \
	({OOP_Object *__obj = obj;\
	  HIDD_GC_UnsetClipRect_(HiddGCBase, __obj); })

static inline VOID HIDD_GC_UnsetClipRect_(OOP_MethodID GCBase, OOP_Object *obj)
{
    struct pHidd_GC_UnsetClipRect p;
    
    p.mID = GCBase + moHidd_GC_UnsetClipRect;
    
    OOP_DoMethod(obj, &p.mID);
}

#if !defined(HiddPlanarBMBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddPlanarBMBase HIDD_PlanarBM_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_PlanarBM_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID PlanarBMMethodBase;

    if (!PlanarBMMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OCLASS(obj)->OOPBasePtr;

	PlanarBMMethodBase = OOP_GetMethodID(IID_Hidd_PlanarBM, 0);
    }

    return PlanarBMMethodBase;
}
#endif

/********* PlanarBM **********************************/
#define HIDD_PlanarBM_SetBitMap(obj, bitMap) \
	({OOP_Object *__obj = obj;\
	  HIDD_PlanarBM_SetBitMap_(HiddPlanarBMBase, __obj, bitMap); })

static inline BOOL HIDD_PlanarBM_SetBitMap_(OOP_MethodID PlanarBMBase, OOP_Object *obj, struct BitMap *bitMap)
{
    struct pHidd_PlanarBM_SetBitMap p;
    
    p.mID    = PlanarBMBase + moHidd_PlanarBM_SetBitMap;
    p.bitMap = bitMap;

    return OOP_DoMethod(obj, &p.mID);
}

#define HIDD_PlanarBM_GetBitMap(obj, bitMap) \
	({OOP_Object *__obj = obj;\
	  HIDD_PlanarBM_GetBitMap_(HiddPlanarBMBase, __obj, bitMap); })

static inline BOOL HIDD_PlanarBM_GetBitMap_(OOP_MethodID PlanarBMBase, OOP_Object *obj, struct BitMap *bitMap)
{
    struct pHidd_PlanarBM_GetBitMap p;

    p.mID    = PlanarBMBase + moHidd_PlanarBM_GetBitMap;
    p.bitMap = bitMap;

    return OOP_DoMethod(obj, &p.mID);
}

/********* ColorMap *********************************/

#if !defined(HiddColorMapBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddColorMapBase HIDD_ColorMap_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_ColorMap_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID ColorMapMethodBase;

    if (!ColorMapMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OCLASS(obj)->OOPBasePtr;

	ColorMapMethodBase = OOP_GetMethodID(IID_Hidd_ColorMap, 0);
    }

    return ColorMapMethodBase;
}
#endif

#define HIDD_CM_SetColors(obj, colors, firstColor, numColors, pixFmt) \
	({OOP_Object *__obj = obj;\
	  HIDD_CM_SetColors_(HiddColorMapBase, __obj, colors, firstColor, numColors, pixFmt); })

static inline BOOL HIDD_CM_SetColors_(OOP_MethodID ColorMapBase, OOP_Object *obj, HIDDT_Color *colors, UWORD firstColor, UWORD numColors, OOP_Object *pixFmt)
{
    struct pHidd_ColorMap_SetColors p;
    
    p.mID        = ColorMapBase + moHidd_ColorMap_SetColors;
    p.colors	 = colors;
    p.firstColor = firstColor;
    p.numColors	 = numColors;
    p.pixFmt	 = pixFmt;
    
    return OOP_DoMethod(obj, &p.mID);
}

#define HIDD_CM_GetPixel(obj, pixelNo) \
	({OOP_Object *__obj = obj;\
	  HIDD_CM_GetPixel_(HiddColorMapBase, __obj, pixelNo); })

static inline HIDDT_Pixel HIDD_CM_GetPixel_(OOP_MethodID ColorMapBase, OOP_Object *obj, ULONG pixelNo)
{
    struct pHidd_ColorMap_GetPixel p;
    
    p.mID     = ColorMapBase + moHidd_ColorMap_GetPixel;
    p.pixelNo = pixelNo;
    
    return OOP_DoMethod(obj, &p.mID);
}


#define HIDD_CM_GetColor(obj, colorNo, colorReturn) \
	({OOP_Object *__obj = obj;\
	  HIDD_CM_GetColor_(HiddColorMapBase, __obj, colorNo, colorReturn); })

static inline BOOL HIDD_CM_GetColor_(OOP_MethodID ColorMapBase, OOP_Object *obj, ULONG colorNo, HIDDT_Color *colorReturn)
{
    struct pHidd_ColorMap_GetColor p;
    
    p.mID         = ColorMapBase + moHidd_ColorMap_GetColor;
    p.colorNo	  = colorNo;
    p.colorReturn = colorReturn;
    
    return OOP_DoMethod(obj, &p.mID);
}

#endif /* HIDD_GRAPHICS_INLINE_H */
