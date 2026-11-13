/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function getwchar().
*/
#include <libraries/stdcio.h>

#define _STDIO_H_NOMACRO

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

wint_t getwchar(

/*  SYNOPSIS*/
        void)

/*    FUNCTION
        The getwchar() function is the wide-character equivalent of getchar().
        It reads the next wide character from stdin and returns it as a wint_t
        value. If the end-of-file is reached or an error occurs, WEOF is returned.

    INPUTS
        None.

    RESULT
        Returns the next wide character read from stdin as a wint_t.
        Returns WEOF if end-of-file is encountered or an error occurs.

    NOTES
        The behavior of getwchar() depends on the current locale, particularly
        the LC_CTYPE category, which affects the interpretation of multibyte
        sequences as wide characters.
        
        Internally, getwchar() is equivalent to calling fgetwc(stdin).

    EXAMPLE
        wint_t wc;
        while ((wc = getwchar()) != WEOF)
            putwchar(wc);

    BUGS
        If stdin is not a valid input stream or the locale is misconfigured,
        results may be undefined or incorrect.

    SEE ALSO
        fgetwc(), getchar(), putwchar(), getwc()

    INTERNALS
        getwchar() is a thin wrapper around fgetwc(stdin), which performs
        multibyte-to-wide character conversion using the active locale and an
        internal mbstate_t object.

******************************************************************************/
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    return fgetwc(StdCIOBase->_stdin);
}
