/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Read from a file at a given offset.
*/

/*****************************************************************************

    NAME */
#include <unistd.h>

        ssize_t pread (

/*  SYNOPSIS */
        int     d,
        void   *buf,
        size_t  nbytes,
        off_t   offset)

/*  FUNCTION
        Read up to nbytes from the file at offset, leaving the file offset
        of the descriptor unchanged.

    INPUTS
        d      - file descriptor to read from
        buf    - where to put the data
        nbytes - number of bytes to read
        offset - position in the file to read from

    RESULT
        The number of bytes read, 0 at or beyond the end of the file, or -1
        with errno set.

    NOTES
        See pread64(), which this calls.

    EXAMPLE

    BUGS

    SEE ALSO
        pread64(), read(), lseek()

    INTERNALS

******************************************************************************/
{
    return pread64(d, buf, nbytes, offset);
} /* pread */
