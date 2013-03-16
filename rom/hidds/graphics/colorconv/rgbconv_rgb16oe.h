CONVERTFUNC(RGB16OE,RGB15)
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

            dst[x] = ((s >> 1) & (RGB15_RMASK | RGB15_GMASK)) | (s & RGB15_BMASK);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,BGR16)
{
    CONVERTFUNC_INIT

    SWAP16OE16CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,BGR15)
{
    CONVERTFUNC_INIT

    SWAP16OE15CODE
        
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,RGB16)
{
    CONVERTFUNC_INIT

    SWAP16CODE
        
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,ARGB32)
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

            dst[x] = UPSHIFT16(s, RGB16, ARGB32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,BGRA32)
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

            dst[x] = UPSHIFT16(s, RGB16, BGRA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,RGBA32)
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

            dst[x] = UPSHIFT16(s, RGB16, RGBA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(RGB16OE,ABGR32)
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

            dst[x] = UPSHIFT16(s, RGB16, ABGR32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,RGB24)
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

            s = UPSHIFT16(s, RGB16, RGB24);

            PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,BGR24)
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

            s = UPSHIFT16(s, RGB16, BGR24);

            PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(RGB16OE,RGB15OE)
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

            s = ((s >> 1) & (RGB15_RMASK | RGB15_GMASK)) | (s & RGB15_BMASK);
            dst[x] = INV16(s);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,BGR16OE)
{
    CONVERTFUNC_INIT

    SWAP16OE16OECODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,BGR15OE)
{
    CONVERTFUNC_INIT

    SWAP16OE15OECODE
        
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,XRGB32)
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

            dst[x] = UPSHIFT16(s, RGB16, ARGB32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,BGRX32)
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

            dst[x] = UPSHIFT16(s, RGB16, BGRA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB16OE,RGBX32)
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

            dst[x] = UPSHIFT16(s, RGB16, RGBA32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(RGB16OE,XBGR32)
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

            dst[x] = UPSHIFT16(s, RGB16, ABGR32);
        }
        src = (UWORD *)(((UBYTE *)src) + srcMod);
        dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}
