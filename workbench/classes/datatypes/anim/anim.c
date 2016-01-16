
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

struct AnimInstData;
struct FrameNode;

/* main includes */
#include "classbase.h"
#include "classdata.h"

#define DLTAHDR_SIZE 8

LONG LoadILBMBody( struct ClassBase *cb, struct BitMap *bm, struct BitMapHeader *bmh, UBYTE *data, ULONG len )
{
    const BYTE *src = (const BYTE *)data;
    //const BYTE *end = src + len;
    UBYTE nplanes = bmh->bmh_Depth, p;
    UWORD pitch = bm->BytesPerRow;
    UWORD out = 0;
    UWORD y, ofs;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    // The mask plane is interleaved after the bitmap planes, so we need to count
    // it as another plane when reading.
    if (bmh->bmh_Masking == mskHasMask)
        nplanes += 1;

    for (y = 0; y < bmh->bmh_Height; ++y)
    {
        for (p = 0; p < nplanes; ++p)
        {
            if (p < bmh->bmh_Depth)
            {
                // Read data into bitplane
                if (bmh->bmh_Compression == cmpNone)
                {
                    memcpy(&bm->Planes[p][out], src, pitch);
                    src += pitch;
                }
                else for (ofs = 0; ofs < pitch;)
                {
                    if (*src >= 0)
                    {
                        memcpy(&bm->Planes[p][out + ofs], src + 1, *src + 1);
                        ofs += *src + 1;
                        src += *src + 2;
                    }
                    else
                    {
                        memset(&bm->Planes[p][out + ofs], src[1], -*src + 1);
                        ofs += -*src + 1;
                        src += 2;
                    }
                }
            }
            else
            {
                // This is the mask plane. Skip over it.
                if (bmh->bmh_Compression == cmpNone)
                {
                    src += pitch;
                }
                else for (ofs = 0; ofs < pitch;)
                {
                    if (*src >= 0)
                    {
                        ofs += *src + 1;
                        src += *src + 2;
                    }
                    else
                    {
                        ofs += -*src + 1;
                        src += 2;
                    }
                }
            }
        }
        out += pitch;
    }

    return 0;
}

//ANIM-1

// ANIM 2
LONG unpacklongdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

// ANIM 3
LONG unpackshortdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

//ANIM-4
LONG unpackanim4longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

LONG unpackanim4worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

//ANIM 5
LONG unpackbytedelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    UWORD numcols = bm->BytesPerRow;
    UBYTE opptr;
    UBYTE mask = 0;
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
            stop = (UBYTE *)((IPTR)pixels + ((bm->Rows - 1) * bm->BytesPerRow));
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
                            *pixels = ((*pixels & mask) ^ *ops);
                            pixels = (UBYTE *)((IPTR)pixels + bm->BytesPerRow);
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
                            *pixels = ((*pixels & mask) ^ fill);
                            pixels = (UBYTE *)((IPTR)pixels + bm->BytesPerRow);
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

//ANIM-6

//ANIM-7
LONG unpackanim7longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    UWORD numcols = (bm->BytesPerRow >> 2);
    UWORD pitch = bm->BytesPerRow;
    ULONG opptr, dataptr;
    const ULONG *data;
    ULONG mask = 0;
    ULONG *pixels;
    ULONG *stop;
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
                            *pixels = ((*pixels & mask) ^ *data);
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
                            *pixels = ((*pixels & mask) ^ fill);
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

LONG unpackanim7worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *lists = (const ULONG *)dlta;
    UWORD numcols = bm->BytesPerRow >> 1;
    UWORD pitch = bm->BytesPerRow;
    ULONG mask = 0;
    UWORD *pixels;
    UWORD *stop;
    const UBYTE *ops;
    UBYTE p;
    UWORD x;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    for (p = 0; p < bm->Depth; ++p)
    {
        ULONG opptr = AROS_BE2LONG(lists[p]);
        if (opptr == 0)
        { // No ops for this plane.
            continue;
        }
        const UWORD *data = (const UWORD *)((const UBYTE *)dlta + AROS_BE2LONG(lists[p + 8]));
        const UBYTE *ops = (const UBYTE *)dlta + opptr;
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
                            *pixels = (*pixels & mask) ^ *data;
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
                            *pixels = (*pixels & mask) ^ fill;
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

//ANIM-8
static const UWORD *Do8short(UWORD *pixel, UWORD *stop, const UWORD *ops, UWORD xormask, int pitch)
{
    UWORD opcount = AROS_BE2WORD(*ops++);

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

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

LONG unpackanim8longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *planes = (const ULONG *)dlta;
    int numcols = (GetBitMapAttr( bm, BMA_WIDTH) + 31) / 32;
    int pitch = bm->BytesPerRow;
    BOOL lastisshort = (GetBitMapAttr( bm, BMA_WIDTH) & 16) != 0;
    const UWORD xormask = (anhd->ah_Operation & acmpXORILBM) ? 0xFF : 0x00;
    UWORD x;
    UBYTE p;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

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
            ULONG *pixel = (ULONG *)(bm->Planes[p] + x);
            ULONG *stop = (ULONG *)((UBYTE *)pixel + GetBitMapAttr( bm, BMA_HEIGHT) * pitch);
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
                            pixel = (ULONG *)((UBYTE *)pixel + pitch);
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
                            pixel = (ULONG *)((UBYTE *)pixel + pitch);
                        }
                    }
                }
                else
                { // Skip op: Skip some rows
                    pixel = (ULONG *)((UBYTE *)pixel + op * pitch);
                }
            }
        }
    }

    return 0;
}

LONG unpackanim8worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *planes = (const ULONG *)dlta;
    int numcols = (GetBitMapAttr( bm, BMA_WIDTH) + 15) / 16;
    int pitch = bm->BytesPerRow / 2;
    const UWORD xormask = (anhd->ah_Operation & acmpXORILBM) ? 0xFF : 0x00;
    UWORD x;
    UBYTE p;

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

    for (p = 0; p < bm->Depth; ++p)
    {
        ULONG ptr = AROS_BE2LONG(planes[p]);
        if (ptr == 0)
        { // No ops for this plane.
            continue;
        }
        const UWORD *ops = (const UWORD *)dlta + ptr;
        for (x = 0; x < numcols; ++x)
        {
            UWORD *pixel = (UWORD *)(bm->Planes[p] + x);
            UWORD *stop = pixel + GetBitMapAttr( bm, BMA_HEIGHT) * pitch;
            ops = Do8short(pixel, stop, ops, xormask, pitch);
        }
    }

    return 0;
}

//ANIM-I
LONG unpackanimidelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

// ANIM-J
LONG unpackanimjdelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}