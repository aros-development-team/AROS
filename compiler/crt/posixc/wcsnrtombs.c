/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the Posix function wcsnrtombs().
*/

#include <proto/exec.h>

#include <stddef.h>
#include <wchar.h>
#include <errno.h>
#include <limits.h>

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
        size_t wcsnrtombs(

/*  SYNOPSIS */
        char * restrict dst,
        const wchar_t ** restrict src,
        size_t nwc,
        size_t len,
        mbstate_t * restrict ps
        )

/*    FUNCTION
        Converts at most 'nwc' wide characters from the string pointed to by *src
        into a corresponding multibyte character string.

        The conversion uses the current locale (UTF-8 assumed here) and updates
        the src pointer to point past the last converted wide character.

    INPUTS
        dst  - Pointer to destination multibyte buffer (may be NULL).
        src  - Address of the wide character string pointer. The pointer is updated
               during conversion. If dst is NULL, the function returns the number
               of bytes that would result from the conversion.
        nwc  - Maximum number of wide characters to convert.
        len  - Maximum number of bytes to write.
        ps   - Conversion state object (ignored in this implementation).

    RESULT
        Returns the number of bytes written or needed.
        Returns (size_t)-1 on conversion error and sets errno to EILSEQ.

    NOTES
        This implementation assumes stateless UTF-8 encoding and does not
        currently use mbstate_t. It wraps around wcrtomb().

    SEE ALSO
        stdc.library/wcrtomb(), stdc.library/wcsrtombs(), stdc.library/wcstombs(), stdc.library/mbsrtowcs()

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    size_t count = 0;
    size_t res;
    const wchar_t *s;
    char *buf;

    if (!src || !*src)
        return 0;

    buf = AllocVec(StdCBase->__locale_cur->__lc_mb_max, MEMF_ANY);
    s = *src;

    while (nwc > 0 && *s) {
        res = wcrtomb(buf, *s, ps);
        if (res == (size_t)-1) {
            errno = EILSEQ;
            FreeVec(buf);
            return (size_t)-1;
        }

        if (count + res > len) {
            // Not enough room in destination buffer
            break;
        }

        if (dst) {
            for (size_t i = 0; i < res; ++i)
                dst[count + i] = buf[i];
        }

        count += res;
        ++s;
        --nwc;
    }

    *src = (*s == L'\0' || nwc == 0) ? NULL : s;

    FreeVec(buf);
    return count;
}
