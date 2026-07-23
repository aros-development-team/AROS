/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>
#include <stdio.h>

#include <proto/dos.h>

#define POSIXC_NOSTDIO_DECL

#include "__stdio.h"
#include "__fdesc.h"
#include "__dos64.h"

/*
 * ftello engine: full 64-bit position via the __dos64 helpers.
 */
off64_t __ftello64(FILE *stream)
{
    QUAD cnt;
    BPTR fh;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;
        return -1;
    }

    if (fdesc->fcb->privflags & _FCB_ISDIR)
    {
        errno = EISDIR;
        return -1;
    }

    fh = fdesc->fcb->handle;

    Flush (fh);
    cnt = __dos64_getpos (fdesc->fcb);

    if (cnt == -1)
        errno = __stdc_ioerr2errno (IoErr ());

    return cnt;
}

#if (__WORDSIZE < 64)
/*
 * 32-bit off_t variant: positions beyond off_t range report EOVERFLOW.
 */
off_t __ftello(FILE *stream)
{
    QUAD cnt = __ftello64(stream);

    if (cnt != (QUAD)(off_t)cnt)
    {
        errno = EOVERFLOW;
        return (off_t)-1;
    }
    return (off_t)cnt;
}
#else
/*
 * on 64bit ftello is an alias of ftello64
 */
AROS_MAKE_ALIAS(__ftello64,__ftello);
#endif
