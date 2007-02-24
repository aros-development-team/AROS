CONVERTFUNC(BGR15,RGB16) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP1516CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,BGR16) /* Untested */
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

    	    dst[x] = ((s & (BGR15_BMASK | BGR15_GMASK)) << 1) | (s & BGR15_RMASK);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT        
}

CONVERTFUNC(BGR15,RGB15) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP1515CODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,ARGB32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, BGR15, ARGB32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,BGRA32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, BGR15, BGRA32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
   
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,RGBA32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, BGR15, RGBA32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}


CONVERTFUNC(BGR15,ABGR32) /* Untested */
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

    	    dst[x] = UPSHIFT16(s, BGR15, ABGR32);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (ULONG *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,RGB24) /* Untested */
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

    	    s = UPSHIFT16(s, BGR15, RGB24);
	    
    	    PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,BGR24) /* Untested */
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

    	    s = UPSHIFT16(s, BGR15, BGR24);
	    
    	    PUT24(dst, COMP8(s, 1), COMP8(s, 2), COMP8(s, 3))
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (UBYTE *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,RGB16OE) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP1516OECODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,BGR16OE) /* Untested */
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

    	    s = ((s & (BGR15_BMASK | BGR15_GMASK)) << 1) | (s & BGR15_RMASK);
	    dst[x] = INV16(s);
	}
    	src = (UWORD *)(((UBYTE *)src) + srcMod);
	dst = (UWORD *)(((UBYTE *)dst) + dstMod);
    }
    
    return 1;
    
    CONVERTFUNC_EXIT        
}

CONVERTFUNC(BGR15,RGB15OE) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP1515OECODE
    
    CONVERTFUNC_EXIT
}

CONVERTFUNC(BGR15,BGR15OE) /* Untested */
{
    CONVERTFUNC_INIT

    SWAP16CODE
    
    CONVERTFUNC_EXIT
}

