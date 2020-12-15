#ifdef __SSE__
/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <exec/types.h>

#include <emmintrin.h>

#include "setmem_pc.h"

/****i************************************************************************

    NAME */
#include <proto/utility.h>

        AROS_LH3(APTR, SetMem_SSE,

/*  SYNOPSIS */
        AROS_LHA(APTR, destination, A0),
        AROS_LHA(UBYTE, c, D0),
        AROS_LHA(LONG, length, D1),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 66, Utility)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    register ULONG i;
    register char *p;
    ULONG postsize;
    char val;

    D(bug("[utility:pc] %s(0x%p, %02x, %d)\n", __func__, destination, c, length);)

    p = (char *) destination;
    val = (char) c;

    if (length > 15)
    {
        ULONG presize, ssefillcount;

        presize = (((IPTR) destination + 15 ) & ~15) - (IPTR)destination;
        ssefillcount = (length - presize) / 16;
        postsize = length - (ssefillcount * 16) - presize;

        /* setup sse value .. */
        __m128i c16 = _mm_set1_epi8(val);

        D(bug("[utility:pc] %s: 0x%p, %d\n", __func__, p, presize);)

        /* fill inital bytes ... */
        __smallsetmem(p, val, presize);
        p += presize;

        D(bug("[utility:pc] %s: 0x%p, %d x 16\n", __func__, p, ssefillcount);)

        /* sse fill 16bytes at a time ... */
        for (i = 0; i < ssefillcount; ++i) {
            D(bug("[utility:pc] %s: fill ", __func__);)

            if ((ssefillcount - i) > 3)
            {
                D(bug("x4");)
                _mm_store_si128((__m128i*)p, c16);
                _mm_store_si128((__m128i*)((IPTR)p + 16), c16);
                _mm_store_si128((__m128i*)((IPTR)p + 32), c16);
                _mm_store_si128((__m128i*)((IPTR)p + 48), c16);
                p += (16 << 2);
                i += 3;
            }
            else if ((ssefillcount - i) > 1)
            {
                D(bug("x2");)
                _mm_store_si128((__m128i*)p, c16);
                _mm_store_si128((__m128i*)((IPTR)p + 16), c16);
                p += (16 << 1);
                i += 1;
            }
            else
            {
                D(bug("x1");)
                _mm_store_si128((__m128i*)p, c16);
                p += 16;
            }
            D(bug("\n");)
        }
    }
    else
    {
        postsize = length;
    }

    D(bug("[utility:pc] %s: 0x%p, %d\n", __func__, p, postsize);)

    /* file remainder ... */
    __smallsetmem(p, val, postsize);

    return destination;

    AROS_LIBFUNC_EXIT
}

#if defined(USE_SSE_COPYMEM)
AROS_MAKE_ALIAS(AROS_SLIB_ENTRY(SetMem_SSE,Utility,66),AROS_SLIB_ENTRY(SetMem,Utility,66));
#endif /* USE_SSE_COPYMEM */
#endif /* __SSE__ */
