/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.

    C99 function wcsncpy().
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

        wchar_t *wcsncpy(

/*  SYNOPSIS */
        wchar_t *wcdst,
        const wchar_t *wcsrc,
        size_t cnt)

/*  FUNCTION
        Copies upto the specified length of wide characters from
        a wide string, to another wide string.

    INPUTS
        wcdst - the wide string that will be copied to.
        wcsrc - the wide string to copy.
        cnt - maximum number of wide characters to copy.

    RESULT
        A pointer to the resulting wide string.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    wchar_t *_wcptr;

    /* copy the wide characters */
    for (_wcptr = wcdst; (cnt-- > 0) & *wcsrc; wcsrc++)
        *_wcptr++ = *wcsrc;

    /* terminate the wide string, and fill the remaining
     * wide characters as specified in the spec... */
    while (cnt-- > 0)
        *_wcptr++ = 0x0000;

    return wcdst;
} /* wcsncpy */
