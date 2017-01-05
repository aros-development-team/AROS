CONVERTFUNC(BGR24,RGB16)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = DOWNSHIFT16(s, BGR24, RGB16);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,BGR16)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = DOWNSHIFT16(s, BGR24, BGR16);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,RGB15)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = DOWNSHIFT16(s, BGR24, RGB15);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,BGR15)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = DOWNSHIFT16(s, BGR24, BGR15);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,ABGR32)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = s;
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,RGBA32)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = SHUFFLE24(s, BGR24, RGBA32);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,BGRA32)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = s << 8;
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(BGR24,ARGB32)
{
    CONVERTFUNC_INIT

    SWAP2432CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,RGB24)
{
    CONVERTFUNC_INIT

    SWAP2424CODE
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,RGB16OE)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            s = DOWNSHIFT16(s, BGR24, RGB16);
            dst[x] = INV16(s);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,BGR16OE)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            s = DOWNSHIFT16(s, BGR24, BGR16);
            dst[x] = INV16(s);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,RGB15OE)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            s = DOWNSHIFT16(s, BGR24, RGB15);
            dst[x] = INV16(s);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,BGR15OE)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            s = DOWNSHIFT16(s, BGR24, BGR15);
            dst[x] = INV16(s);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,XBGR32)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = s;
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,RGBX32)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = SHUFFLE24(s, BGR24, RGBA32);
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,BGRX32)
{
    CONVERTFUNC_INIT

    UBYTE *src = (UBYTE *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = GET24;

            dst[x] = s << 8;
        }
        src = (UBYTE *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR24,XRGB32)
{
    CONVERTFUNC_INIT

    SWAP2432CODE

    CONVERTFUNC_EXIT
}
