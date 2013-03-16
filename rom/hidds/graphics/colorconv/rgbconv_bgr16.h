CONVERTFUNC(BGR16,RGB16)
{
    CONVERTFUNC_INIT

    SWAP1616CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,BGR15)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            UWORD s = src[x];

            dst[x] = ((s >> 1) & (BGR15_BMASK | BGR15_GMASK)) | (s & BGR15_RMASK);

        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;
        
    CONVERTFUNC_EXIT    
}

CONVERTFUNC(BGR16,RGB15)
{
    CONVERTFUNC_INIT

    SWAP1615CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,ARGB32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR16, ARGB32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,BGRA32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR16, BGRA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,RGBA32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR16, RGBA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(BGR16,ABGR32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR16, ABGR32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,RGB24)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    UBYTE *dst = (UBYTE *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            s = UPSHIFT16(s, BGR16, RGB24);

            PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,BGR24)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    UBYTE *dst = (UBYTE *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            s = UPSHIFT16(s, BGR16, BGR24);

            PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,RGB16OE)
{
    CONVERTFUNC_INIT

    SWAP1616OECODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,BGR15OE)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            UWORD s = src[x];

            s = ((s >> 1) & (BGR15_BMASK | BGR15_GMASK)) | (s & BGR15_RMASK);
            dst[x] = INV16(s);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }

    return 1;
        
    CONVERTFUNC_EXIT    
}

CONVERTFUNC(BGR16,RGB15OE)
{
    CONVERTFUNC_INIT

    SWAP1615OECODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,BGR16OE)
{
    CONVERTFUNC_INIT

    SWAP16CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,XRGB32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR16, ARGB32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,BGRX32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR16, BGRA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR16,RGBX32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR16, RGBA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(BGR16,XBGR32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = src[x];

            dst[x] = UPSHIFT16(s, BGR16, ABGR32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}
