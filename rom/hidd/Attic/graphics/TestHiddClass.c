/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Test for GfxHiddClass
    Lang: english
*/

#define INTUI_V36_NAMES_ONLY

#include "gfxhidd_intern.h"
#undef IntuitionBase
#undef GfxBase
#undef GfxHiddBase
#undef DOSBase
#undef UtilityBase

#include <stdio.h>

#include <aros/config.h>

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gfxhidd.h>
#include <proto/graphics.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/intuition.h>

#include <utility/tagitem.h>

#include <hidd/graphics.h>

#include <proto/alib.h>
#include <proto/intuition.h>

struct DosLibrary    *DOSBase;
struct Library       *GfxBase;
struct IntuitionBase *IntuitionBase;
struct Library       *GfxHiddBase;

APTR   gc; /* graphics context */
struct Object *obj, *gcObj;
HIDDT_BitMap  bmObj;
ULONG  bm;
APTR   gc;
struct DateStamp date1, date2;
ULONG  ticks;
ULONG  i, cnt = 500000;
ULONG  x, col;

ULONG DiffStamp(struct DateStamp date1, struct DateStamp date2)
{
    ULONG ticks = 0;

    ticks = date2.ds_Tick - date1.ds_Tick;
    ticks = ticks + 50 * 60 * (date2.ds_Minute - date1.ds_Minute);

    return(ticks);
}


int main(int argc, char **argv)
{
    IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 37);
    if(IntuitionBase)
    {
  
        DOSBase = (struct DosLibrary *) OpenLibrary("dos.library", 37);
        if(DOSBase)
        {
  
            GfxHiddBase = (struct Library *) OpenLibrary("aros:libs/gfxhidd.library", 0);
            if(GfxHiddBase)
            {
                GfxBase = (struct Library *) OpenLibrary("graphics.library", 0);
                if(GfxBase)
                {
      
                    printf("Libs open\n");
          
                    obj = NewObject(NULL, GRAPHICSHIDD, TAG_END);
                    if(obj)
                    {
                        bmObj = (HIDDT_BitMap) DoMethod((ULONG *)obj,
                                   HIDDM_Graphics_Cmd, 0,
                                   HIDDV_Graphics_Cmd_CreateBitMap,
                                   HIDDA_BitMap_Width      , 640,
                                   HIDDA_BitMap_Height     , 200,
                                   HIDDA_BitMap_Depth      , 5,
                                   HIDDA_BitMap_Displayable, TRUE,
                                   TAG_END
                                );
      
                        if(bmObj)
                        {
      
                            bm = 0;
                            GetAttr(HIDDA_BitMap_BitMap, bmObj, &bm);
                            printf("%li\n", bm);
      
                            gcObj = NewObject(NULL, GRAPHICSHIDDGC,
                                              HIDDA_GC_BitMap, bm,
                                              TAG_END
                                             );
      
                            if(gcObj)
                            {
                                col = 0;
                                for(x = 0; x < 35; x++)
                                {
                                    DoMethod((ULONG *)gcObj, HIDDM_Graphics_GC_WritePixelDirect, x, 50, col);
                                    col++;
                                    if(col >= 32) col = 0;
                                }
      
                                printf("ReadPixel_Q:\n");
                                for(x = 0; x < 35; x++)
                                {
                                    printf("Col at (%li, %i) = %li\n", x, 50,
                                      DoMethod((ULONG *)gcObj, HIDDM_Graphics_GC_ReadPixel_Q, x, 50, col));
                                }
      
                                Delay(5*50);
                                DisposeObject(gcObj);
                            }
      
                            DisposeObject(bmObj);
                        }
      
                        DisposeObject(obj);
                    }
      
                    CloseLibrary((struct Library *)GfxBase);
      
                }

                CloseLibrary((struct Library *)GfxHiddBase);
            }
  
            CloseLibrary((struct Library *)DOSBase);
        }
  
        CloseLibrary((struct Library *)IntuitionBase);
    }
}

