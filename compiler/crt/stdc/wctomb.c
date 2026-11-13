/*
    Copyright (C) 2007-2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function wctomb().
*/
#include <stddef.h>
#include <wchar.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int wctomb(

/*  SYNOPSIS */
        char *s,
        wchar_t wc)

/*  FUNCTION
        Converts a single wide character to its multibyte equivalent and stores
        it in the buffer pointed to by 's'.

    INPUTS
        s  - Pointer to the output buffer to store the resulting multibyte character.
             If NULL, the function is called to reset the conversion state.

        wc - The wide character to convert.

    RESULT
        Returns the number of bytes written into 's' if successful.
        If 'wc' is the null character (L'\0'), a single null byte is written and 1 is returned.
        If the wide character cannot be represented as a valid multibyte sequence,
        -1 is returned and errno is set to EILSEQ.

        If 's' is NULL, the function does not perform a conversion. Instead, it returns 0
        if the encoding is stateless (as with UTF-8), or a non-zero value if the encoding
        is state-dependent. For UTF-8, 0 is always returned in this case.

    NOTES
        This function is a wrapper around wcrtomb(), using a static internal mbstate_t object
        to preserve compatibility with C standard behavior. However, since UTF-8 is a stateless
        encoding, the static state is effectively unused.

        The function does not support stateful encodings and behaves according to the "C" or
        UTF-8 locale model only. Behavior is undefined if multibyte encodings require internal state.

    EXAMPLE
        wchar_t wc = L'O';
        char buf[MB_CUR_MAX];
        int len = wctomb(buf, wc);
        if (len > 0) {
            // buf now contains the multibyte representation of 'wc'
        }

    BUGS
        No locale-specific behavior is supported.
        No stateful encoding support (e.g., Shift-JIS or ISO 2022).

    SEE ALSO
        wcrtomb(), mbtowc(), mbrtowc(), wcstombs(), mbstowcs()

    INTERNALS
        Internally initializes a static mbstate_t structure and forwards the call to wcrtomb().
        For UTF-8, state is unused and conversions are always stateless.

******************************************************************************/
{
    // POSIX requires wctomb to use a static internal state
    static mbstate_t ps;

    // If s == NULL, reset state — not used in our case, just return 0
    if (!s)
        return 0;

    size_t len = wcrtomb(s, wc, &ps);
    if (len == (size_t)-1)
        return -1;

    return (int)len;
}
