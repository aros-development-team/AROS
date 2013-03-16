CONVERTFUNC(BGR15OE,BGR15)
{
    CONVERTFUNC_INIT

    SWAP16CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,RGB16)
{
    CONVERTFUNC_INIT

    SWAP15OE16CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,BGR16)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            UWORD s = INV16(src[x]);

            dst[x] = ((s & (BGR15_BMASK | BGR15_GMASK)) << 1) | (s & BGR15_RMASK);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT        
}

CONVERTFUNC(BGR15OE,RGB15)
{
    CONVERTFUNC_INIT

    SWAP15OE15CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,ARGB32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            dst[x] = UPSHIFT16(s, BGR15, ARGB32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,BGRA32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            dst[x] = UPSHIFT16(s, BGR15, BGRA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,RGBA32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            dst[x] = UPSHIFT16(s, BGR15, RGBA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(BGR15OE,ABGR32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            dst[x] = UPSHIFT16(s, BGR15, ABGR32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,RGB24)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    UBYTE *dst = (UBYTE *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            s = UPSHIFT16(s, BGR15, RGB24);

            PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,BGR24)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    UBYTE *dst = (UBYTE *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            s = UPSHIFT16(s, BGR15, BGR24);

            PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,RGB16OE)
{
    CONVERTFUNC_INIT

    SWAP15OE16OECODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,BGR16OE)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    UWORD *dst = (UWORD *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            UWORD s = INV16(src[x]);

            s = ((s & (BGR15_BMASK | BGR15_GMASK)) << 1) | (s & BGR15_RMASK);
            dst[x] = INV16(s);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT        
}

CONVERTFUNC(BGR15OE,RGB15OE)
{
    CONVERTFUNC_INIT

    SWAP15OE15OECODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,XRGB32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            dst[x] = UPSHIFT16(s, BGR15, ARGB32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,BGRX32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            dst[x] = UPSHIFT16(s, BGR15, BGRA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15OE,RGBX32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            dst[x] = UPSHIFT16(s, BGR15, RGBA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(BGR15OE,XBGR32)
{
    CONVERTFUNC_INIT

    UWORD *src = (UWORD *)srcPixels;
    ULONG *dst = (ULONG *)dstPixels;
    ULONG x, y;

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            ULONG s = INV16(src[x]);

            dst[x] = UPSHIFT16(s, BGR15, ABGR32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}
