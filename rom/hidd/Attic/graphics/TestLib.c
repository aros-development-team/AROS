/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Test for gfxhidd.library
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
#include <math.h>

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

APTR   bm; /* bitMap */
APTR   gc; /* graphics context */
struct Object *obj;
struct DateStamp date1, date2;
ULONG  ticks;
ULONG  i, cnt = 500000;
ULONG  x, col;
float  w;


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
                        bm = HIDD_Graphics_CreateBitMap(640, 200, 5,
                               HIDD_Graphics_BitMap_Flags_Displayable, NULL, NULL);
      
                        if(bm)
                        {
                            gc = HIDD_Graphics_CreateGC(bm, NULL);
    
                            if(gc)
                            {
                                printf("Graphics context\n");
      
                                col = 0;
                                for(x = 0; x < 35; x++)
                                {
                                    HIDD_Graphics_WritePixelDirect(gc, x, 50, col);
                                    col++;
                                    if(col >= 32) col = 0;
                                }
      
                                printf("ReadPixel_Q:\n");
                                for(x = 0; x < 35; x++)
                                {
                                    printf("Col at (%li, %i) = %li\n", x, 50,
                                      HIDD_Graphics_ReadPixel_Q(gc, x, 50));
                                }
      
    
                                /* Test HIDD_Graphics_DrawLine_Q */
                                for(w = 0; w < 360; w = w + 10)
                                {
                        
                                    HIDD_Graphics_DrawLine_Q(gc, 160, 100,
                                                             (LONG) (160 + cos(M_PI*w/180)*130),
                                                             (LONG) (100 + sin(M_PI*w/180)* 80)
                                                            );
                                }
      
                                Delay(5*50);
                                HIDD_Graphics_DeleteGC(gc, NULL);
                            }
      
                            HIDD_Graphics_DeleteBitMap(bm, NULL);
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
