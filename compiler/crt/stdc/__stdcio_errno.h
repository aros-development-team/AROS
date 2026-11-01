/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    errno for stdcio.library code, force-included into every stdcio
    source file (see mmakefile.src).

    stdcio reaches stdc through its own rellib StdCBase (opened by
    genmodule), and stdc.library is per-opener, so the plain errno
    macro would set the errno of that private base - never seen by the
    calling program. stdcio_startup records the program's own StdCBase
    in StdCIOBase->StdCBase; set errno there when it is known.
*/
#ifndef __STDCIO_ERRNO_H
#define __STDCIO_ERRNO_H

#include <errno.h>
#include <libraries/stdc.h>
#include <libraries/stdcio.h>

static inline int *__stdcio_errnoptr(void)
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    if (StdCIOBase != NULL && StdCIOBase->StdCBase != NULL)
        return &StdCIOBase->StdCBase->_errno;
    return __stdc_geterrnoptr();
}

#undef errno
#define errno (*__stdcio_errnoptr())

#endif /* __STDCIO_ERRNO_H */
