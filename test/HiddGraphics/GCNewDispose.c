/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Test for graphics.hidd
    Lang: english
*/

/*****************************************************************************

    NAME

        GCNewDispose

    SYNOPSIS

        GCNewDispose HIDD/K,FG/N/K,BG/N/K

    LOCATION

        test/HiddGraphics

    FUNCTION

        Creates a gc, prints the attributes of the gc and dispose
        the gc

    INPUTS
        HIDD   - name of the hidd to use e.g. "graphics-X11.hidd"
                 (default: graphics.hdd)
        FG     - Set foreground color to this value (default: 1)
        BG     - Set background color to this value (default: 2)

    RESULT
        RETURN_OK    - hidd works
        RETURN_ERROR - hidd produce errors
        RETURN_FAIL  - could not test hidd i.e. OpenLibrary() fails

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define AROS_USE_OOP

#include <stdlib.h>
#include <stdio.h>

#include <aros/config.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <utility/tagitem.h>

#include <oop/oop.h>
#include <hidd/graphics.h>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>


#define B(b) (struct Library **) b

struct OpnLibs
{
    STRPTR libName;
    ULONG  version;
    struct Library **base;
};

struct DosLibrary    *DOSBase;
struct Library       *OOPBase;
struct Library       *HIDDGraphicsBase;
/***************************************************************/

struct OpnLibs LibsArray[] = {{"dos.library"      , 37, B(&DOSBase)         },
                              {AROSOOP_NAME       ,  0, B(&OOPBase)         },
                              {NULL,                 0, NULL          }
                             };

BOOL OpenLibs()
{
    ULONG  i = 0;
    BOOL  ok = TRUE;

    while(LibsArray[i].base)
    {
        *LibsArray[i++].base = NULL;
    }

    i = 0;

    while(LibsArray[i].libName && ok)
    {
        *LibsArray[i].base = OpenLibrary(LibsArray[i].libName, LibsArray[i].version);
        if(*LibsArray[i].base == NULL)
        {
            printf("Can't open library '%s' V%li!\n",
                   LibsArray[i].libName,
                   LibsArray[i].version
                  );
            ok = FALSE;
        }

        i++;
    }

    return ok;
}
/***************************************************************/

void CloseLibs()
{
    ULONG i    = 0;
    BOOL  quit = FALSE;

    while(LibsArray[i].base && !quit)
    {
        if(*LibsArray[i].base != NULL)
        {
            CloseLibrary(*LibsArray[i].base);
            i++;
        }
        else
        {
            quit = TRUE;
        }
    }
}
/***************************************************************/

ULONG GetAttr(Object *obj, ULONG attrID)
{
    static MethodID mid = 0;
    struct pRoot_Get p;
    ULONG  ret;

    if(!mid) mid = GetMethodID(IID_Root, moRoot_Get);
        
    p.mID     = mid;
    p.attrID  = attrID;
    p.storage = &ret;

    DoMethod(obj, (Msg)&p);

    return ret;
}
/***************************************************************/

Object * NewGC(Object *hiddGfx, ULONG gcType, struct TagItem *tagList)
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

int main(int argc, char **argv)
{
    ULONG ret = RETURN_FAIL;

    AttrBase HiddGfxAttrBase;
    AttrBase HiddGCAttrBase;
    AttrBase HiddBitMapAttrBase;

    Object   *gfxHidd;
    Object   *bitMap;
    Object   *gc;

    STRPTR hiddName = "graphics.hidd";
    ULONG  fg       = 1;
    ULONG  bg       = 2;

    /* ReadArgs() declarations                */
    /* each entry must have a size of 4 bytes */
    struct Args
    {
        STRPTR hiddName;
        IPTR   *fg;
        IPTR   *bg;
    };

    struct Args args = {hiddName, &fg, &bg};
    struct RDArgs *rda;


    if(OpenLibs())
    {
        rda = ReadArgs("HIDD/K,FG/N/K,BG/N/K", (IPTR *)&args, NULL);
        if (rda != NULL)
        {
            HIDDGraphicsBase = OpenLibrary(args.hiddName, 0);
            if(HIDDGraphicsBase)
            {
                ret = RETURN_ERROR;

                HiddGfxAttrBase    = ObtainAttrBase(IID_Hidd_Gfx);
                HiddBitMapAttrBase = ObtainAttrBase(IID_Hidd_BitMap);
                HiddGCAttrBase     = ObtainAttrBase(IID_Hidd_GCQuick);
        
                if(HiddGfxAttrBase && HiddBitMapAttrBase && HiddGCAttrBase)
                {
                    gfxHidd = NewObject(NULL, CLID_Hidd_Gfx, NULL);
                    if(gfxHidd)
                    {
                        bitMap = NewBitMap(gfxHidd, NULL);
                        if(bitMap)
                        {
                            struct TagItem gc_tags[] =
                            {
                                {aHidd_GC_BitMap,     (IPTR) bitMap},
                                {aHidd_GC_Foreground, (IPTR) *args.fg},
                                {aHidd_GC_Background, (IPTR) *args.bg},
                                {TAG_DONE, 0UL}
                            };

                            gc = NewGC(gfxHidd, HIDDV_Gfx_GCType_Quick, gc_tags);
                            if(gc)
                            {
                                printf("GC created:\n");
                                printf("  fg    : %li\n", GetAttr(gc, aHidd_GC_Foreground));
                                printf("  bg    : %li\n", GetAttr(gc, aHidd_GC_Background));
                                printf("  drMode: %li\n", GetAttr(gc, aHidd_GC_DrawMode));
                                printf("  bitMap: %li\n", GetAttr(gc, aHidd_GC_BitMap));
        
                                DisposeGC(gfxHidd, gc);
        
                                ret = RETURN_OK;
                            }
        
                            DisposeBitMap(gfxHidd, bitMap);
                        }

                        DisposeObject(gfxHidd);
                    } /* if(gfxHidd) */
                }  /* if(HiddGfxAttrBase && HiddBitMapAttrBase && HiddGCAttrBase) */

                if(HiddGCAttrBase)     ReleaseAttrBase(IID_Hidd_GCQuick);
                if(HiddBitMapAttrBase) ReleaseAttrBase(IID_Hidd_BitMap);
                if(HiddGfxAttrBase)    ReleaseAttrBase(IID_Hidd_Gfx);

                CloseLibrary(HIDDGraphicsBase);
            } /* if(HIDDGraphicsBase) */

            FreeArgs(rda);
        }
        else
        {
           PrintFault(IoErr(), "");
        }  /* if (rda != NULL) */

    } /* if OpenLibs() */

    CloseLibs();

    return(ret);
}
