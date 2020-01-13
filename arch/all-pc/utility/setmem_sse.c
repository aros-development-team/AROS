#ifdef __SSE__
/*
    Copyright � 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <exec/types.h>

#include <emmintrin.h>

static VOID __smallsetmem(APTR destination, UBYTE c, ULONG length)
{
    int i;

    register char *p = (char *) destination;

    for (i = 0; i < length; i++) {
        p[i] = c;
    }
}

/*****************************************************************************

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

    D(bug("[utility:pc] %s(0x%p, %02x, %d)\n", __func__, destination, c, length));

    p = (char *) destination;
    val = (char) c;

    if (length > 15)
    {
        ULONG presize, ssefillcount;

        presize = (((IPTR) destination + 15 ) & ~15) - (IPTR)destination;
        ssefillcount = (length - presize) / 16;
        postsize = length - ssefillcount * 16 - presize;

        /* setup sse value .. */
        __m128i c16 = _mm_set_epi8(val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val);

        D(bug("[utility:pc] %s: 0x%p, %d\n", __func__, p, presize));

        /* fill inital bytes ... */
        __smallsetmem(p, val, presize);
        p += presize;

        D(bug("[utility:pc] %s: 0x%p, %d x 16\n", __func__, p, ssefillcount));

        /* sse fill 16bytes at a time ... */
        for (i = 0; i < ssefillcount; ++i) {
            _mm_store_si128((__m128i*)p, c16);
            p += 16;
        }
    }
    else
    {
        postsize = length;
    }

    D(bug("[utility:pc] %s: 0x%p, %d\n", __func__, p, postsize));

    /* file remainder ... */
    __smallsetmem(p, val, postsize);

    return destination;

    AROS_LIBFUNC_EXIT
}

#if defined(USE_SSE_COPYMEM)
AROS_MAKE_ALIAS(AROS_SLIB_ENTRY(SetMem_SSE,Utility,66),AROS_SLIB_ENTRY(SetMem,Utility,66));
#endif /* USE_SSE_COPYMEM */
#endif /* __SSE__ */
