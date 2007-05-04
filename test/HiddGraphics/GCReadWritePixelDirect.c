/*
    Copyright � 1998, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Test for graphics.hidd
    Lang: english
*/

/*****************************************************************************

    NAME

        GCReadWritePixelDirect

    SYNOPSIS

        GCReadWritePixelDirect HIDD/K,WIDTH/N/K,HEIGHT/N/K,DEPTH/N/K,CHUNKY/S,DISPLAYABLE=DP/S

    LOCATION

        test/HiddGraphics

    FUNCTION

        Creates a gc, writes and reads some pixels and disposes
        the gc.

    INPUTS
        HIDD   - name of the hidd to use e.g. "graphics-X11.hidd"
                 (default: graphics.hidd)
        WIDTH  - width of bitmap (default: 320)
        HEIGHT - height of bitmap (default: 200)
        DEPTH  - depth of bitmap (default: 8)
        CHUNKY - create bitmap in chunky-mode (default: planar)
        DISPLAYABLE - show bitmap (default: FALSE)

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
#include <hidd/graphics-amiga-intuition.h>

#include "gfxhiddtool.h"

#undef  SDEBUG
#define SDEBUG 0
#undef  DEBUG
#define DEBUG 0
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

    OOP_AttrBase HiddGCAttrBase;
    OOP_AttrBase HiddGfxAttrBase;
    OOP_AttrBase HiddBitMapAttrBase;

    struct pHidd_BitMap_GetPixel  msg_ReadPixel;
    struct pHidd_BitMap_PutPixel  msg_WritePixel;

    OOP_Object   *gfxHidd;
    OOP_Object   *bitMap;
    OOP_Object   *gc;

    STRPTR hiddName = "graphics.hidd";
    ULONG  width    = 320;
    ULONG  height   = 200;
    ULONG  depth    = 8;
    ULONG  format   = vHidd_BitMap_Format_Planar;

    WORD  x, y;
    ULONG val;
    char  wait;

    struct Args
    {
        STRPTR hiddName;
        IPTR   *width;
        IPTR   *height;
        IPTR   *depth;
        IPTR   *chunky;
        ULONG  displayable;
    };

    struct Args args = {hiddName, &width, &height, &depth, 0, 0};
    struct RDArgs *rda;


    if(ght_OpenLibs(LibsArray))
    {
        rda = ReadArgs("HIDD/K,WIDTH/N/K,HEIGHT/N/K,DEPTH/N/K,CHUNKY/S,DISPLAYABLE=DP/S", (IPTR *)&args, NULL);
        if (rda != NULL)
        {
            if(args.chunky      != 0) format           = vHidd_BitMap_Format_Chunky;
            if(args.displayable != 0) args.displayable = (ULONG) TRUE;

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
                        struct TagItem bm_tags[] =
                        {
                            {aHidd_BitMap_Width,       (IPTR) *args.width},
                            {aHidd_BitMap_Height,      (IPTR) *args.height},
                            {aHidd_BitMap_Depth,       (IPTR) *args.depth},
                            {aHidd_BitMap_Format,      (IPTR) format},
                            {aHidd_BitMap_Displayable, (IPTR) args.displayable},
                            {TAG_DONE, 0UL}
                        };
    
                        bitMap = HIDD_Gfx_NewBitMap(gfxHidd, bm_tags);
                        if(bitMap)
                        {
                            struct TagItem gc_tags[] =
                            {
                                {aHidd_GC_BitMap,     (IPTR) bitMap},
                                {TAG_DONE, 0UL}
                            };
        
                            gc = HIDD_Gfx_NewGC(gfxHidd, gc_tags);
                            if(gc)
                            {
                                msg_WritePixel.val = 0;
                                msg_WritePixel.mID = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_PutPixel);
                                msg_ReadPixel.mID  = GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_GetPixel);

                                for(x = 0; x < 30; x++)
                                {
                                    msg_WritePixel.x = x;
                                    msg_WritePixel.y = 1;
                                    OOP_DoMethod(gc, (OOP_Msg) &msg_WritePixel);
                                }

                                for(y = 1; y < 11; y++)
                                {
                                    for(x = 0; x < 10; x++)
                                    {
                                        printf("  x: %i y: %i val: %li ", x, y, msg_WritePixel.val);
                                        msg_WritePixel.x = x;
                                        msg_WritePixel.y = y;
                                        DoMethod(gc, (Msg) &msg_WritePixel);
          
                                        msg_ReadPixel.x = x;
                                        msg_ReadPixel.y = y;
                                        printf("ret: %li: ", (val = OOP_DoMethod(gc, (OOP_Msg) &msg_ReadPixel)));
                                        if(msg_WritePixel.val == OOP_DoMethod(gc, (OOP_Msg) &msg_ReadPixel))
                                        {
                                            printf("OK\n");
                                        }
                                        else
                                        {
                                            printf("ERROR\n");
                                            ret = RETURN_ERROR;
                                        }
        
                                        msg_WritePixel.val++;
                                    }
                                }

                                printf("Press enter to continue");
                                scanf("%c", &wait);

                                HIDD_Gfx_DisposeGC(gfxHidd, gc);
        
                                ret = RETURN_OK;
                            }
        
                            HIDD_Gfx_DisposeBitMap(gfxHidd, bitMap);
                        }
    
                        if(gfxHidd) OOP_DisposeObject(gfxHidd);
                    }
                }  /* if(HiddGfxAttrBase && HiddBitMapAttrBase && HiddGCAttrBase) */
    
                if(HiddGfxAttrBase)    OOP_ReleaseAttrBase(IID_Hidd_Gfx);
                if(HiddBitMapAttrBase) OOP_ReleaseAttrBase(IID_Hidd_BitMap);
                if(HiddGCAttrBase)     OOP_ReleaseAttrBase(IID_Hidd_GC);

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
