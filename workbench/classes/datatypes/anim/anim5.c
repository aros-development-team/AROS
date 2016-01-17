
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

// ANIM 5
LONG generic_unpackbytedelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    UWORD numcols = bm->BytesPerRow;
    UWORD pitch = bm->BytesPerRow;
    UBYTE opptr;
    const UBYTE xormask = (anhd->ah_Flags & ahfXOR) ? 0xFF : 0x00;
    UBYTE *pixels;
    UBYTE *stop;
    const UBYTE *ops;
    UBYTE p;
    UWORD x;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    D(bug("[anim.datatype] %s: dlta @ 0x%p\n", __PRETTY_FUNCTION__, dlta));
    D(bug("[anim.datatype] %s: lists @ 0x%p\n", __PRETTY_FUNCTION__, lists));

    for (p = 0; p < bm->Depth; p++)
    {
        opptr = AROS_BE2LONG(lists[p]);
        D(bug("[anim.datatype] %s:   plane #%d @ 0x%p\n", __PRETTY_FUNCTION__, p, bm->Planes[p]));
        if ((opptr == 0) || (opptr > dltasize))
        {
            // No ops for this plane or invalid pointer.
            D(bug("[anim.datatype] %s: no ops/invalid op ptr (0x%08x)\n", __PRETTY_FUNCTION__, opptr));
            continue;
        }
        ops = (const UBYTE *)((IPTR)dlta + opptr);
        for (x = 0; x < numcols; x++)
        {
            pixels = (UBYTE *)((IPTR)bm->Planes[p] + x);
            stop = (UBYTE *)((IPTR)pixels + ((bm->Rows - 1) * pitch));
            BYTE opcount = *ops++;
            while (opcount-- > 0)
            {
                UBYTE op = *ops++;
                if (op & 0x80)
                { // Uniq op: copy data literally
                    UBYTE cnt = op & 0x7F;
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
                    UBYTE cnt = *ops++;
                    UBYTE fill = *ops++;

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
                    pixels = (UBYTE *)((IPTR)pixels + (op * bm->BytesPerRow));
                }
            }
        }
    }

    return 0;
}
