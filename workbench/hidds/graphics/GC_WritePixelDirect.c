/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: gc class WritePixelDirect implementation.
    Lang: english
*/

#include <aros/config.h>
#include <exec/types.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#undef  SDEBUG
#define SDEBUG 0
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef  BM
#define BM data


/*** BitMap::PutPixel() ********************************************/

VOID bitmap_putpixel(Class *cl, Object *obj, struct pHidd_BitMap_PutPixel *msg)
{
    struct HIDDBitMapData *data = INST_DATA(cl, obj);

    UBYTE *dest;
    UBYTE **plane;
    ULONG offset;
    ULONG mask;
    UBYTE pixel, notPixel;
    ULONG i;

    EnterFunc(bug("BitMap::WritePixelDirect()\n"));

    if(BM->format & vHidd_BitMap_Format_Chunky)
    /* bitmap in chunky-mode */
    {
        /* bitmap in chunky-mode */
        dest = BM->buffer + msg->x * BM->bytesPerPixel + msg->y * BM->bytesPerRow;

        switch(BM->bytesPerPixel)
        {
            case 1: *((UBYTE *) dest)   = (UBYTE) msg->val; break;
            case 2: *((UWORD *) dest)   = (UWORD) msg->val; break;
#warning FIXME: word acces on odd adress
            case 3: *((UBYTE *) dest++) = (UBYTE) msg->val >> 16; break;
                    *((UWORD *) dest  ) = (UWORD) msg->val; break;
            case 4: *((ULONG *) dest)   = (ULONG) msg->val; break;
        }
    }
    else
    {
         /* bitmap in plane-mode */
         plane     = (UBYTE **) BM->buffer;
         offset    = msg->x / 8 + msg->y * BM->bytesPerRow;
         pixel     = 128 >> (msg->x % 8);
         notPixel  = ~pixel;
         mask      = 1;

         for(i = 0; i < BM->depth; i++)
         {
             if(msg->val & mask)
             {
                 *(*plane + offset) = *(*plane + offset) | pixel;
             }
             else
             {
                 *(*plane + offset) = *(*plane + offset) & notPixel;
             }

             mask = mask << 1;
             plane++;
         }
    }

    ReturnVoid("BitMap::WritePixelDirect");
}
