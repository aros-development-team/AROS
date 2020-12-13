#ifdef __AVX__
/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <exec/types.h>

#include "setmem_pc.h"

/****i************************************************************************

    NAME */
#include <proto/utility.h>

        AROS_LH3(APTR, SetMem_AVX,

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

    if (length > 31)
    {
        ULONG presize, avxfillcount;

        presize = (((IPTR) destination + 31 ) & ~31) - (IPTR)destination;
        avxfillcount = (length - presize) / 32;
        postsize = length - avxfillcount * 32 - presize;

        /* setup avx value .. */
        __m256i c32 = _mm256_set1_epi8(val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val, val);

        D(bug("[utility:pc] %s: 0x%p, %d\n", __func__, p, presize));

        /* fill inital bytes ... */
        __smallsetmem(p, val, presize);
        p += presize;

        D(bug("[utility:pc] %s: 0x%p, %d x 32\n", __func__, p, avxfillcount));

        /* avx fill 32bytes at a time ... */
        for (i = 0; i < avxfillcount; ++i) {
            if ((avxfillcount - i) > 3)
            {
                _mm256_store_si256((__m256i*)p, c32);
                _mm256_store_si256((__m256i*)((IPTR)p + 32), c32);
                _mm256_store_si256((__m256i*)((IPTR)p + 64), c32);
                _mm256_store_si256((__m256i*)((IPTR)p + 96), c32);
                p += (32 << 2);
                i += 3;
            }
            if ((avxfillcount - i) > 1)
            {
                _mm256_store_si256((__m256i*)p, c32);
                _mm256_store_si256((__m256i*)((IPTR)p + 32), c32);
                p += (32 << 1);
                i += 1;
            }
            else
            {
                _mm256_store_si256((__m256i*)p, c32);
                p += 32;
            }
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

#if defined(USE_AVX_COPYMEM)
AROS_MAKE_ALIAS(AROS_SLIB_ENTRY(SetMem_AVX,Utility,66),AROS_SLIB_ENTRY(SetMem,Utility,66));
#endif /* USE_AVX_COPYMEM */
#endif /* __AVX__ */
