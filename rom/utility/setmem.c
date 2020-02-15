/*
    Copyright � 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/utility.h>

        AROS_LH3(APTR, SetMem,

/*  SYNOPSIS */
        AROS_LHA(APTR, destination, A0),
        AROS_LHA(UBYTE, c, D0),
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

    UBYTE *bptr = destination;
    ULONG *uptr;
    ULONG lfill = c | (c << 8);
    lfill = lfill | (lfill << 16);
    BYTE prefill = (((IPTR)bptr + AROS_LONGALIGN - 1) & ~(AROS_LONGALIGN - 1)) - (IPTR)bptr;

    /* Prefill aligns pointer to the AROS_LONGALIGN boundary */
    while (prefill-- && length) {
        *bptr++ = lfill;
        length--;
    }
    
    uptr = (ULONG*)bptr;

    /* Unroll the LONG fill loop a little */
    while (length >= 16)
    {
        *uptr++ = lfill;
        *uptr++ = lfill;
        *uptr++ = lfill;
        *uptr++ = lfill;
        length = length - 16;
    }

    /* Complete the LONG fill loop */
    while (length >= 4)
    {
        *uptr++ = lfill;
        length = length - 4;
    }

    bptr = (UBYTE *)uptr;

    /* Finalize the fill with bytes */
    while (length--)
        *bptr++ = lfill;

    return destination;

    AROS_LIBFUNC_EXIT
}
