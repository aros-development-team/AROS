
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

// ANIM-8
static const UWORD *Do8short(UWORD *pixel, UWORD *stop, const UWORD *ops, UWORD xormask, UWORD pitch)
{
    UWORD opcount = AROS_BE2WORD(*ops++);

    DFORMATS("[anim.datatype] %s()\n", __func__);

    while (opcount-- > 0)
    {
        UWORD op = AROS_BE2WORD(*ops++);
        if (op & 0x8000)
        { // Uniq op: copy data literally
            UWORD cnt = op & 0x7FFF;
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
            UWORD cnt = AROS_BE2WORD(*ops++);
            UWORD fill = *ops++;
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
    UWORD x;
    UBYTE p;

    DFORMATS("[anim.datatype] %s()\n", __func__);

    for (p = 0; p < bm->Depth; ++p)
    {
        ULONG ptr = AROS_BE2LONG(planes[p]);
        if (ptr == 0)
        { // No ops for this plane.
            continue;
        }
        const ULONG *ops = (const ULONG *)dlta + ptr;
        for (x = 0; x < numcols; ++x)
        {
            ULONG *pixel = (ULONG *)(bm->Planes[p] + (x << 2));
            ULONG *stop = (ULONG *)((IPTR)pixel + ((bm->Rows - 1) * pitch));
            if (x == numcols - 1 && lastisshort)
            {
                    Do8short((UWORD *)pixel, (UWORD *)stop, (UWORD *)ops, xormask, pitch / 2);
                    continue;
            }
            ULONG opcount = AROS_BE2LONG(*ops++);
            while (opcount-- > 0)
            {
                ULONG op = AROS_BE2LONG(*ops++);
                if (op & 0x80000000)
                { // Uniq op: copy data literally
                    ULONG cnt = op & 0x7FFFFFFF;
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
                    ULONG cnt = AROS_BE2LONG(*ops++);
                    ULONG fill = *ops++;
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
    UWORD x;
    UBYTE p;

    DFORMATS("[anim.datatype] %s()\n", __func__);

    for (p = 0; p < bm->Depth; ++p)
    {
        ULONG ptr = AROS_BE2LONG(planes[p]);
        if (ptr == 0)
        { // No ops for this plane.
            continue;
        }
        const UWORD *ops = (const UWORD *)((IPTR)dlta + ptr);
        for (x = 0; x < numcols; ++x)
        {
            UWORD *pixel = (UWORD *)((IPTR)bm->Planes[p] + (x << 1));
            UWORD *stop = (UWORD *)((IPTR)pixel + ((bm->Rows - 1) * pitch));
            ops = Do8short(pixel, stop, ops, xormask, pitch);
        }
    }

    return 0;
}
