/*
    Copyright (C) 2007-2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function wcstombs().
*/

#include <proto/exec.h>
#include <stddef.h>
#include <wchar.h>
#include <errno.h>

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        size_t wcstombs(

/*  SYNOPSIS */
        char * restrict dest,
        const wchar_t * restrict src,
        size_t n)

/*  FUNCTION
        Converts a null-terminated wide character string (wchar_t*) into its
        corresponding multibyte string (char*), using the current locale's encoding.
        At most 'n' bytes are written into 'dest', not including the terminating null byte.

    INPUTS
        dest - Pointer to the destination buffer that receives the converted multibyte string.
               If NULL, the function returns the number of bytes that would be written, not
               including the null terminator.

        src  - Pointer to the null-terminated wide character string to convert.

        n    - Maximum number of bytes to write into 'dest'.

    RESULT
        Returns the number of bytes stored in 'dest', excluding the terminating null byte.
        If a wide character cannot be converted to a valid multibyte sequence, the function
        returns (size_t)-1 and sets errno to EILSEQ.

        If 'dest' is NULL, the function returns the number of bytes that would be needed
        to encode the wide string, excluding the terminating null byte.

    NOTES
        stdc.library currently only implements "C" or UTF-8 compatible locale behavior.
        The encoding is treated as stateless and locale-independent.
        The terminating null character (L'\0') is converted, but the function does not
        include it in the returned count.

    EXAMPLE
        wchar_t wstr[] = L"Hello ??!";
        char buffer[64];
        size_t len = wcstombs(buffer, wstr, sizeof(buffer));
        if (len != (size_t)-1) {
            // buffer now contains UTF-8-encoded string
        }

    BUGS
        Does not handle stateful encodings or locale-specific behavior.
        Buffer overflow is possible if 'n' is too small to hold even one multibyte character.

    SEE ALSO
        wcrtomb(), mbstowcs(), wctomb(), mbtowc()

    INTERNALS
        Iterates through each wchar_t in 'src', converting using wcrtomb().
        If 'dest' is NULL, the total number of bytes is computed without writing.
        The function stops once either the null terminator is reached or the byte limit 'n' is exceeded.

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    size_t total = 0;
    size_t len;
    char *buf;              // max multibyte length (typically 4 for UTF-8)
    mbstate_t ps;          // unused in our `wcrtomb`, but here for completeness

    // Clear state (not used in your wcrtomb, but conventionally passed)
    ps = (mbstate_t){0};
    buf = AllocVec(StdCBase->__locale_cur->__lc_mb_max, MEMF_ANY);

    while (*src) {
        len = wcrtomb(buf, *src, &ps);
        if (len == (size_t)-1)
        {
            FreeVec(buf);
            return (size_t)-1;
        }

        if (total + len > n)
            break;

        if (dest) {
            for (size_t i = 0; i < len; ++i)
                dest[total + i] = buf[i];
        }

        total += len;
        ++src;
    }
    FreeVec(buf);

    return total;
}
