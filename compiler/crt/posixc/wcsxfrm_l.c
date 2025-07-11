/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcsxfrm_l.
*/

#include <wchar.h>

size_t wcsxfrm_l(wchar_t *dest, const wchar_t *src, size_t n, locale_t loc)
{
    return wcsxfrm(dest, src, n);
}
