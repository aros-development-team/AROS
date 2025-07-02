/*
    Copyright (C) 2007-2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function mbstowcs().
*/
#include <stddef.h>
#include <wchar.h>
#include <errno.h>

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        size_t mbstowcs(

/*  SYNOPSIS */
        wchar_t * restrict dest,
        const char * restrict src,
        size_t n)

/*  FUNCTION
        Converts a null-terminated multibyte character string (char*) into its
        corresponding wide character string (wchar_t*), using the current locale's encoding.

        At most 'n' wide characters are written into 'dest'. Conversion stops
        on the first invalid multibyte sequence, or when a null byte is encountered.

    INPUTS
        dest - Pointer to the output wide character buffer. If NULL, the function
               returns the number of wide characters that would result from the conversion,
               excluding the terminating null wide character.

        src  - Pointer to the null-terminated multibyte string to convert.

        n    - Maximum number of wide characters to write into 'dest'.

    RESULT
        Returns the number of wide characters converted (excluding the terminating L'\0').
        Returns (size_t)-1 if an invalid multibyte sequence is encountered during conversion,
        and sets errno to EILSEQ.

        If 'dest' is NULL, the function returns the number of wide characters that would
        be generated from the conversion, not counting the final null wide character.

    NOTES
        stdc.library currently only implements "C" or UTF-8-compatible locales.
        The encoding is stateless, and no locale-dependent shift state is used.
        Conversion uses mbtowc() for each multibyte sequence encountered in 'src'.

    EXAMPLE
        const char *utf8 = "Hello ??!";
        wchar_t wbuf[64];
        size_t count = mbstowcs(wbuf, utf8, 64);
        if (count != (size_t)-1) {
            // wbuf now contains wide character equivalents
        }

    BUGS
        Does not support locale-specific encodings or stateful encodings like ISO 2022 or Shift-JIS.
        Buffer overflow may occur if 'n' is smaller than the number of resulting wide characters.

    SEE ALSO
        mbtowc(), wcstombs(), wctomb(), mblen()

    INTERNALS
        Iterates through 'src' one multibyte character at a time using mbtowc(),
        storing each wide character into 'dest' (if not NULL).
        Stops at null terminator or when 'n' wide characters have been written.

******************************************************************************/
 {
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    size_t count = 0;
    size_t len;
    wchar_t wc;
    mbstate_t ps = {0}; // Not used in our implementation

    while (*src && count < n) {
        len = mbtowc(&wc, src, StdCBase->__locale_cur->__lc_mb_max);
        if (len == (size_t)-1)
            return (size_t)-1;

        if (dest)
            dest[count] = wc;

        src += len;
        count++;
    }

    return count;
}
