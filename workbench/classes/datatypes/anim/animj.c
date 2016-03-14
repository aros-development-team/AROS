
/*
**
** $Id$
**  anim.datatype 41.8
**
** These are "generic" software implementations of the unpacking code
** and are meant for reference. Arch specific "optimised" versions should be set
** in the class init code (see classbase.c)
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

struct ClassBase;
struct AnimInstData;
struct FrameNode;

/* main includes */
#include "classbase.h"
#include "classdata.h"

// ANIM-J
LONG generic_unpackanimjdelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm )
{
    UBYTE *pixel, *src;
    UWORD opmode, op, opcnt, opheight, opwidth, planeoffset, xormask;
    UWORD pitch = bm->BytesPerRow;
    BOOL dltaend = FALSE;
    UWORD x,y;
    UBYTE p;

    DFORMATS("[anim.datatype] %s()\n", __func__)

    while ( dlta < (UBYTE *)((IPTR)dlta + dltasize))
    {
        opmode = AROS_BE2WORD(*((UWORD *)dlta) );
        dlta = (UBYTE *)((IPTR)dlta + sizeof(UWORD));

        switch ( opmode )
        {
        case 0:
            DFORMATS("[anim.datatype] %s: end of dlta\n", __func__)
            dltaend = TRUE;
            break;

        case 1:
            DFORMATS("[anim.datatype] %s: column mode\n", __func__)
            xormask     = AROS_BE2WORD( *((UWORD *)dlta) );
            DFORMATS("[anim.datatype] %s:     xor mask : %04x\n", __func__, xormask)
            opheight    = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 2)) );
            opcnt       = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 4)) );
            DFORMATS("[anim.datatype] %s:     %d operation(s), %d rows\n", __func__, opcnt, opheight)
            opwidth     = 1;
            dlta        = (UBYTE *)((IPTR)dlta + 6);
            break;

        case 2:
            DFORMATS("[anim.datatype] %s: area mode\n", __func__)
            xormask     = AROS_BE2WORD( *((UWORD *)dlta) );
            DFORMATS("[anim.datatype] %s:     xor mask : %04x\n", __func__, xormask)
            opheight    = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 2)) );
            opwidth     = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 4)) );
            opcnt       = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 6)) );
            DFORMATS("[anim.datatype] %s:     %d operation(s), %dx%d\n", __func__, opcnt, opwidth, opheight)
            dlta        = (UBYTE *)((IPTR)dlta + 8);
            break;

        default:
            return 0;
        }

        if (dltaend) break;

        for ( op = 0; op < opcnt; op++ )
        {
            planeoffset = AROS_BE2WORD( *((UWORD *)dlta) );
            dlta = (UBYTE *)((IPTR)dlta + sizeof(UWORD));

            for ( y = 0; y < opheight; y++ )
            {
                for ( p = 0; p < bm->Depth; p++ )
                {
                    pixel = (UBYTE *)((IPTR)bm->Planes[p] + (planeoffset) + (y * pitch));
                    src = (UBYTE *)((IPTR)deltabm->Planes[p] + (planeoffset) + (y * pitch));
                    for ( x = 0; x < opwidth; x++ )
                    {
                        if (xormask)
                            pixel[ x ] = src[x] ^ *dlta;
                        else
                            pixel[ x ] = *dlta;
                        dlta++;
                    }

                }
            }
        }

        /* skip odd byte */
        if (( opcnt * opheight * opwidth * bm->Depth ) & 1 )
            dlta++;
    }
    return 0;
}
