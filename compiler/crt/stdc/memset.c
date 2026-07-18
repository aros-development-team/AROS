/*
    Copyright (C) 1995-2012, The AROS Development Team. All rights reserved.

    C99 function memset().
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <string.h>

        void * memset (

/*  SYNOPSIS */
        void * dest,
        int    c,
        size_t count)

/*  FUNCTION
        Fill the memory at dest with count times c.

    INPUTS
        dest - The first byte of the destination area in memory
        c - The byte to fill memory with
        count - How many bytes to write

    RESULT
        dest.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        memmove(), memcpy()

    INTERNALS

******************************************************************************/
{
    UBYTE * ptr = dest;

    while (((IPTR)ptr)&(AROS_LONGALIGN-1) && count)
    {
        *ptr ++ = c;
        count --;
    }

    if (count > sizeof(ULONG))
    {
        ULONG * ulptr = (ULONG *)ptr;
        ULONG fill;

        fill = (ULONG)(c & 0xFF);
        fill = (fill <<  8) | fill;
        fill = (fill << 16) | fill;

#if defined(__mc68000__)
        /*
         * On 68000, GCC -Os compiles the plain long-fill loop to ~6 instructions
         * per 4 bytes (~18 cycles/byte), which dominates ROM boot time clearing
         * MEMF_CLEAR allocations. Fill 32 bytes per iteration with movem.l (the
         * fastest 68000 memory fill) instead.
         */
        {
            /*
             * Fill 16 bytes per iteration with movem.l (d3-d6). We pin only four
             * data registers so the allocator still has registers for the loop
             * counter and pointer; that is enough to beat the plain long loop by
             * a wide margin while staying well within the 68000's register file.
             */
            register ULONG f3 asm("d3") = fill;
            register ULONG f4 asm("d4") = fill;
            register ULONG f5 asm("d5") = fill;
            register ULONG f6 asm("d6") = fill;

            while (count >= 16)
            {
                asm volatile ("movem.l %%d3-%%d6,%0@\n\tlea %0@(16),%0"
                              : "+a" (ulptr)
                              : "d"(f3),"d"(f4),"d"(f5),"d"(f6)
                              : "memory");
                count -= 16;
            }
        }
#endif

        while (count > sizeof(ULONG))
        {
            *ulptr ++ = fill;
            count -= sizeof(ULONG);
        }

        ptr = (UBYTE *)ulptr;
    }

    while (count --)
        *ptr ++ = c;

    return dest;
} /* memset */

