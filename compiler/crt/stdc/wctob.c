/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function wctob().
*/

#include <proto/exec.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <wchar.h>

int wctob(

/*  SYNOPSIS */
        wint_t wc)

/*    FUNCTION
        The wctob() function attempts to convert a wide character to a
        single-byte character, according to the current locale's encoding.
        The conversion is only successful if the wide character maps to
        a single byte.

    INPUTS
        wc - A wide character (wint_t) to convert.

    RESULT
        Returns the corresponding single-byte character (as an unsigned char)
        cast to int, or EOF if the character cannot be represented as a
        single-byte sequence.

    NOTES
        This function only succeeds for wide characters that map directly
        to a single byte in the current locale encoding.

    EXAMPLE
        int c = wctob(L'A');   // Might return 'A' if in ASCII-compatible locale.

    BUGS
        Fails for wide characters that require multibyte sequences (e.g.,
        most non-ASCII characters in UTF-8).

    SEE ALSO
        btowc(), wcrtomb(), setlocale()

    INTERNALS
        Uses wcrtomb() with a zero-initialized mbstate_t.
        Only accepts output lengths of 1 for successful conversion.

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    char *buf, ret;
    mbstate_t state = {0};

    if (wc == WEOF)
        return EOF;

    buf = AllocVec(StdCBase->__locale_cur->__lc_mb_max, MEMF_ANY);
    size_t len = wcrtomb(buf, wc, &state);
	ret = buf[0];
	FreeVec(buf);
    if (len == 1)
	{
        return (unsigned char)ret;
	}

    return EOF;
}
