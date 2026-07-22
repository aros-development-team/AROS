/*
    Copyright (C) 2012-2013, The AROS Development Team. All rights reserved.

    Get pointer to errno variable in stdc.library libbase.
    This function is in both the static linklib and stdc.library.
*/
#ifndef STDC_STATIC
#include <libraries/stdc.h>
#endif

int *__stdc_geterrnoptr(void)
{
#ifdef STDC_STATIC
    static int static_errno;

    return &static_errno;
#else
    return &(__aros_getbase_StdCBase()->_errno);
#endif
}
