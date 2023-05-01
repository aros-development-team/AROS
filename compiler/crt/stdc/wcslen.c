/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.

    C99 function wcslen().
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

        size_t wcslen(

/*  SYNOPSIS */
        const wchar_t *wcstr)

/*  FUNCTION
        Returns the length of a wide string.

    INPUTS
        wcstr - wide string to tally.

    RESULT
        The number of non-null wide characters, in the wide string.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int cnt = 0;

    while (*wcstr++) cnt++;

    return cnt;
} /* wcslen */
