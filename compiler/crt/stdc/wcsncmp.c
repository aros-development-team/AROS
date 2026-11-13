/*
    Copyright (C) 2020-2025, The AROS Development Team. All rights reserved.

    C99 function wcsncmp().
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

        int wcsncmp(

/*  SYNOPSIS */
        const wchar_t *wcstra,
        const wchar_t *wcstrb,
        size_t cnt)

/*  FUNCTION
        Lexicographically compares two null-terminated wide strings, upto the
        specified length

    INPUTS
        wcstra - wide string to compare.
        wcstrb - wide string to compare.
        cnt - maximum number of wide characters to compare.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    while (cnt-- > 0)
    {
        wchar_t stra = *wcstra++;
        wchar_t strb = *wcstrb++;
        if (stra == L'\0' || stra != strb)
        {
        if (stra < strb)
            return -1;
        else if (stra > strb)
            return 1;
        else
            break;
        }
    }
    return 0;
} /* wcsncmp */
