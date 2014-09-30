/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_USE_OOP

#include <stdio.h>
#include <strings.h>

#include <aros/config.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <utility/tagitem.h>

#include <oop/oop.h>
#include <hidd/graphics.h>

#include "gfxhiddtool.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

extern struct Library    *OOPBase;
/***************************************************************/

BOOL ght_OpenLibs(struct ght_OpenLibs *libsArray)
{
    ULONG  i = 0;
    BOOL  ok = TRUE;

    while(libsArray[i].base)
    {
        *libsArray[i++].base = NULL;
    }

    i = 0;

    while(libsArray[i].libName && ok)
    {
        *libsArray[i].base = OpenLibrary(libsArray[i].libName, libsArray[i].version);
        if(*libsArray[i].base == NULL)
        {
            printf("Can't open library '%s' V%li!\n",
                   libsArray[i].libName,
                   libsArray[i].version
                  );
            ok = FALSE;
        }

        i++;
    }

    return ok;
}
/***************************************************************/

void ght_CloseLibs(struct ght_OpenLibs *libsArray)
{
    ULONG i    = 0;
    BOOL  quit = FALSE;

    while(libsArray[i].base && !quit)
    {
        if(*libsArray[i].base != NULL)
        {
            CloseLibrary(*libsArray[i].base);
            i++;
        }
        else
        {
            quit = TRUE;
        }
    }
}
/***************************************************************/

ULONG ght_GetAttr(Object *obj, ULONG attrID)
{
    ULONG  ret;

    GetAttr(obj, attrID, &ret);
    return ret;
}
/***************************************************************/

OOP_Object * NewGC(OOP_Object *hiddGfx, ULONG gcType, struct TagItem *tagList)
{
    static OOP_MethodID mid = 0;
    struct pHidd_Gfx_NewGC p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewGC);
        
    p.mID      = mid;
/*    p.gcType   = gcType;*/
    p.attrList = tagList;

    return((OOP_Object *) OOP_DoMethod(hiddGfx, (OOP_Msg) &p));
}
/***************************************************************/

void DisposeGC(OOP_Object *hiddGfx, OOP_Object *gc)
{
    static OOP_MethodID mid = 0;
    struct pHidd_Gfx_DisposeGC p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeGC);
        
    p.mID    = mid;
    p.gc     = gc;

    OOP_DoMethod(hiddGfx, (OOP_Msg) &p);
}
/***************************************************************/

OOP_Object * NewBitMap(OOP_Object *hiddGfx, struct TagItem *tagList)
{
    static OOP_MethodID mid = 0;
    struct pHidd_Gfx_NewBitMap p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewBitMap);
        
    p.mID      = mid;
    p.attrList = tagList;

    return((OOP_Object *) OOP_DoMethod(hiddGfx, (OOP_Msg) &p));
}
/***************************************************************/

void DisposeBitMap(OOP_Object *hiddGfx, OOP_Object *bitMap)
{
    static OOP_MethodID mid = 0;
    struct pHidd_Gfx_DisposeBitMap p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeBitMap);
        
    p.mID    = mid;
    p.bitMap = bitMap;

    OOP_DoMethod(hiddGfx, (OOP_Msg) &p);
}
/***************************************************************/

