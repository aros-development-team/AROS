/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME
#include <string.h>

        void *memcpy (

    SYNOPSIS
        void * restrict dst0,
        const void * restrict src0,
        size_t length)

    FUNCTION
        Copy a block of memory; handling of overlapping regions is not
        guaranteed.

    INPUTS
        dst0: destination for copy
        src0: source for copy
        length: number of bytes to copy

    RESULT
        dst0

    NOTES
        stdc.library/memcpy() is an alias to stdc.library/memmove()
        So overlapping regions are handled OK if this function is used.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
