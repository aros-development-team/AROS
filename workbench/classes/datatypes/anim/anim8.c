
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

// ANIM-8
static const UWORD *Do8short(UWORD *pixel, UWORD *stop, const UWORD *ops, UWORD xormask, UWORD pitch)
{
    UWORD opcount, cnt, fill;

    DFORMATS("[anim.datatype] %s()\n", __func__)

    opcount = AROS_BE2WORD(*ops);
    ops++;

    while (opcount-- > 0)
    {
        UWORD op = AROS_BE2WORD(*ops++);
        if (op & 0x8000)
        { // Uniq op: copy data literally
            cnt = op & 0x7FFF;
            while (cnt-- > 0)
            {
                if (pixel < stop)
                {
                    *pixel = (*pixel & xormask) ^ *ops;
                    pixel += pitch;
                }
                ops++;
            }
        }
        else if (op == 0)
        { // Same op: copy one byte to several rows
            cnt = AROS_BE2WORD(*ops);
            ops++;
            fill = *ops;
            ops++;
            while (cnt-- > 0)
            {
                if (pixel < stop)
                {
                    *pixel = (*pixel & xormask) ^ fill;
                    pixel += pitch;
                }
            }
        }
        else
        { // Skip op: Skip some rows
            pixel += op * pitch;
        }
    }
    return ops;
}

LONG generic_unpackanim8longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *planes = (const ULONG *)dlta;
    UWORD numcols = bm->BytesPerRow >> 2;
    UWORD pitch = bm->BytesPerRow;
    BOOL lastisshort = (GetBitMapAttr( bm, BMA_WIDTH) & 16) != 0;
    const ULONG xormask = (anhd->ah_Flags & ahfXOR) ? 0xFFFFFFFF : 0x00;
    const ULONG *ops;
    ULONG opcount, ptr, op, cnt, fill, *pixel, *stop;
    UWORD x;
    UBYTE p;

    DFORMATS("[anim.datatype] %s()\n", __func__)

    for (p = 0; p < bm->Depth; ++p)
    {
        ptr = AROS_BE2LONG(planes[p]);
        if ((ptr == 0) || (ptr > dltasize))
        { // No ops for this plane.
            continue;
        }
        ops = (const ULONG *)((IPTR)dlta + ptr);
        for (x = 0; x < numcols; ++x)
        {
            pixel = (ULONG *)((IPTR)bm->Planes[p] + (x << 2));
            stop = (ULONG *)((IPTR)pixel + ((bm->Rows - 1) * pitch));

            if (x == numcols - 1 && lastisshort)
            {
                    Do8short((UWORD *)pixel, (UWORD *)stop, (UWORD *)ops, xormask, pitch / 2);
                    continue;
            }
            opcount = AROS_BE2LONG(*ops);
            ops++;
            while (opcount-- > 0)
            {
                op = AROS_BE2LONG(*ops);
                ops++;
                if (op & 0x80000000)
                { // Uniq op: copy data literally
                    cnt = op & 0x7FFFFFFF;
                    while (cnt-- > 0)
                    {
                        if (pixel < stop)
                        {
                            *pixel = (*pixel & xormask) ^ *ops;
                            pixel = (ULONG *)((IPTR)pixel + pitch);
                        }
                        ops++;
                    }
                }
                else if (op == 0)
                { // Same op: copy one byte to several rows
                    cnt = AROS_BE2LONG(*ops);
                    ops++;
                    fill = *ops;
                    ops++;
                    while (cnt-- > 0)
                    {
                        if (pixel < stop)
                        {
                            *pixel = (*pixel & xormask) ^ fill;
                            pixel = (ULONG *)((IPTR)pixel + pitch);
                        }
                    }
                }
                else
                { // Skip op: Skip some rows
                    pixel = (ULONG *)((IPTR)pixel + (op * pitch));
                }
            }
        }
    }

    return 0;
}

LONG generic_unpackanim8worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *planes = (const ULONG *)dlta;
    UWORD numcols = bm->BytesPerRow >> 1;
    UWORD pitch = bm->BytesPerRow;
    const UWORD xormask = (anhd->ah_Flags & ahfXOR) ? 0xFFFF : 0x00;
    const UWORD *ops;
    UWORD x, *pixel, *stop;
    UBYTE p;

    DFORMATS("[anim.datatype] %s()\n", __func__)

    for (p = 0; p < bm->Depth; ++p)
    {
        ULONG ptr = AROS_BE2LONG(planes[p]);
        if ((ptr == 0) || (ptr > dltasize))
        { // No ops for this plane.
            continue;
        }
        ops = (const UWORD *)((IPTR)dlta + ptr);
        for (x = 0; x < numcols; ++x)
        {
            pixel = (UWORD *)((IPTR)bm->Planes[p] + (x << 1));
            stop = (UWORD *)((IPTR)pixel + ((bm->Rows - 1) * pitch));
            ops = Do8short(pixel, stop, ops, xormask, pitch);
        }
    }

    return 0;
}
