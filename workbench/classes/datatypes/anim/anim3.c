
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

// ANIM 3
LONG generic_unpackshortdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    ULONG opptr;
    const UWORD *ops;
    UWORD *pixels, *stop;
    WORD offset;
    UWORD cnt;
    UBYTE p;

    DFORMATS("[anim.datatype] %s()\n", __func__)

    for (p = 0; p < bm->Depth; p++)
    {
        opptr = AROS_BE2LONG(lists[p]);
        DFORMATS("[anim.datatype] %s:   plane #%d @ 0x%p\n", __func__, p, bm->Planes[p])
        if ((opptr != 0) && (opptr < dltasize))
        {
            ops = (const UWORD *)((IPTR)dlta + opptr);
            pixels = (UWORD *)((IPTR)bm->Planes[p]);
            stop = (UWORD *)((IPTR)pixels + (bm->Rows * bm->BytesPerRow));
            while (*ops  != 0xFFFF)
            {
                offset = AROS_BE2WORD(*ops);
                ops++;

                if (offset >= 0)
                {
                    if ((pixels = (UWORD *)((IPTR)pixels + (offset << 1))) <= stop)
                        *pixels = *ops;
                    ops++;
                }
                else
                {
                    pixels = (UWORD *)((IPTR)pixels - ((offset + 2) << 1));
                    cnt = AROS_BE2WORD(*ops);
                    ops++;

                    while (cnt-- > 0)
                    {
                        pixels++;
                        if (pixels <= stop)
                            *pixels = *ops;
                        ops++;
                    }
                }
            }
        }
    }

    return 0;
}
