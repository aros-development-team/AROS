
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

// ANIM-7
LONG generic_unpackanim7longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    UWORD numcols = (bm->BytesPerRow >> 2);
    UWORD pitch = bm->BytesPerRow;
    ULONG opptr, dataptr;
    const ULONG *data;
    const ULONG xormask = (anhd->ah_Flags & ahfXOR) ? 0xFFFFFFFF : 0x00;
    ULONG *pixels;
    ULONG *stop;
    const UBYTE *ops;
    UBYTE p;
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

        dataptr = AROS_BE2LONG(lists[p + 8]);
        if (dataptr == 0)
            data = NULL;
        else
            data = (const ULONG *)((IPTR)dlta + dataptr);
        ops = (const UBYTE *)((IPTR)dlta + opptr);
        for (x = 0; x < numcols; x++)
        {
            pixels = (ULONG *)((IPTR)bm->Planes[p] + (x << 2));
            stop = (ULONG *)((IPTR)pixels + ((bm->Rows - 1) * pitch));
            UBYTE opcount = *ops++;
            while (opcount-- > 0)
            {
                UBYTE op = *ops++;
                if (op & 0x80)
                {
                    // Uniq op: copy data literally
                    UBYTE cnt = op & 0x7F;
                    while (cnt-- > 0)
                    {
                        if (pixels <= stop)
                        {
                            *pixels = ((*pixels & xormask) ^ *data);
                            pixels = (ULONG *)((IPTR)pixels + pitch);
                        }
                        data++;
                    }
                }
                else if (op == 0)
                {
                    // Same op: copy one entry to several rows
                    UBYTE cnt = *ops++;
                    ULONG fill = *data;
                    data++;
                    while (cnt-- > 0)
                    {
                        if (pixels <= stop)
                        {
                            *pixels = ((*pixels & xormask) ^ fill);
                            pixels = (ULONG *)((IPTR)pixels + pitch);
                        }
                    }
                }
                else
                {
                    // Skip op: Skip some rows
                    pixels = (ULONG *)((IPTR)pixels + (op * pitch));
                }
            }
        }
    }

    return 0;
}

LONG generic_unpackanim7worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    UWORD numcols = bm->BytesPerRow >> 1;
    UWORD pitch = bm->BytesPerRow;
    const UWORD xormask = (anhd->ah_Flags & ahfXOR) ? 0xFFFF : 0x00;
    UWORD *pixels;
    UWORD *stop;
    const UBYTE *ops;
    UBYTE p;
    UWORD x;

    DFORMATS("[anim.datatype] %s()\n", __func__)

    for (p = 0; p < bm->Depth; ++p)
    {
        ULONG opptr = AROS_BE2LONG(lists[p]);
        if (opptr == 0)
        { // No ops for this plane.
            continue;
        }
        const UWORD *data = (const UWORD *)((const UBYTE *)dlta + AROS_BE2LONG(lists[p + 8]));
        ops = (const UBYTE *)dlta + opptr;
        for (x = 0; x < numcols; ++x)
        {
            pixels = (UWORD *)((IPTR)bm->Planes[p] + (x << 1));
            stop = (UWORD *)((IPTR)pixels + ((bm->Rows - 1) * pitch));
            UBYTE opcount = *ops++;
            while (opcount-- > 0)
            {
                UBYTE op = *ops++;
                if (op & 0x80)
                {
                    // Uniq op: copy data literally
                    UBYTE cnt = op & 0x7F;
                    while (cnt-- > 0)
                    {
                        if (pixels < stop)
                        {
                            *pixels = (*pixels & xormask) ^ *data;
                            pixels = (UWORD *)((IPTR)pixels + pitch);
                        }
                        data++;
                    }
                }
                else if (op == 0)
                {
                    // Same op: copy one byte to several rows
                    UBYTE cnt = *ops++;
                    UWORD fill = *data;
                    data++;
                    while (cnt-- > 0)
                    {
                        if (pixels < stop)
                        {
                            *pixels = (*pixels & xormask) ^ fill;
                            pixels = (UWORD *)((IPTR)pixels + pitch);
                        }
                    }
                }
                else
                {
                    // Skip op: Skip some rows
                    pixels = (UWORD *)((IPTR)pixels + (op * pitch));
                }
            }
        }
    }

    return 0;
}
