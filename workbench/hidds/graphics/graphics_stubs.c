/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Stubs for graphics, bitmap and gc hidd class
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
extern struct Library *OOPBase;

/***************************************************************/

Object * HIDD_Gfx_NewGC(Object *hiddGfx, ULONG gcType, struct TagItem *tagList)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_NewGC p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewGC);
        
    p.mID      = mid;
    p.gcType   = gcType;
    p.attrList = tagList;

    return((Object *) DoMethod(hiddGfx, (Msg) &p));
}
/***************************************************************/

void HIDD_Gfx_DisposeGC(Object *hiddGfx, Object *gc)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_DisposeGC p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeGC);
        
    p.mID    = mid;
    p.gc     = gc;

    DoMethod(hiddGfx, (Msg) &p);
}
/***************************************************************/

Object * HIDD_Gfx_NewBitMap(Object *hiddGfx, struct TagItem *tagList)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_NewBitMap p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewBitMap);
        
    p.mID      = mid;
    p.attrList = tagList;

    return((Object *) DoMethod(hiddGfx, (Msg) &p));
}
/***************************************************************/

void HIDD_Gfx_DisposeBitMap(Object *hiddGfx, Object *bitMap)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_DisposeBitMap p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeBitMap);
        
    p.mID    = mid;
    p.bitMap = bitMap;

    DoMethod(hiddGfx, (Msg) &p);
}
/***************************************************************/

