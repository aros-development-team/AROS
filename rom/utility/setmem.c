/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */
#include <proto/utility.h>

        AROS_LH3(APTR, SetMem,

/*  SYNOPSIS */
        AROS_LHA(APTR, destination, A0),
        AROS_LHA(ULONG, c, D0),
        AROS_LHA(LONG, length, D1),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 66, Utility)

/*  FUNCTION
        Fill a memory block with a Byte.

    INPUTS
        destination - address where the filling starts
        c           - value to be filled in
        length      - number of Bytes to be filled in

    RESULT
        The destination address

    NOTES

    EXAMPLE
        SetMem(addr, 10, 100);

    BUGS

    SEE ALSO

    INTERNALS
        There are platform dependent variants of this function.
        

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    register UBYTE *ptr;
    ULONG postsize;

    ptr = destination;

    if (length > AROS_LONGALIGN)
    {
        BYTE prefill = (((IPTR)ptr + AROS_LONGALIGN) & ~(AROS_LONGALIGN-1)) % AROS_LONGALIGN;
        WORD longfill = (length - prefill) / AROS_LONGALIGN;
        postsize = length - longfill * AROS_LONGALIGN - prefill;

        while (prefill--)
            *ptr ++ = c;

        if (longfill > 0)
        {
            ULONG * ulptr = (ULONG *)ptr;
            ULONG fill = ((c & 0xFF) <<  8) | (c & 0xFF);
            fill = (fill << 16) | fill;
            while ((longfill > 1) && ((longfill -= 2) > 0))
            {
                *ulptr ++ = fill;
                *ulptr ++ = fill;
            }

            while (longfill--)
                *ulptr ++ = fill;

            ptr = (UBYTE *)ulptr;
        }
    }
    else
        postsize = length;

    while (postsize--)
        *ptr ++ = c;

    return destination;

    AROS_LIBFUNC_EXIT
}
