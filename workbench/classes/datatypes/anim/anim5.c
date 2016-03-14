
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

// ANIM 5
LONG generic_unpackbytedelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    UWORD numcols = bm->BytesPerRow;
    UWORD pitch = bm->BytesPerRow;
    ULONG opptr;
    const UBYTE xormask = (anhd->ah_Flags & ahfXOR) ? 0xFF : 0x00;
    UBYTE *pixels, *stop;
    const UBYTE *ops;
    UBYTE p, opcount, op, cnt, fill;
    UWORD x;

    DFORMATS("[anim.datatype] %s()\n", __func__)

    DFORMATS("[anim.datatype] %s: dlta @ 0x%p\n", __func__, dlta)
    DFORMATS("[anim.datatype] %s: lists @ 0x%p\n", __func__, lists)

    for (p = 0; p < bm->Depth; p++)
    {
        opptr = AROS_BE2LONG(lists[p]);
        DFORMATS("[anim.datatype] %s:   plane #%d @ 0x%p\n", __func__, p, bm->Planes[p])
        if ((opptr == 0) || (opptr > dltasize))
        {
            // No ops for this plane or invalid pointer.
            DFORMATS("[anim.datatype] %s: no ops/invalid op ptr (0x%08x)\n", __func__, opptr)
            continue;
        }
        ops = (const UBYTE *)((IPTR)dlta + opptr);
        for (x = 0; x < numcols; x++)
        {
            pixels = (UBYTE *)((IPTR)bm->Planes[p] + x);
            stop = (UBYTE *)((IPTR)pixels + ((bm->Rows - 1) * pitch));
            opcount = *ops++;
            while (opcount-- > 0)
            {
                op = *ops++;
                if (op & 0x80)
                { // Uniq op: copy data literally
                    cnt = op & 0x7F;
                    while (cnt-- > 0)
                    {
                        if (pixels <= stop)
                        {
                            *pixels = ((*pixels & xormask) ^ *ops);
                            pixels = (UBYTE *)((IPTR)pixels + pitch);
                        }
                        ops++;
                    }
                }
                else if (op == 0)
                { // Same op: copy one entry to several rows
                    cnt = *ops;
                    ops++;
                    fill = *ops;
                    ops++;

                    while (cnt-- > 0)
                    {
                        if (pixels <= stop)
                        {
                            *pixels = ((*pixels & xormask) ^ fill);
                            pixels = (UBYTE *)((IPTR)pixels + pitch);
                        }
                    }
                }
                else
                { // Skip op: Skip some rows
                    pixels = (UBYTE *)((IPTR)pixels + (op * pitch));
                }
            }
        }
    }

    return 0;
}
