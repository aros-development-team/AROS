/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX/XSI function wcswidth().
*/

#include <wchar.h>

int wcswidth(const wchar_t *pwcs, size_t n)
{
    int width = 0;

    for (; n > 0 && *pwcs != L'\0'; n--, pwcs++)
    {
        int w = wcwidth(*pwcs);
        if (w < 0)
            return -1;
        width += w;
    }

    return width;
}
