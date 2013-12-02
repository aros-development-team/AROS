/*
    Copyright � 2003-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include "bitmap.h"
#include <exec/types.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>

LONG bfffo(ULONG val, UBYTE bitoffset)
{
    val &= (0xffffffff >> bitoffset);
    if (val)
        return __builtin_clz(val);
    else
        return 32;
}

LONG bfflo(ULONG val, UBYTE bitoffset)
{
    val &= (0xffffffff << (31-bitoffset));
    if (val)
        return 31-__builtin_ctz(val);
    else
        return -1;
}

LONG bfffz(ULONG val, UBYTE bitoffset)
{
    return bfffo(~val, bitoffset);
}

LONG bfflz(ULONG val, UBYTE bitoffset)
{
    return bfflo(~val, bitoffset);
}

ULONG bfset(ULONG data, UBYTE bitoffset, UBYTE bits)
{
    ULONG mask = ~((1 << (32 - bits)) - 1);
    mask >>= bitoffset;
    return data | mask;
}

ULONG bfclr(ULONG data, UBYTE bitoffset, UBYTE bits)
{
    ULONG mask = ~((1 << (32 - bits)) - 1);
    mask >>= bitoffset;
    return data & ~mask;
}

ULONG bfcnto(ULONG v)
{
    ULONG const w = v - ((v >> 1) & 0x55555555);                    // temp
    ULONG const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);     // temp
    ULONG const c = ((x + ((x >> 4) & 0xF0F0F0F)) * 0x1010101) >> 24; // count

    return c;
}

ULONG bfcntz(ULONG v)
{
    return bfcnto(~v);
}

LONG bmffo(ULONG *bitmap, ULONG longs, LONG bitoffset)
{
    ULONG *scan = bitmap;
    ULONG err = 32*longs;
    int longoffset, bit;

    longoffset = bitoffset >> 5;
    longs -= longoffset;
    scan += longoffset;

    bitoffset = bitoffset & 0x1F;

    if (bitoffset != 0) {
        if ((bit = bfffo(*scan, bitoffset)) < 32) {
            return (bit + ((scan - bitmap) << 5));
        }
        scan++;
        longs--;
    }

    while (longs-- > 0) {
        if (*scan++ != 0) {
            scan--;
            return (bfffo(*scan,0) + ((scan - bitmap) << 5));
        }
    }

    return (err);
}

LONG bmffz(ULONG *bitmap, ULONG longs, LONG bitoffset)
{
    ULONG *scan = bitmap;
    ULONG err = 32*longs;
    int longoffset, bit;

    longoffset = bitoffset >> 5;
    longs -= longoffset;
    scan += longoffset;

    bitoffset = bitoffset & 0x1F;

    if (bitoffset != 0) {
        if ((bit = bfffz(*scan, bitoffset)) < 32) {
            return (bit + ((scan - bitmap) << 5));
        }
        scan++;
        longs--;
    }

    while (longs-- > 0) {
        if (*scan++ != 0xFFFFFFFF) {
            scan--;
            return (bfffz(*scan,0) + ((scan - bitmap) << 5));
        }
    }

    return (err);
}

LONG bmclr(ULONG *bitmap, ULONG longs, LONG bitoffset, LONG bits)
{
    ULONG *scan = bitmap;
    int longoffset;
    int orgbits = bits;

    longoffset = bitoffset >> 5;
    longs -= longoffset;
    scan += longoffset;

    bitoffset = bitoffset & 0x1F;

    if (bitoffset != 0) {
        if (bits < 32) {
            *scan = bfclr(*scan, bitoffset, bits);
        } else {
            *scan = bfclr(*scan, bitoffset, 32);
        }
        scan++;
        longs--;
        bits -= 32 - bitoffset;
    }

    while (bits > 0 && longs-- > 0) {
        if (bits > 31) {
            *scan++ = 0;
        } else {
            *scan = bfclr(*scan, 0, bits);
        }
        bits -= 32;
    }

    if (bits <= 0) {
        return (orgbits);
    }
    return (orgbits - bits);
}

LONG bmset(ULONG *bitmap, ULONG longs, LONG bitoffset, LONG bits)
{
    ULONG *scan = bitmap;
    int longoffset;
    int orgbits = bits;

    longoffset = bitoffset >> 5;
    longs -= longoffset;
    scan += longoffset;

    bitoffset = bitoffset & 0x1F;

    if (bitoffset != 0) {
        if (bits < 32) {
            *scan = (bfset((*scan), bitoffset, bits));
        } else {
            *scan = (bfset((*scan), bitoffset, 32));
        }
        scan++;
        longs--;
        bits -= 32 - bitoffset;
    }

    while (bits > 0 && longs-- > 0) {
        if (bits > 31) {
            *scan++ = 0xFFFFFFFF;
        } else {
            *scan = (bfset((*scan), 0, bits));
        }
        bits -= 32;
    }

    if (bits <= 0) {
        return (orgbits);
    }
    return (orgbits - bits);
}

int bmtstz(ULONG *bitmap, ULONG bitoffset, LONG bits)
{
    LONG longoffset = bitoffset >> 5;
    ULONG *scan = bitmap;
    ULONG mask;

    scan += longoffset;
    bitoffset &= 0x1f;

    if (bitoffset != 0)
    {
        if ((bits + bitoffset) < 32)
        {
            mask = (0xffffffff >> bitoffset) & (0xffffffff << (32 - (bits+bitoffset)));
            bits=0;
        }
        else
        {
            mask = (0xffffffff >> bitoffset);
            bits -= (32-bitoffset);
        }
        if ((mask & (*scan++)) != 0)
            return 0;
    }

    while (bits > 0)
    {
        if (bits >= 32)
        {
            mask=0xffffffff;
            bits -= 32;
        }
        else
        {
            mask = 0xffffffff << (32-bits);
            bits = 0;
        }
        if ((mask & (*scan++)) != 0)
            return 0;
    }

    return 1;
}

ULONG bmcnto(ULONG *bitmap, LONG bitoffset, LONG bits)
{
    LONG longoffset = bitoffset >> 5;
    ULONG *scan = bitmap;
    ULONG count = 0;
    ULONG mask;

    scan += longoffset;
    bitoffset &= 0x1f;

    if (bitoffset != 0)
    {
        if ((bits + bitoffset) < 32)
        {
            mask = (0xffffffff >> bitoffset) & (0xffffffff << (32 - (bits+bitoffset)));
            bits=0;
        }
        else
        {
            mask = (0xffffffff >> bitoffset);
            bits -= (32-bitoffset);
        }
        count += bfcnto(*scan++ & mask);
    }

    while (bits > 0)
    {
        if (bits >= 32)
        {
            mask=0xffffffff;
            bits -= 32;
        }
        else
        {
            mask = 0xffffffff << (32-bits);
            bits = 0;
        }
        count += bfcnto(*scan++ & mask);
    }

    return count;
}

ULONG bmcntz(ULONG *bitmap, LONG bitoffset, LONG bits)
{
    LONG longoffset = bitoffset >> 5;
    ULONG *scan = bitmap;
    ULONG count = 0;
    ULONG mask;

    scan += longoffset;
    bitoffset &= 0x1f;

    if (bitoffset != 0)
    {
        if ((bits + bitoffset) < 32)
        {
            mask = ~((0xffffffff >> bitoffset) & (0xffffffff << (32 - (bits+bitoffset))));
            bits=0;
        }
        else
        {
            mask = ~(0xffffffff >> bitoffset);
            bits -= (32-bitoffset);
        }

        count += bfcntz(*scan++ | mask);
    }

    while (bits > 0)
    {
        if (bits >= 32)
        {
            mask=0;
            bits -= 32;
        }
        else
        {
            mask = ~(0xffffffff << (32-bits));
            bits = 0;
        }

        count += bfcntz(*scan++ | mask);
    }

    return count;
}

void *mh_Alloc(struct MemHeaderExt *mhe, IPTR size, ULONG *flags)
{
	return NULL;
}

void mh_Free(struct MemHeaderExt *mhe, APTR  mem,  IPTR size)
{
}

void *mh_AllocAbs(struct MemHeaderExt *mhe, IPTR size, APTR  mem)
{
	return NULL;
}

void *mh_ReAlloc(struct MemHeaderExt *mhe, APTR  old,  IPTR size)
{
	return NULL;
}

IPTR mh_Avail(struct MemHeaderExt *mhe, ULONG flags)
{
	struct ati_staticdata *sd = (APTR)mhe->mhe_UserData;
	IPTR size = 0;

//	Forbid();

	ObtainSemaphore(&sd->CardMemLock);

	if (flags & MEMF_TOTAL)
		size = sd->Card.FbUsableSize;
	else if (flags & MEMF_LARGEST)
	{
		ULONG ptr;

		ptr = bmffz(sd->CardMemBmp, sd->CardMemSize, 0);

	    while (ptr < (sd->CardMemSize << 5))
	    {
	        ULONG tmpptr = bmffo(sd->CardMemBmp, sd->CardMemSize, ptr);

	        if ((tmpptr - ptr) > size)
	        	size = tmpptr - ptr;

	        ptr = bmffz(sd->CardMemBmp, sd->CardMemSize, tmpptr);
	    }

	    size <<= 10;
	}
	else
		size = bmcntz(sd->CardMemBmp, 0, sd->Card.FbUsableSize >> 10) << 10;

//    Permit();

	ReleaseSemaphore(&sd->CardMemLock);

	return size;
}

void BitmapInit(struct ati_staticdata *sd)
{
    /*
     * If Radeon chip has some video memory, create a bitmap representing all allocations.
     * Divide whole memory into 1KB chunks
     */
    if (sd->Card.FbUsableSize)
    {
        sd->CardMemBmp = AllocPooled(sd->memPool, sd->Card.FbUsableSize >> 13);

    	sd->managedMem.mhe_MemHeader.mh_Node.ln_Type = NT_MEMORY;
    	sd->managedMem.mhe_MemHeader.mh_Node.ln_Name = "Radeon VRAM";
    	sd->managedMem.mhe_MemHeader.mh_Node.ln_Pri = -128;
    	sd->managedMem.mhe_MemHeader.mh_Attributes = MEMF_CHIP | MEMF_MANAGED | MEMF_PUBLIC;

    	sd->managedMem.mhe_UserData = sd;

    	sd->managedMem.mhe_Alloc = mh_Alloc;
    	sd->managedMem.mhe_Free = mh_Free;
    	sd->managedMem.mhe_AllocAbs = mh_AllocAbs;
    	sd->managedMem.mhe_ReAlloc = mh_ReAlloc;
    	sd->managedMem.mhe_Avail = mh_Avail;

    	Disable();
    	AddTail(&SysBase->MemList, (struct Node *)&sd->managedMem);
    	Enable();
    }

    /* Number of ULONG's in bitmap */
    sd->CardMemSize = sd->Card.FbUsableSize >> 15;

    bug("[ATIBMP] Bitmap at %p, size %d bytes (%d bits)\n", sd->CardMemBmp, sd->CardMemSize << 2, sd->Card.FbUsableSize >> 10);
}

void BitmapFree(struct ati_staticdata *sd, ULONG ptr, ULONG size)
{
    if (ptr + size < sd->Card.FbUsableSize)
    {
        bmclr(sd->CardMemBmp, sd->CardMemSize, ptr >> 10, (size + 1023) >> 10);
    }
}

ULONG BitmapAlloc(struct ati_staticdata *sd, ULONG size)
{
    ULONG ptr;
    size = (size + 1023) >> 10;

    ptr = bmffz(sd->CardMemBmp, sd->CardMemSize, 0);

    D(bug("[ATIBMP] BitmapAlloc(%d)\n", size));

    while (ptr <= (sd->CardMemSize << 5) + size)
    {
        D(bug("[ATIBMP] ptr=%08x\n", ptr));

        if (bmtstz(sd->CardMemBmp, ptr, size))
        {
            bmset(sd->CardMemBmp, sd->CardMemSize, ptr, size);
            break;
        }

        ptr = bmffo(sd->CardMemBmp, sd->CardMemSize, ptr);
        ptr = bmffz(sd->CardMemBmp, sd->CardMemSize, ptr);
    }

    if (ptr > (sd->CardMemSize << 5) - size)
        ptr = 0xffffffff;
    else
        ptr <<= 10;

    return ptr;
}
