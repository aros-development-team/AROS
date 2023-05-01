/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.

    C99 function wcscmp().
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

        int wcscmp(

/*  SYNOPSIS */
        const wchar_t *wcstra,
        const wchar_t *wcstrb)

/*  FUNCTION
        Lexicographically compares two null-terminated wide strings.

    INPUTS
        wcstra - wide string to compare.
        wcstrb - wide string to compare.

    RESULT
        If wcstra appears before wcstrb lexographically, then -1 is returned.
        If wcstrb appears before wcstra lexographically, then 1 is returned.
        If the strings are equal, 0 is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    while (*wcstra &&
               *wcstrb &&
               (*wcstra == *wcstrb))
    {
        wcstra++;
        wcstrb++;
    }

    if (*wcstra < *wcstrb)
        return -1;
    else if (*wcstra > *wcstrb)
        return 1;
    return 0;
} /* wcscmp */
