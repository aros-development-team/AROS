CONVERTFUNC(RGBA32,RGB16) /* Untested */
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

CONVERTFUNC(RGBA32,BGR16) /* Untested */
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

CONVERTFUNC(RGBA32,RGB15) /* Untested */
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

CONVERTFUNC(RGBA32,BGR15) /* Untested */
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

CONVERTFUNC(RGBA32,ABGR32) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP32CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBA32,ARGB32) /* Untested */
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

    	    dst[x] = SHUFFLE32(s, RGBA32, ARGB32);
	}
    	src = (ULONG *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(RGBA32,BGRA32) /* Untested */
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

    	    dst[x] = SHUFFLE32(s, RGBA32, BGRA32);
	}
    	src = (ULONG *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(RGBA32,RGB24) /* Untested */
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

CONVERTFUNC(RGBA32,BGR24) /* Untested */
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

CONVERTFUNC(RGBA32,RGB16OE) /* Untested */
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

CONVERTFUNC(RGBA32,BGR16OE) /* Untested */
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

CONVERTFUNC(RGBA32,RGB15OE) /* Untested */
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

CONVERTFUNC(RGBA32,BGR15OE) /* Untested */
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
