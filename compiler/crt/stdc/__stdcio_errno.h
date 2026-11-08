/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    errno for stdcio.library code, force-included into every stdcio
    source file (see mmakefile.src).

    stdcio reaches stdc through its own rellib StdCBase (opened by
    genmodule). errno is per thread (__stdc_threaderrno.c): whichever base
    __stdc_geterrnoptr() is called through, it answers the calling thread's
    errno, the one the program reads. Writing a base's _errno field directly
    (as this header once did, to the program's base) misses that location.
*/
#ifndef __STDCIO_ERRNO_H
#define __STDCIO_ERRNO_H

#include <errno.h>

#undef errno
#define errno (*__stdc_geterrnoptr())

#endif /* __STDCIO_ERRNO_H */
