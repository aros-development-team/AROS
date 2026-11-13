/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function setvbuf().
*/

#include <dos/stdio.h>
#include <proto/dos.h>
#include <errno.h>
#include "__fdesc.h"
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        int setvbuf (

/*  SYNOPSIS */
        FILE *stream,
        char *buf,
        int mode,
        size_t size)

/*  FUNCTION
        Sets the buffering mode and buffer size for a given stream.

        This function must be called after the stream is opened but before
        any other operations (such as reading or writing) are performed
        on it. The user can supply their own buffer, or pass NULL to let
        the system allocate one. The mode specifies how the buffer is used.

    INPUTS
        stream  - Pointer to a valid FILE object.
        buf     - User-provided buffer (or NULL for system-allocated).
        mode    - One of _IOFBF (full), _IOLBF (line), _IONBF (none).
        size    - Size of the buffer (must meet platform-specific minimum if buf is not NULL).

    RESULT
        Returns 0 on success, or EOF (-1) on failure. If an error occurs,
        `errno` is set to indicate the cause (e.g., EFAULT, EINVAL, EBADF).

    NOTES
        - This function must be called before any stream I/O.
        - Supplying a buffer smaller than the platform minimum will result
          in an error.
        - Mode must be a valid constant (_IOFBF, _IOLBF, or _IONBF).
        - If `size` is 0, the system will determine an appropriate buffer size.

    EXAMPLE
        FILE *fp = fopen("example.txt", "w");
        char buffer[512];
        if (setvbuf(fp, buffer, _IOFBF, sizeof(buffer)) != 0)
        {
            perror("Failed to set buffer");
        }

    BUGS
        - No runtime check ensures that the stream hasn't already been used.
        - The required minimum buffer size (208 bytes) is AROS-specific and
          may not match other platforms' expectations.

    SEE ALSO
        fopen(), fflush(), fclose(), perror()

    INTERNALS
        - This implementation maps C standard buffering modes to internal
          AROS constants: _IOFBF ? BUF_FULL, _IOLBF ? BUF_LINE, _IONBF ? BUF_NONE.
        - Uses `__getfdesc()` to access the internal file descriptor.
        - Buffer size is validated (must be at least 208 bytes if user-supplied).
        - The actual work is delegated to AROS’s DOS-level `SetVBuf()` function.
        - If `size` is 0, -1 is passed to `SetVBuf()` to use a default size.

******************************************************************************/
{
    fdesc *desc;

    if (!stream)
    {
        errno = EFAULT;
        return EOF;
    }

    /* Fail if provided buffer is smaller than minimum required by DOS */
    if (buf && size < 208)
    {
        errno = EFAULT;
        return EOF;
    }

    switch (mode)
    {
        case _IOFBF: mode = BUF_FULL; break;
        case _IOLBF: mode = BUF_LINE; break;
        case _IONBF: mode = BUF_NONE; break;
        default:
            errno = EINVAL;
            return EOF;
    }

    desc = __getfdesc(stream->fd);
    if (!desc)
    {
        errno = EBADF;
        return EOF;
    }

    return SetVBuf(desc->fcb->handle, buf, mode, size ? size : -1);
} /* setvbuf */

