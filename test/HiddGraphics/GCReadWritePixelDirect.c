/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Test for graphics.hidd
    Lang: english
*/

/*****************************************************************************

    NAME

        GCReadWritePixelDirect

    SYNOPSIS

        GCReadWritePixelDirect HIDD/K,WIDTH/N/K,HEIGHT/N/K,DEPTH/N/K,CHUNKY/S

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

    AttrBase HiddGCAttrBase;
    AttrBase HiddGfxAttrBase;
    AttrBase HiddBitMapAttrBase;

    struct pHidd_GC_ReadPixel        msg_ReadPixel;
    struct pHidd_GC_WritePixelDirect msg_WritePixel;

    Object   *gfxHidd;
    Object   *bitMap;
    Object   *gc;

    STRPTR hiddName = "graphics.hidd";
    ULONG  width    = 320;
    ULONG  height   = 200;
    ULONG  depth    = 8;
    ULONG  format   = HIDDV_BitMap_Format_Planar;

    WORD  x, y;
    ULONG val;

    struct Args
    {
        STRPTR hiddName;
        IPTR   *width;
        IPTR   *height;
        IPTR   *depth;
        IPTR   *chunky;
    };

    struct Args args = {hiddName, &width, &height, &depth, 0};
    struct RDArgs *rda;


    if(OpenLibs())
    {
        rda = ReadArgs("HIDD/K,WIDTH/N/K,HEIGHT/N/K,DEPTH/N/K,CHUNKY/S", (IPTR *)&args, NULL);
        if (rda != NULL)
        {
            if(args.chunky != 0)
            {
                format = HIDDV_BitMap_Format_Chunky;
            }

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
                        struct TagItem bm_tags[] =
                        {
                            {aHidd_BitMap_Width,  (IPTR) *args.width},
                            {aHidd_BitMap_Height, (IPTR) *args.height},
                            {aHidd_BitMap_Depth,  (IPTR) *args.depth},
                            {aHidd_BitMap_Format, (IPTR) format},
                            {TAG_DONE, 0UL}
                        };
    
                        bitMap = NewBitMap(gfxHidd, bm_tags);
                        if(bitMap)
                        {
                            struct TagItem gc_tags[] =
                            {
                                {aHidd_GC_BitMap,     (IPTR) bitMap},
                                {TAG_DONE, 0UL}
                            };
        
                            gc = NewGC(gfxHidd, HIDDV_Gfx_GCType_Quick, gc_tags);
                            if(gc)
                            {
                                msg_WritePixel.val = 0;
                                msg_WritePixel.mID = GetMethodID(IID_Hidd_GCQuick, moHidd_GC_WritePixelDirect);
                                msg_ReadPixel.mID  = GetMethodID(IID_Hidd_GCQuick, moHidd_GC_ReadPixel);
        
                                for(y = 0; y < 10; y++)
                                {
                                    for(x = 0; x < 10; x++)
                                    {
                                        printf("  x: %i y: %i val: %li ", x, y, msg_WritePixel.val);
                                        msg_WritePixel.x = x;
                                        msg_WritePixel.y = y;
                                        DoMethod(gc, (Msg) &msg_WritePixel);
          
                                        msg_ReadPixel.x = x;
                                        msg_ReadPixel.y = y;
                                        printf("ret: %li: ", (val = DoMethod(gc, (Msg) &msg_ReadPixel)));
                                        if(msg_WritePixel.val == DoMethod(gc, (Msg) &msg_ReadPixel))
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
        
                                DisposeGC(gfxHidd, gc);
        
                                ret = RETURN_OK;
                            }
        
                            DisposeBitMap(gfxHidd, bitMap);
                        }
    
                        DisposeObject(gfxHidd);
                    }
                }  /* if(HiddGfxAttrBase && HiddBitMapAttrBase && HiddGCAttrBase) */
    
                if(HiddGfxAttrBase)    ReleaseAttrBase(IID_Hidd_Gfx);
                if(HiddBitMapAttrBase) ReleaseAttrBase(IID_Hidd_BitMap);
                if(HiddGCAttrBase)     ReleaseAttrBase(IID_Hidd_GCQuick);

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
