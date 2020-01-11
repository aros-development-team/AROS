/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
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

    UBYTE * ptr = destination;

    while (((IPTR)ptr)&(AROS_LONGALIGN-1) && length)
    {
        *ptr ++ = c;
        length --;
    }

    if (length > sizeof(ULONG))
    {
        ULONG * ulptr = (ULONG *)ptr;
        ULONG fill;

        fill = (ULONG)(c & 0xFF);
        fill = (fill <<  8) | fill;
        fill = (fill << 16) | fill;

        while (length > sizeof(ULONG))
        {
            *ulptr ++ = fill;
            length -= sizeof(ULONG);
        }

        ptr = (UBYTE *)ulptr;
    }

    while (length --)
        *ptr ++ = c;

    return destination;

    AROS_LIBFUNC_EXIT
}
