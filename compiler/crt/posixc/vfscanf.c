/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function vfscanf()
*/

/* Original source from libnix */

#include <proto/dos.h>
#include <errno.h>
#include <stdarg.h>
#include "__fdesc.h"
#include "__stdio.h"

/*
** If the VFSCANF_DIRECT_DOS define is set to 1, dos.library functions FGetC()
** and UnGetC() are used directly, for possibly better speed. Otherwise
** the clib functions fgetc/ungetc are used.
*/
 
#define VFSCANF_DIRECT_DOS 1

#if VFSCANF_DIRECT_DOS

struct __vfscanf_handle
{
    FILE  *stream;
    fdesc *fdesc;
};

static int __getc(void *_h);
static int __ungetc(int c, void *_h);

#endif

/*****************************************************************************

    NAME */
#include <stdio.h>

        int vfscanf (

/*  SYNOPSIS */
        FILE       * stream,
        const char * format,
        va_list      args)

/*  FUNCTION
        Reads input from the specified stream, interprets it according to
        the provided format string, and stores the results in the locations
        pointed to by the variable argument list `args`.

    INPUTS
        stream - A pointer to a readable input stream.
        format - A scanf-style format string describing expected input format.
        args   - A `va_list` containing pointers to the variables where
                 the results of the formatted input will be stored.

    RESULT
        Returns the number of input items successfully matched and assigned,
        which may be fewer than provided for, or zero in the event of an
        early matching failure. Returns 0 if the stream is invalid.

    NOTES
        - Whitespace in the format string matches any amount of whitespace
          in the input.
        - Input fields must match the expected format exactly or parsing will stop.
        - This implementation includes both direct DOS-level and POSIX-style
          fallback code paths, depending on compile-time `VFSCANF_DIRECT_DOS`.

    EXAMPLE
        va_list ap;
        va_start(ap, fmt);
        vfscanf(stdin, "%d %s", ap);
        va_end(ap);

    BUGS
        - Returns 0 on stream error instead of EOF (-1), which may not conform
          strictly to standard expectations.
        - Error and EOF handling uses AROS-specific flags and conventions.
        - `VFSCANF_DIRECT_DOS` path relies on AROS `UnGetC()` and `Flush()` calls,
          making portability limited.

    SEE ALSO
        scanf(), fscanf(), vscanf(),
        stdc.library/sscanf(), stdc.library/vsscanf(),
        va_start(), va_list

    INTERNALS
        - Uses `__getfdesc()` to get AROS-specific file descriptor structure.
        - Flushes stream handle before reading.
        - Calls `__vcscan()` to perform actual parsing.
        - If `VFSCANF_DIRECT_DOS` is defined:
            * Uses a custom `__vfscanf_handle` struct.
            * Input and unput functions (`__getc`, `__ungetc`) are passed
              as callbacks to `__vcscan()`.
            * `__getc.c` is included directly to implement common input logic.
        - Otherwise, uses standard `fgetc` and `ungetc`.

******************************************************************************/
{
#if VFSCANF_DIRECT_DOS
    struct __vfscanf_handle h;
    
    h.stream = stream;
    h.fdesc  = __getfdesc(stream->fd);

    if (!h.fdesc)
    {
        errno = EBADF;
        return 0;
    }


    Flush (h.fdesc->fcb->handle);

    return __vcscan (&h, __getc, __ungetc, format, args);
#else
    fdesc *fdesc;
    
    fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;
        return 0;
    }

    Flush (fdesc->fcb->handle);
    
    return __vcscan (stream,  __posixc_fgetc, ungetc, format, args);
       
#endif
} /* vfscanf */

#if VFSCANF_DIRECT_DOS

static int __ungetc(int c, void *_h)
{
    struct __vfscanf_handle *h = _h;
    /* Note: changes here might require changes in ungetc.c!! */

    if (c < -1)
        c = (unsigned int)c;

    if (!UnGetC(h->fdesc->fcb->handle, c))
    {
        errno = __stdc_ioerr2errno(IoErr());

        if (errno)
        {
            h->stream->flags |= __POSIXC_STDIO_ERROR;
        }
        else
        {
            h->stream->flags |= __POSIXC_STDIO_EOF;
        }
        
        c = EOF;
    }

    return c;
}

static int __getc(void *_h)
{
    struct __vfscanf_handle *h = _h;
    fdesc *fdesc = h->fdesc;
    int c;

/* include the common posixc getc code */
#define getcstream h->stream
#include "__getc.c"
}

#endif
