/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function putwchar().
*/
#include <libraries/stdcio.h>

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

wint_t putwchar(

/*  SYNOPSIS */
        wchar_t wc)

/*    FUNCTION
        The putwchar() function is the wide-character equivalent of putchar().
        It writes the wide character wc to the standard output stream using the
        current locale to convert it to its multibyte representation.

    INPUTS
        wc - The wide character to be written to stdout.

    RESULT
        Returns the wide character written as a wint_t value on success.
        Returns WEOF if an error occurs.

    NOTES
        The output is affected by the current locale, particularly LC_CTYPE,
        which governs the multibyte encoding used during output.
        
        Internally, putwchar() is equivalent to fputwc(wc, stdout).

    EXAMPLE
        putwchar(L'?');

    BUGS
        If stdout is not a valid output stream or the locale does not support
        the character, the function may fail or produce unexpected results.

    SEE ALSO
        fputwc(), putchar(), getwchar(), putwc()

    INTERNALS
        This function delegates to fputwc() with stdout. The wide character
        is converted to a multibyte sequence and buffered or written directly
        depending on stdout's mode.

******************************************************************************/
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    return fputwc(wc, StdCIOBase->_stdout);
}
