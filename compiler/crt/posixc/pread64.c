/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Read from a file at a given offset.
*/
#include <errno.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

        ssize_t pread64 (

/*  SYNOPSIS */
        int        d,
        void      *buf,
        size_t     nbytes,
        __off64_t  offset)

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
        Implemented with lseek64() and read(), so unlike POSIX it is not
        atomic: another thread using the same descriptor at the same time
        may see the offset move. Since lseek() past the end extends a file,
        an offset at or beyond the end returns 0 without seeking.

    EXAMPLE

    BUGS

    SEE ALSO
        pread(), read(), lseek64()

    INTERNALS

******************************************************************************/
{
    __off64_t old, size;
    ssize_t cnt;
    int error;

    if (offset < 0)
    {
        errno = EINVAL;
        return -1;
    }

    old = lseek64(d, 0, SEEK_CUR);
    if (old == -1)
        return -1;
    size = lseek64(d, 0, SEEK_END);
    if (size == -1)
        return -1;
    if (offset >= size || nbytes == 0)
    {
        lseek64(d, old, SEEK_SET);
        return 0;
    }

    if (lseek64(d, offset, SEEK_SET) == -1)
        cnt = -1;
    else
        cnt = read(d, buf, nbytes);

    error = errno;
    lseek64(d, old, SEEK_SET);
    errno = error;
    return cnt;
} /* pread64 */
