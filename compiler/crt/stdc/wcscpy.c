/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.

    C99 function wcscpy().
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

        wchar_t *wcscpy(

/*  SYNOPSIS */
        wchar_t *wcdst,
        const wchar_t *wcsrc)

/*  FUNCTION
        Copies a wide string, to another wide string.

    INPUTS
        wcdst - the wide string that will be copied to.
        wcsrc - the wide string to copy.

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
    for (_wcptr = wcdst; *wcsrc; wcsrc++)
        *_wcptr++ = *wcsrc;

    /* terminate the wide string */
    *_wcptr = 0x0000;

    return wcdst;
} /* wcscpy */
