CONVERTFUNC(RGB15,RGB16) /* Untested */
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

    	    dst[x] = ((s & (RGB15_RMASK | RGB15_GMASK)) << 1) | (s & RGB15_BMASK);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,BGR16) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP1516CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,BGR15) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP1515CODE
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,ARGB32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, RGB15, ARGB32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,BGRA32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, RGB15, BGRA32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,RGBA32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, RGB15, RGBA32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(RGB15,ABGR32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, RGB15, ABGR32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,RGB24) /* Untested */
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

    	    s = UPSHIFT16(s, RGB15, RGB24);
	    
    	    PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,BGR24) /* Untested */
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

    	    s = UPSHIFT16(s, RGB15, BGR24);
	    
    	    PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,RGB16OE) /* Untested */
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

    	    s = ((s & (RGB15_RMASK | RGB15_GMASK)) << 1) | (s & RGB15_BMASK);
	    dst[x] = INV16(s);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,BGR16OE) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP1516OECODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,BGR15OE) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP1515OECODE
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,RGB15OE) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP16CODE
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,XRGB32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, RGB15, ARGB32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,BGRX32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, RGB15, BGRA32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGB15,RGBX32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, RGB15, RGBA32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(RGB15,XBGR32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, RGB15, ABGR32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}
