/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Change the position in a stream.
*/

#include <fcntl.h>
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "__stdio.h"
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        int fseek (

/*  SYNOPSIS */
        FILE * stream,
        long   offset,
        int    whence)

/*  FUNCTION
        Change the current position in a stream.

    INPUTS
        stream - Modify this stream
        offset, whence - How to modify the current position. whence
                can be SEEK_SET, then offset is the absolute position
                in the file (0 is the first byte), SEEK_CUR then the
                position will change by offset (ie. -5 means to move
                5 bytes to the beginning of the file) or SEEK_END.
                SEEK_END means that the offset is relative to the
                end of the file (-1 is the last byte and 0 is
                the EOF).

    RESULT
        0 on success and -1 on error. If an error occurred, the global
        variable errno is set.

    NOTES

    EXAMPLE

    BUGS
    Not fully compatible with ISO fseek, especially in 'ab' and 'a+b'
    modes

    Since it's not possible to use Seek() for directories, this
    implementation fails with EISDIR for directory file descriptors.

    SEE ALSO
        fopen(), fwrite()

    INTERNALS

******************************************************************************/
{
    /* fseek is equivalent to fseeko with a (possibly narrower) offset;
       the shared engine handles 64-bit positions via dos64.library
       when available */
    return __fseeko64 (stream, (off64_t)offset, whence);
} /* fseek */
