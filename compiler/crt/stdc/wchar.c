/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 wchar linklib stubs.
*/

#include <aros/debug.h>

#include <aros/types/mbstate_t.h>

#include <string.h>

#define STDC_NOINLINE_WCHAR

/*****************************************************************************

    NAME */
#include <wchar.h>

/* This doesnt really belong here, but it gets it in the linklib.. */
int wcwidth(wchar_t wc) {
    if (wc == 0)
        return 0;
    if (wc < 32 || (wc >= 0x7f && wc < 0xa0))
        return -1; // control characters

#if defined(__WCHAR_MAX__) && (__WCHAR_MAX__ > 255)
    // Wide (East Asian Wide/Fullwidth) range
    if ((wc >= 0x1100 && wc <= 0x115F) || // Hangul Jamo
        (wc >= 0x2329 && wc <= 0x232A) ||
        (wc >= 0x2E80 && wc <= 0xA4CF) || // CJK...
        (wc >= 0xAC00 && wc <= 0xD7A3) ||
        (wc >= 0xF900 && wc <= 0xFAFF) ||
        (wc >= 0xFE10 && wc <= 0xFE19) ||
        (wc >= 0xFE30 && wc <= 0xFE6F) ||
        (wc >= 0xFF00 && wc <= 0xFF60) ||
        (wc >= 0xFFE0 && wc <= 0xFFE6))
        return 2;
#endif

    return 1;
}

int wcswidth(const wchar_t *pwcs, size_t n) {
    int width = 0;
    for (size_t i = 0; i < n && pwcs[i] != L'\0'; ++i) {
        int w = wcwidth(pwcs[i]);
        if (w < 0)
            return -1;
        width += w;
    }
    return width;
}

int mbsinit(const mbstate_t *ps)
{
    return (ps == NULL) || (ps->__state == 0 && ps->__count == 0 && ps->__value == 0);
}
