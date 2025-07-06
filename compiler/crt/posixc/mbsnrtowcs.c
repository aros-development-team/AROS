/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the Posix function mbsnrtowcs().
*/

#include <proto/exec.h>

#include <stddef.h>
#include <wchar.h>
#include <errno.h>
#include <limits.h>

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
        size_t mbsnrtowcs(

/*  SYNOPSIS */
        wchar_t * restrict dst,
        const char ** restrict src,
        size_t nms,
        size_t len,
        mbstate_t * restrict ps
        )

/*    FUNCTION
        Converts at most 'nms' bytes from the multibyte string pointed to by *src
        into wide characters, storing at most 'len' wide characters in dst.

        The conversion uses the current locale (UTF-8 assumed here) and updates
        the src pointer to point past the last converted multibyte character.

    INPUTS
        dst  - Pointer to destination wide character buffer (may be NULL).
        src  - Address of the multibyte character string pointer. The pointer is updated
               during conversion. If dst is NULL, the function returns the number
               of wide characters that would result from the conversion.
        nms  - Maximum number of bytes to convert from the multibyte string.
        len  - Maximum number of wide characters to write.
        ps   - Conversion state object (ignored in this implementation).

    RESULT
        Returns the number of wide characters written or needed.
        Returns (size_t)-1 on conversion error and sets errno to EILSEQ.

    NOTES
        This implementation assumes stateless UTF-8 encoding and does not
        currently use mbstate_t. It wraps around mbrtowc().

    SEE ALSO
        wcsnrtombs(),
        stdc.library/mbrtowc(), stdc.library/mbstowcs(), stdc.library/wcsrtombs()

******************************************************************************/
{
    size_t count = 0;
    size_t res;
    const char *s;
    wchar_t wc;

    if (!src || !*src)
        return 0;

    s = *src;

    while (nms > 0) {
        res = mbrtowc(&wc, s, nms, ps);
        if (res == (size_t)-1) {
            errno = EILSEQ;
            return (size_t)-1;
        } else if (res == (size_t)-2) {
            // Incomplete multibyte sequence
            break;
        } else if (res == 0) {
            // Null terminator found
            if (count < len && dst)
                dst[count] = L'\0';
            ++count;
            s += 1; // null character consumes one byte
            *src = NULL;
            return count;
        }

        if (count < len && dst)
            dst[count] = wc;

        ++count;
        s += res;
        nms -= res;
    }

    *src = s;
    return count;
}
