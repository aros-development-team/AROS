/*
    (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: gc class ReadPixel implementation.
    Lang: english
*/

#include <aros/config.h>
#include <exec/types.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

#undef  BM
#define BM ((struct HIDDBitMapData *) gc->bitMap)


/*** GC::ReadPixel_Q() ****************************************************/

ULONG gc_readpixel_q(Class *cl, Object *obj, struct pHidd_GC_ReadPixel *msg)
{
    struct HIDDGCData *gc = INST_DATA(cl, obj);

    UBYTE *src;
    UBYTE **plane;
    ULONG offset;
    UBYTE pixel;
    ULONG retVal;
    ULONG i;

    EnterFunc(bug("GC::ReadPixel_Q() x: %i y: %i\n", msg->x, msg->y));

    if(BM->format & HIDDV_BitMap_Format_Chunky)
    {
        /* bitmap in chunky format */

        D(bug("Chunky\n"));

        src = BM->buffer + msg->x * BM->bytesPerPixel + msg->y * BM->bytesPerRow;

        switch(BM->bytesPerPixel)
        {
            case 1: retVal = ((ULONG) *((UBYTE *) src)); break;
            case 2: retVal = ((ULONG) *((UWORD *) src)); break;
            case 3: retVal = ((ULONG) (*((UBYTE *) src++) << 16) | *((UWORD *) src)); break;
            case 4: retVal = ((ULONG) *((ULONG *) src)); break;
        }
    }
    else
    {
         /* bitmap in planar format */

         D(bug("Planar\n"));

         plane     = (UBYTE **) BM->buffer;
         offset    = msg->x / 8 + msg->y * BM->bytesPerRow;
         pixel     = 128 >> msg->x % 8;
         retVal    = 0;

         for(i = 0; i < BM->depth; i++)
         {
             if(*(*plane + offset) & pixel)
             {
                 retVal = retVal | (1 << i);
             }

             plane++;
         }
    }

    ReturnInt("GC::ReadPixel_Q", ULONG, retVal);
}

