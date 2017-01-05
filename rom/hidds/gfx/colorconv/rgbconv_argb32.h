CONVERTFUNC(ARGB32,RGB16)
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

            dst[x] = DOWNSHIFT16(s, ARGB32, RGB16);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,BGR16)
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

            dst[x] = DOWNSHIFT16(s, ARGB32, BGR16);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,RGB15)
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

            dst[x] = DOWNSHIFT16(s, ARGB32, RGB15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,BGR15)
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

            dst[x] = DOWNSHIFT16(s, ARGB32, BGR15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,BGRA32)
{
    CONVERTFUNC_INIT

    SWAP32CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,RGBA32)
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

            dst[x] = SHUFFLE32(s, ARGB32, RGBA32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(ARGB32,ABGR32)
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

            dst[x] = SHUFFLE32(s, ARGB32, ABGR32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,RGB24)
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

            PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,BGR24)
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

            PUT24(dst, COMP8(s, 3), COMP8(s, 2), COMP8(s, 1))
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,RGB16OE)
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

            s = DOWNSHIFT16(s, ARGB32, RGB16);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,BGR16OE)
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

            s = DOWNSHIFT16(s, ARGB32, BGR16);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,RGB15OE)
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

            s = DOWNSHIFT16(s, ARGB32, RGB15);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,BGR15OE)
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

            s = DOWNSHIFT16(s, ARGB32, BGR15);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,XRGB32)
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

            dst[x] = s & 0x00FFFFFF;
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,BGRX32)
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

            dst[x] = SHUFFLE24(s, ARGB32, BGRA32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(ARGB32,RGBX32)
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

            dst[x] = SHUFFLE24(s, ARGB32, RGBA32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(ARGB32,XBGR32)
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

            dst[x] = SHUFFLE24(s, ARGB32, ABGR32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}
