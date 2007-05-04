/*
    Copyright © 1998, The AROS Development Team. All rights reserved.
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

#include "gfxhiddtool.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

struct DosLibrary    *DOSBase;
struct Library       *OOPBase;
struct Library       *HIDDGraphicsBase;

struct ght_OpenLibs LibsArray[] =
{
    GHT_LIB("dos.library"      , 37, &DOSBase),
    GHT_LIB(AROSOOP_NAME       ,  0, &OOPBase),
    GHT_LIB(NULL               ,  0, NULL)
};
/***************************************************************/

int main(int argc, char **argv)
{
    ULONG ret = RETURN_FAIL;

    OOP_AttrBase HiddGfxAttrBase     = 0;
    OOP_AttrBase HiddGCAttrBase      = 0;
    OOP_AttrBase HiddBitMapAttrBase  = 0;

    OOP_Object   *gfxHidd;
    OOP_Object   *bitMap;
    OOP_Object   *gc;

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


    if(ght_OpenLibs(LibsArray))
    {
        rda = ReadArgs("HIDD/K,FG/N/K,BG/N/K", (IPTR *)&args, NULL);
        if (rda != NULL)
        {
            HIDDGraphicsBase = OpenLibrary(args.hiddName, 0);
            if(HIDDGraphicsBase)
            {
                ret = RETURN_ERROR;

                HiddGfxAttrBase    = OOP_ObtainAttrBase(IID_Hidd_Gfx);
                HiddBitMapAttrBase = OOP_ObtainAttrBase(IID_Hidd_BitMap);
                HiddGCAttrBase     = OOP_ObtainAttrBase(IID_Hidd_GC);
                if(HiddGfxAttrBase && HiddBitMapAttrBase && HiddGCAttrBase)
                {
                    gfxHidd = OOP_NewObject(NULL, args.hiddName, NULL);
                    if(gfxHidd)
                    {
                        bitMap = HIDD_Gfx_NewBitMap(gfxHidd, NULL);
                        if(bitMap)
                        {
                            struct TagItem gc_tags[] =
                            {
                                {aHidd_GC_BitMap,     (IPTR) bitMap},
                                {aHidd_GC_Foreground, (IPTR) *args.fg},
                                {aHidd_GC_Background, (IPTR) *args.bg},
                                {TAG_DONE, 0UL}
                            };

                            gc = HIDD_Gfx_NewGC(gfxHidd, gc_tags);
                            if(gc)
                            {
                                printf("GC created:\n");
                                printf("  fg    : %li\n", ght_GetAttr(gc, aHidd_GC_Foreground));
                                printf("  bg    : %li\n", ght_GetAttr(gc, aHidd_GC_Background));
                                printf("  drMode: %li\n", ght_GetAttr(gc, aHidd_GC_DrawMode));
                                printf("  bitMap: %li\n", ght_GetAttr(gc, aHidd_GC_BitMap));
        
                                HIDD_Gfx_DisposeGC(gfxHidd, gc);
        
                                ret = RETURN_OK;
                            }
        
                            HIDD_Gfx_DisposeBitMap(gfxHidd, bitMap);
                        }

                        if(gfxHidd) OOP_DisposeObject(gfxHidd);
                    } /* if(gfxHidd) */
                }  /* if(HiddGfxAttrBase && HiddBitMapAttrBase && HiddGCAttrBase) */

                if(HiddGCAttrBase)     OOP_ReleaseAttrBase(IID_Hidd_GC);
                if(HiddBitMapAttrBase) OOP_ReleaseAttrBase(IID_Hidd_BitMap);
                if(HiddGfxAttrBase)    OOP_ReleaseAttrBase(IID_Hidd_Gfx);

                CloseLibrary(HIDDGraphicsBase);
            } /* if(HIDDGraphicsBase) */

            FreeArgs(rda);
        }
        else
        {
           PrintFault(IoErr(), "");
        }  /* if (rda != NULL) */

    } /* if OpenLibs() */

    ght_CloseLibs(LibsArray);

    return(ret);
}
