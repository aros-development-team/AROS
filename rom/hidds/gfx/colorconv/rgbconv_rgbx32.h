CONVERTFUNC(RGBX32,RGB16)
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

            dst[x] = DOWNSHIFT16(s, RGBA32, RGB16);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,BGR16)
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

            dst[x] = DOWNSHIFT16(s, RGBA32, BGR16);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,RGB15)
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

            dst[x] = DOWNSHIFT16(s, RGBA32, RGB15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,BGR15)
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

            dst[x] = DOWNSHIFT16(s, RGBA32, BGR15);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,RGBA32)
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


CONVERTFUNC(RGBX32,ABGR32)
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

            dst[x] = SHUFFLE24(s, RGBA32, ABGR32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,ARGB32)
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

            dst[x] = SHUFFLE24(s, RGBA32, ARGB32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(RGBX32,BGRA32)
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

            dst[x] = SHUFFLE24(s, RGBA32, BGRA32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,RGB24)
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

CONVERTFUNC(RGBX32,BGR24)
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

CONVERTFUNC(RGBX32,RGB16OE)
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

            s = DOWNSHIFT16(s, RGBA32, RGB16);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,BGR16OE)
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

            s = DOWNSHIFT16(s, RGBA32, BGR16);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,RGB15OE)
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

            s = DOWNSHIFT16(s, RGBA32, RGB15);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,BGR15OE)
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

            s = DOWNSHIFT16(s, RGBA32, BGR15);
            dst[x] = INV16(s);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,BGRX32)
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

            dst[x] = SHUFFLE24(s, RGBA32, BGRA32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBX32,XBGR32)
{
    CONVERTFUNC_INIT

    SWAP32CODE
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(RGBX32,XRGB32)
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

            dst[x] = SHUFFLE24(s, RGBA32, ARGB32);
        }
        src = (ULONG *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}
