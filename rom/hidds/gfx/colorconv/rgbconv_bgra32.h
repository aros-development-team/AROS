CONVERTFUNC(BGRA32,RGB16)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];
            dst[x] = DOWNSHIFT16(s, BGRA32, RGB16);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,BGR16)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = DOWNSHIFT16(s, BGRA32, BGR16);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,RGB15)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = DOWNSHIFT16(s, BGRA32, RGB15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,BGR15)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = DOWNSHIFT16(s, BGRA32, BGR15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,ARGB32)
{
    CONVERTFUNC_INIT

    SWAP32CODE

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,RGBA32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = SHUFFLE32(s, BGRA32, RGBA32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}


CONVERTFUNC(BGRA32,ABGR32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = SHUFFLE32(s, BGRA32, ABGR32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,RGB24)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UBYTE *dst = (UBYTE *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            PUT24(dst, COMP8(s, 2), COMP8(s, 1), COMP8(s, 0))
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,BGR24)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UBYTE *dst = (UBYTE *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            PUT24(dst, COMP8(s, 0), COMP8(s, 1), COMP8(s, 2))
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,RGB16OE)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            s = DOWNSHIFT16(s, BGRA32, RGB16);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,BGR16OE)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            s = DOWNSHIFT16(s, BGRA32, BGR16);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,RGB15OE)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            s = DOWNSHIFT16(s, BGRA32, RGB15);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,BGR15OE)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            s = DOWNSHIFT16(s, BGRA32, BGR15);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,BGRX32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = s & 0xFFFFFF00;
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,XRGB32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = SHUFFLE24(s, BGRA32, ARGB32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGRA32,RGBX32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = SHUFFLE24(s, BGRA32, RGBA32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}


CONVERTFUNC(BGRA32,XBGR32)
{
    CONVERTFUNC_INIT

    ULONG *src = (ULONG *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = SHUFFLE24(s, BGRA32, ABGR32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }

    return 1;

    CONVERTFUNC_EXIT
}
