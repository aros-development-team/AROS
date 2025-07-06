/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function mbsrtowcs().
*/

#include <stddef.h>
#include <wchar.h>
#include <errno.h>
#include <limits.h>

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
        size_t mbsrtowcs(

/*  SYNOPSIS */
        wchar_t * restrict dst,
        const char ** restrict src,
        size_t len,
        mbstate_t * restrict ps)

/*  FUNCTION
        Converts a null-terminated multibyte character string pointed to by *src
        into a corresponding wide character string.

        The conversion uses the current locale (UTF-8 assumed here) and updates
        the src pointer to point past the last converted multibyte character.

    INPUTS
        dst  - Pointer to destination wide character buffer (may be NULL).
        src  - Address of the multibyte string pointer. The pointer is updated
               during conversion. If dst is NULL, the function returns the
               number of wide characters that would result from the conversion.
        len  - Maximum number of wide characters to write.
        ps   - Conversion state object (ignored in this implementation).

    RESULT
        Returns the number of wide characters written or needed.
        Returns (size_t)-1 on conversion error and sets errno to EILSEQ.

    NOTES
        This implementation assumes stateless UTF-8 encoding and does not
        currently use mbstate_t. It wraps around mbrtowc().

    SEE ALSO
        mbrtowc(), mbstowcs(), wcstombs()

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    size_t count = 0;
    size_t res;
    wchar_t wc;
    const char *s;

    if (!src || !*src)
        return 0;

    s = *src;

    while (count < len || dst == NULL) {
        res = mbrtowc(&wc, s, StdCBase->__locale_cur->__lc_mb_max, ps);
        if (res == (size_t)-1) {
            errno = EILSEQ;
            return (size_t)-1;
        }
        if (res == (size_t)-2) {
            // Incomplete sequence, shouldn't occur with null-terminated strings
            break;
        }
        if (res == 0) {
            if (dst)
                dst[count] = L'\0';
            s += 1;
            count++;
            break;
        }

        if (dst)
            dst[count] = wc;

        s += res;
        count++;
    }

    *src = s;
    return count;
}
