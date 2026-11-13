/*
    Copyright (C) 2012-2026, The AROS Development Team. All rights reserved.

    Get pointer to the calling thread's errno variable.
    This function is in both the static linklib and stdc.library.
*/
#ifndef STDC_STATIC
#include <libraries/stdc.h>
#include <proto/stdc.h>
#endif

int *__stdc_geterrnoptr(void)
{
#ifdef STDC_STATIC
    static int static_errno;

    return &static_errno;
#else
    /* errno belongs to the calling thread, whichever base this is called
       through: a pthread worker has one of its own (__stdc_threaderrno.c) */
    return __stdc_threaderrnoptr();
#endif
}
