/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX/XSI function wcswidth().
*/

#include <wchar.h>

/****************************************************************************
    NAME
        int wcswidth(

    SYNOPSIS
        const wchar_t *pwcs, size_t n)

    FUNCTION
        Sum the terminal-column estimates for a bounded sequence
        of wide characters.

    INPUTS
        pwcs - Readable sequence ending at a null wide character
               or containing at least n elements. May be NULL when
               n is zero.
        n - Maximum number of wchar_t elements to examine, not
            a byte count or a column limit.

    RESULT
        The sum of wcwidth() results before the first null wide
        character or after n elements, whichever comes first.
        Returns -1 immediately if wcwidth() rejects an examined
        value; no partial sum is returned. Returns 0 when n is
        zero or the sequence starts with a null wide character.

    NOTES
        Include <wchar.h>. The sequence is not modified and its
        pointer is not retained. The function does not change
        errno. Widths are added per element; no grapheme-cluster
        analysis or terminal-state tracking is performed. See
        wcwidth() for classification limits.

    BUGS
        The sum is accumulated in int without an overflow check.
        Keep the accumulated width at or below INT_MAX; exceeding
        it causes signed integer overflow rather than a reported
        error.

    SEE ALSO
        wcwidth()

*****************************************************************************/
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
