/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Test for graphics.hidd
    Lang: english
*/

/*****************************************************************************

    NAME

        BitMapNewDispose

    SYNOPSIS

        BitMapNewDispose WIDTH/N/K,HEIGHT/N/K,DEPTH/N/K,CHUNKY/S

    LOCATION

        test/HiddGraphics

    FUNCTION

        Creates a bitmap, prints the attributes of the bitmap and dispose
        the bitmap.

    INPUTS
        WIDTH  - width of bitmap
        HEIGHT - height of bitmap
        DEPTH  - depth of bitmap
        CHUNKY - create bitmap in chunky-mode(default: planar)

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
struct Library       *HiddGraphicsBase;
/***************************************************************/

struct OpnLibs LibsArray[] = {{"dos.library"      , 37, B(&DOSBase)         },
                              {AROSOOP_NAME       ,  0, B(&OOPBase)         },
                              {"graphics.hidd"    ,  0, B(&HiddGraphicsBase)},

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

int main(int argc, char **argv)
{
    ULONG ret = RETURN_FAIL;

    AttrBase HiddGCAttrBase;
    AttrBase HiddGfxAttrBase;
    AttrBase HiddBitMapAttrBase;

    Object   *bitMap;

    ULONG width  = 320;
    ULONG height = 200;
    ULONG depth  = 4;
    ULONG format = HIDDV_BitMap_Format_Planar;

    /* ReadArgs() declarations                */
    /* each entry must have a size of 4 bytes */
    struct Args
    {
        IPTR  *width;
        IPTR  *height;
        IPTR  *depth;
        BOOL  chunky;
    };

    struct Args args = {&width, &height, &depth, 0};
    struct RDArgs *rda;


    if(OpenLibs())
    {
        ret = RETURN_ERROR;

        rda = ReadArgs("WIDTH/N/K,HEIGHT/N/K,DEPTH/N/K,CHUNKY/S", (IPTR *)&args, NULL);
        if (rda != NULL)
        {
            if(args.chunky != 0)
            {
                format = HIDDV_BitMap_Format_Chunky;
            }

            HiddBitMapAttrBase = GetAttrBase(IID_Hidd_BitMap);
    
            if(TRUE)
            {

                struct TagItem tags[] =
                {
                    {aHidd_BitMap_Width,  (IPTR) *args.width},
                    {aHidd_BitMap_Height, (IPTR) *args.height},
                    {aHidd_BitMap_Depth,  (IPTR) *args.depth},
                    {aHidd_BitMap_Format, (IPTR) format},
                    {TAG_DONE, 0UL}
                };
    
    
                bitMap = NewObject(NULL, CLID_Hidd_BitMap, tags);
                if(bitMap)
                {
                    printf("BitMap created:\n");
                    printf("  width      : %li\n", GetAttr(bitMap, aHidd_BitMap_Width));
                    printf("  height     : %li\n", GetAttr(bitMap, aHidd_BitMap_Height));
                    printf("  depth      : %li\n", GetAttr(bitMap, aHidd_BitMap_Depth));
                    printf("  format     : %li\n", GetAttr(bitMap, aHidd_BitMap_Format));
                    printf("  displayable: %li\n", GetAttr(bitMap, aHidd_BitMap_Displayable));
        
                    DisposeObject(bitMap);
        
                    ret = RETURN_OK;
                }
            }  /* if(TRUE) */

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
