/*
 * These functions implement bitfield and bitmap operations for any possible CPU
 * Used instead of m68k asm functions in asmsupport.s file.
 */

#include <exec/types.h>

#ifdef __AROS__
#include <aros/macros.h>
#include <aros/asmcall.h>
#else
#define AROS_BE2LONG(x) x
#define AROS_LONG2BE(x) x
#endif
#include <stdio.h>

#include "asmsupport.h"
#include "debug.h"

#if defined __i386__
static inline LONG frs(ULONG mask)
{
    LONG bit = 0;

    if (mask)
        asm volatile("bsf %1, %0":"=r"(bit):"rm"(mask));

    return (32-bit);
}

#else
static inline LONG frs(ULONG mask)
{
    LONG bit;

    if (mask == 0)
        return (31);

    for (bit = 32; (mask & 1) != 1; bit--)
        mask = mask >> 1;

    return (bit);
}
#endif

/* Finds last set bit in /data/ starting at /bitoffset/.  This function
   considers the MSB to be the first bit. */
static inline WORD bfflo(ULONG data, WORD bitoffset)
{
    ULONG mask = 0xffffffff << (31-bitoffset);
    data &= mask;
    return data == 0 ? -1 : frs(data)-1;
}

/* Finds last zero bit in /data/ starting at /bitoffset/.  This function
   considers the MSB to be the first bit. */
static inline WORD bfflz(ULONG data, WORD bitoffset)
{
    return bfflo(~data, bitoffset);
}

/* Sets /bits/ bits starting from /bitoffset/ in /data/.
   /bits/ must be between 1 and 32. */
static inline ULONG bfset(ULONG data, WORD bitoffset, WORD bits)
{
    ULONG mask = ~((1 << (32 - bits)) - 1);
    mask >>= bitoffset;
    return data | mask;
}

/* Clears /bits/ bits starting from /bitoffset/ in /data/.
   /bits/ must be between 1 and 32. */
static inline ULONG bfclr(ULONG data, WORD bitoffset, WORD bits)
{
    ULONG mask = ~((1 << (32 - bits)) - 1);
    mask >>= bitoffset;
    return data & ~mask;
}

ULONG bfcnto(ULONG v)
{
    ULONG const w = v - ((v >> 1) & 0x55555555);                      // temp
    ULONG const x = (w & 0x33333333) + ((w >> 2) & 0x33333333);       // temp
    ULONG const c = (((x + (x >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count

    return c;
}

ULONG bfcntz(ULONG v)
{
    return bfcnto(~v);
}

LONG bmflo(ULONG *bitmap, LONG bitoffset)
{
    ULONG *scan = bitmap;
    LONG longs;
    int longoffset, bit;

    longoffset = (bitoffset) >> 5;
    longs = longoffset;
    scan += longoffset;

    bitoffset = bitoffset & 0x1F;

    if (bitoffset != 0) {
        if ((bit = bfflo(AROS_BE2LONG(*scan), bitoffset)) >= 0) {
            return (bit + ((scan - bitmap) << 5));
        }
        scan--;
        longs--;
    }

    while (longs-- >= 0) {
        if (*scan-- != 0) {
            scan++;
            return (bfflo(AROS_BE2LONG(*scan),31) + ((scan - bitmap) << 5));
        }
    }

    return (-1);
}

LONG bmflz(ULONG *bitmap, LONG bitoffset)
{
    ULONG *scan = bitmap;
    LONG longs;
    int longoffset, bit;

    longoffset = (bitoffset) >> 5;
    longs = longoffset;
    scan += longoffset;

    bitoffset = bitoffset & 0x1F;

    if (bitoffset != 0) {
        if ((bit = bfflz(AROS_BE2LONG(*scan), bitoffset)) >= 0) {
            return (bit + ((scan - bitmap) << 5));
        }
        scan--;
        longs--;
    }

    while (longs-- >= 0) {
        if (*scan-- != 0xFFFFFFFF) {
            scan++;
            return (bfflz(AROS_BE2LONG(*scan),31) + ((scan - bitmap) << 5));
        }
    }

    return (-1);
}

/* This function finds the first set bit in a region of memory starting
   with /bitoffset/.  The region of memory is /longs/ longs long.  It
   returns the bitoffset of the first set bit it finds. */

LONG bmffo(ULONG *bitmap, LONG longs, LONG bitoffset)
{
    ULONG *scan = bitmap;
    ULONG err = 32*longs;
    int longoffset, bit;

    longoffset = bitoffset >> 5;
    longs -= longoffset;
    scan += longoffset;

    bitoffset = bitoffset & 0x1F;

    if (bitoffset != 0) {
        if ((bit = bfffo(AROS_BE2LONG(*scan), bitoffset)) < 32) {
            return (bit + ((scan - bitmap) << 5));
        }
        scan++;
        longs--;
    }

    while (longs-- > 0) {
        if (*scan++ != 0) {
            --scan;
            return (bfffo(AROS_BE2LONG(*scan),0) + ((scan - bitmap) << 5));
        }
    }

    return (err);
}

/* This function finds the first unset bit in a region of memory starting
   with /bitoffset/.  The region of memory is /longs/ longs long.  It
   returns the bitoffset of the first unset bit it finds. */

LONG bmffz(ULONG *bitmap, LONG longs, LONG bitoffset)
{
    ULONG *scan = bitmap;
    ULONG err = 32*longs;
    int longoffset, bit;

    longoffset = bitoffset >> 5;
    longs -= longoffset;
    scan += longoffset;

    bitoffset = bitoffset & 0x1F;

    if (bitoffset != 0) {
        if ((bit = bfffz(AROS_BE2LONG(*scan), bitoffset)) < 32) {
            return (bit + ((scan - bitmap) << 5));
        }
        scan++;
        longs--;
    }

    while (longs-- > 0) {
        if (*scan++ != 0xFFFFFFFF) {
            --scan;
            return (bfffz(AROS_BE2LONG(*scan),0) + ((scan - bitmap) << 5));
        }
    }

    return (err);
}

/* This function clears /bits/ bits in a region of memory starting
   with /bitoffset/.  The region of memory is /longs/ longs long.  If
   the region of memory is too small to clear /bits/ bits then this
   function exits after having cleared all bits till the end of the
   memory region.  In any case it returns the number of bits which
   were actually cleared. */

LONG bmclr(ULONG *bitmap, LONG longs, LONG bitoffset, LONG bits)
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
            *scan = AROS_LONG2BE(bfclr(AROS_BE2LONG(*scan), bitoffset, bits));
        } else {
            *scan = AROS_LONG2BE(bfclr(AROS_BE2LONG(*scan), bitoffset, 32));
        }
        scan++;
        longs--;
        bits -= 32 - bitoffset;
    }

    while (bits > 0 && longs-- > 0) {
        if (bits > 31) {
            *scan++ = 0;
        } else {
            *scan = AROS_LONG2BE(bfclr(AROS_BE2LONG(*scan), 0, bits));
        }
        bits -= 32;
    }

    if (bits <= 0) {
        return (orgbits);
    }
    return (orgbits - bits);
}

/* This function sets /bits/ bits in a region of memory starting
   with /bitoffset/.  The region of memory is /longs/ longs long.  If
   the region of memory is too small to set /bits/ bits then this
   function exits after having set all bits till the end of the
   memory region.  In any case it returns the number of bits which
   were actually set. */

LONG bmset(ULONG *bitmap, LONG longs, LONG bitoffset, LONG bits)
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
            *scan = AROS_LONG2BE(bfset(AROS_BE2LONG(*scan), bitoffset, bits));
        } else {
            *scan = AROS_LONG2BE(bfset(AROS_BE2LONG(*scan), bitoffset, 32));
        }
        scan++;
        longs--;
        bits -= 32 - bitoffset;
    }

    while (bits > 0 && longs-- > 0) {
        if (bits > 31) {
            *scan++ = 0xFFFFFFFF;
        } else {
            *scan = AROS_LONG2BE(bfset(AROS_BE2LONG(*scan), 0, bits));
        }
        bits -= 32;
    }

    if (bits <= 0) {
        return (orgbits);
    }
    return (orgbits - bits);
}

BOOL bmtsto(ULONG *bitmap, LONG bitoffset, LONG bits)
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

        if ((mask & AROS_BE2LONG(*scan)) != mask)
            return FALSE;
        scan++;
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
        if ((mask & AROS_BE2LONG(*scan)) != mask)
            return FALSE;
        scan++;
    }

    return TRUE;
}

BOOL bmtstz(ULONG *bitmap, LONG bitoffset, LONG bits)
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
        if ((mask & AROS_BE2LONG(*scan)) != 0)
            return FALSE;
        scan++;
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
        if ((mask & AROS_BE2LONG(*scan)) != 0)
            return FALSE;
        scan++;
    }

    return TRUE;
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
        count += bfcnto(AROS_BE2LONG(*scan) & mask);
        scan++;
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
        count += bfcnto(AROS_BE2LONG(*scan) & mask);
        scan++;
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

        count += bfcntz(AROS_BE2LONG(*scan) | mask);
        scan++;
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

        count += bfcntz(AROS_BE2LONG(*scan) | mask);
        scan++;
    }

    return count;
}

ULONG CALCCHECKSUM(ULONG blocksize, ULONG *block)
{
    ULONG sum=1;
    blocksize >>=4;

    while(blocksize--)
    {
#if 0
        sum += (*block++);
        sum += (*block++);
        sum += (*block++);
        sum += (*block++);
#else
        sum += (ULONG)BE2L(*block++);
        sum += (ULONG)BE2L(*block++);
        sum += (ULONG)BE2L(*block++);
        sum += (ULONG)BE2L(*block++);
#endif
    }

    return sum;
}
