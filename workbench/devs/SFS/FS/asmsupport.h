#ifndef ASMSUPPORT_H_
#define ASMSUPPORT_H_

#ifdef __AROS__
#include <exec/rawfmt.h>
#endif
#ifdef __MORPHOS__
#include <exec/rawfmt.h>
#endif
#include <exec/types.h>

#if defined __i386__
static inline LONG fls(ULONG mask)
{
    LONG bit = -1;

    if (mask)
        asm volatile("bsr %1, %0":"=r"(bit):"rm"(mask));

    return (bit + 1);
}
#else
static inline LONG fls(ULONG mask)
{
    LONG bit;

    if (mask == 0)
    return (0);

    for (bit = 1; mask != 1; bit++)
        mask = mask >> 1;

    return (bit);
}
#endif

/* Finds first set bit in /data/ starting at /bitoffset/.  This function
   considers the MSB to be the first bit. */
static inline WORD bfffo(ULONG data, WORD bitoffset)
{
    ULONG mask = 0xffffffff >> bitoffset;
    data &= mask;
    return data == 0 ? 32 : 32-fls(data);
}

/* Finds first zero bit in /data/ starting at /bitoffset/.  This function
   considers the MSB to be the first bit. */
static inline WORD bfffz(ULONG data, WORD bitoffset)
{
    return bfffo(~data, bitoffset);
}

ULONG bfcnto(ULONG v);
ULONG bfcntz(ULONG v);
LONG bmflo(ULONG *bitmap, LONG bitoffset);
LONG bmflz(ULONG *bitmap, LONG bitoffset);
LONG bmffo(ULONG *bitmap, LONG longs, LONG bitoffset);
LONG bmffz(ULONG *bitmap, LONG longs, LONG bitoffset);
LONG bmclr(ULONG *bitmap, LONG longs, LONG bitoffset, LONG bits);
LONG bmset(ULONG *bitmap, LONG longs, LONG bitoffset, LONG bits);
BOOL bmtsto(ULONG *bitmap, LONG bitoffset, LONG bits);
BOOL bmtstz(ULONG *bitmap, LONG bitoffset, LONG bits);
ULONG bmcnto(ULONG *bitmap, LONG bitoffset, LONG bits);
ULONG bmcntz(ULONG *bitmap, LONG bitoffset, LONG bits);

ULONG CALCCHECKSUM(ULONG, ULONG *);

#ifdef RAWFMTFUNC_STRING
#define putChProc   RAWFMTFUNC_STRING
#else
#define putChProc   (void (*)())"\x16\xC0\x4E\x75"
#endif

#ifdef SFS_BE
#define BE2L AROS_BE2LONG
#define L2BE AROS_LONG2BE
#define BE2W AROS_BE2WORD
#define W2BE AROS_WORD2BE
#else
#define BE2L AROS_LE2LONG
#define L2BE AROS_LONG2LE
#define BE2W AROS_LE2WORD
#define W2BE AROS_WORD2LE
#endif


#ifdef __i386__
#define AROS_SWAP_BYTES_LONG_CPU(l)     \
    ({ ULONG v; __asm__ __volatile__("bswap %0":"=r"(v):"0"(l)); v;})
#define AROS_SWAP_BYTES_WORD_CPU(l)     \
    ({ UWORD w; __asm__ __volatile__("xchgb %b0,%h0":"=q"(w):"0"(l)); w;})
#endif


#endif /*ASMSUPPORT_H_*/
