/*
 * Copyright (C) 2012-2015, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/alib.h>

	APTR LibAllocAligned (

/*  SYNOPSIS */
	IPTR memSize,
	ULONG requirements,
	IPTR  alignBytes)

/*  FUNCTION
        Allocate a memory block with a start address and size that are
        multiples of a power-of-two. The returned memory must be freed using
        FreeMem().

    INPUTS
        memSize      - Size in bytes of the aligned area
        requirements - Memory requirements (same as AllocMem())
        alignBytes   - Required alignment, in bytes.
                       This must be a power of 2!

    RESULT
        Pointer to the newly allocated area, or NULL if no satisfying memory
        can be found.

    NOTES
        If alignBytes is not a power of two, NULL is returned.

        If memSize is not a multiple of alignBytes, NULL is returned.

    EXAMPLE

    BUGS

    SEE ALSO
        exec.library/AllocMem, exec.library/FreeMem().

    INTERNALS

******************************************************************************/
{
    APTR ptr;
    IPTR alignMask;

    /* Verify that alignBytes is a power of two */
    if ((alignBytes & (alignBytes-1)) != 0)
        return NULL;

    /* Verify that memSize is modulo alignBytes */
    if ((memSize & (alignBytes - 1)) != 0)
        return NULL;

    alignMask = alignBytes - 1;

    if ((ptr = AllocMem(memSize + alignMask, requirements)))
    {
        APTR aptr = (APTR)((((IPTR)ptr) + alignMask) & ~alignMask);
        Forbid();
        FreeMem(ptr, memSize + alignMask);
        ptr = AllocAbs(memSize, aptr);
        Permit();
    }

    return ptr;
}
