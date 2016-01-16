
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

LONG generic_unpackilbmbody( struct ClassBase *cb, struct BitMap *bm, struct BitMapHeader *bmh, UBYTE *data, ULONG len )
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
LONG generic_unpacklongdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

// ANIM 3
LONG generic_unpackshortdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

//ANIM-4
LONG generic_unpackanim4longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

LONG generic_unpackanim4worddelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize, ULONG flags )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

//ANIM 5
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

//ANIM-6

//ANIM-7
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

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

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

//ANIM-8
static const UWORD *Do8short(UWORD *pixel, UWORD *stop, const UWORD *ops, UWORD xormask, UWORD pitch)
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

LONG generic_unpackanim8longdelta(struct AnimHeader *anhd, struct BitMap *bm, UBYTE *dlta, ULONG dltasize )
{
    const ULONG *planes = (const ULONG *)dlta;
    UWORD numcols = bm->BytesPerRow >> 2;
    UWORD pitch = bm->BytesPerRow;
    BOOL lastisshort = (GetBitMapAttr( bm, BMA_WIDTH) & 16) != 0;
    const ULONG xormask = (anhd->ah_Flags & ahfXOR) ? 0xFFFFFFFF : 0x00;
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

    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));

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

//ANIM-I
LONG generic_unpackanimidelta(struct AnimHeader *anhd, struct ClassBase *cb, UBYTE *dlta, ULONG dltasize, struct BitMap *deltabm, struct BitMap *bm )
{
    D(bug("[anim.datatype] %s()\n", __PRETTY_FUNCTION__));
    return 0;
}

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
