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

Object * NewGC(Object *hiddGfx, ULONG gcType, struct TagItem *tagList)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_NewGC p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewGC);
        
    p.mID      = mid;
/*    p.gcType   = gcType;*/
    p.attrList = tagList;

    return((Object *) DoMethod(hiddGfx, (Msg) &p));
}
/***************************************************************/

void DisposeGC(Object *hiddGfx, Object *gc)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_DisposeGC p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeGC);
        
    p.mID    = mid;
    p.gc     = gc;

    DoMethod(hiddGfx, (Msg) &p);
}
/***************************************************************/

Object * NewBitMap(Object *hiddGfx, struct TagItem *tagList)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_NewBitMap p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_NewBitMap);
        
    p.mID      = mid;
    p.attrList = tagList;

    return((Object *) DoMethod(hiddGfx, (Msg) &p));
}
/***************************************************************/

void DisposeBitMap(Object *hiddGfx, Object *bitMap)
{
    static MethodID mid = 0;
    struct pHidd_Gfx_DisposeBitMap p;
    
    if(!mid) mid = GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_DisposeBitMap);
        
    p.mID    = mid;
    p.bitMap = bitMap;

    DoMethod(hiddGfx, (Msg) &p);
}
/***************************************************************/

