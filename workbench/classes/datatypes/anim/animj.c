
/*
**
**  $VER: anim.c 1.12 (12.11.97)
**  anim.datatype 1.12
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

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    while ( dlta < (UBYTE *)((IPTR)dlta + dltasize))
    {
        opmode = AROS_BE2WORD(*((UWORD *)dlta) );
        dlta += sizeof(UWORD);

        switch ( opmode )
        {
        case 0:
            D(bug("[anim.datatype] %s: end of dlta\n", __PRETTY_FUNCTION__));
            dltaend = TRUE;
            break;

        case 1:
            D(bug("[anim.datatype] %s: column mode\n", __PRETTY_FUNCTION__));
            xormask     = AROS_BE2WORD( *((UWORD *)dlta) );
            opheight    = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 2)) );
            opcnt       = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 4)) );
            opwidth     = 1;
            dlta        += 6;
            break;

        case 2:
            D(bug("[anim.datatype] %s: area mode\n", __PRETTY_FUNCTION__));
            xormask     = AROS_BE2WORD( *((UWORD *)dlta) );
            opheight    = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 2)) );
            opwidth     = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 4)) );
            opcnt       = AROS_BE2WORD( *((UWORD *)((IPTR)dlta + 6)) );
            dlta        += 8;
            break;

        default:
            return 0;
        }

        if (dltaend) break;

        for ( op = 0; op < opcnt; op++ )
        {
            planeoffset = AROS_BE2WORD( *((UWORD *)dlta) );
            dlta += sizeof(UWORD);

            for ( y = 0; y < opheight; y++ )
            {
                for ( p = 0; p < bm->Depth; p++ )
                {
                    pixel = (UBYTE *)((IPTR)bm->Planes[p] + (planeoffset) + (y * pitch));
                    src = (UBYTE *)((IPTR)deltabm->Planes[p] + (planeoffset) + (y * pitch));
                    for ( x = 0; x < opwidth; x++ )
                    {
                        pixel[ x ] = (src[x] & xormask) ^ *dlta++;
                    }

                }
            }
        }

        /* skip odd byte */
        if (( opcnt * opheight * opwidth * bm->Depth ) & 1 ) ++dlta;
    }
    return 0;
}
