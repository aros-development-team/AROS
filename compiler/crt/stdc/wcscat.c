/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.

    C99 function wcscat().
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

        wchar_t *wcscat(

/*  SYNOPSIS */
        wchar_t *wcdst,
        const wchar_t *wcsrc)

/*  FUNCTION
        Appends a wide string, onto another wide string.

    INPUTS
        wcdst - the wide string that will have the text appended to it.
        wcsrc - the wide string to append.

    RESULT
        A pointer to the resulting wide string.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    wchar_t *_wcptr = wcdst;

    /* skip existing characters ... */
    while (*_wcptr) _wcptr++;

    /* and copy the string .. */
    wcscpy(_wcptr, wcsrc);

    return wcdst;
} /* wcscat */
