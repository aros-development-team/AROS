/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function putwc().
*/


#include <wchar.h>
#include <stdio.h>

/****************************************************************************
    NAME */
        wint_t putwc(

/*  SYNOPSIS */
        wchar_t wc,
        FILE *stream)

/*  FUNCTION
        Write a wide character by calling fputwc(). The current conversion
        routine encodes the value as UTF-8 and writes the bytes in order.

    INPUTS
        wc - Unicode scalar value to write. The supported domain excludes
            surrogate values and values above U+10FFFF. See BUGS before writing
            non-ASCII values with the current implementation.
        stream - Non-NULL open output stream. Remains owned by the caller.

    RESULT
        The character value on success, or WEOF on a reported error. A stream
        not open for writing sets errno to EACCES and its error indicator.
        A rejected conversion sets EILSEQ and the error indicator. A failed
        byte write sets the error indicator and maps the DOS error to errno.
        Allocation failure is not safely reported; see BUGS.

    NOTES
        Include <wchar.h>. This wrapper evaluates each argument once and does
        not close the stream. L'\0' writes one zero byte. The converter is
        stateless and ignores the stream's conversion-state object.

        A write error may occur after earlier bytes have been written. There
        is no rollback, so the stream can contain a partial UTF-8 sequence.
        A successful call does not establish that buffered output has reached
        persistent storage. Append mode attempts to seek to the end before
        writing, but the seek result is not checked.

        Call from DOS process context with the C runtime active. Allocation
        and file-handler writes can block; no timeout or cancellation is
        supplied. Coordinate access to both the stream and locale state.
        The wrapper adds no lock spanning conversion and all byte writes.

    BUGS
        fputwc() allocates the current locale's maximum multibyte size, but
        wcrtomb() always emits UTF-8. The default C locale supplies one byte;
        non-ASCII conversion can therefore exceed the requested buffer size.
        This is an implementation defect, not a supported encoding mode.

        The allocation result is unchecked. If allocation fails, wcrtomb()
        treats NULL as a length query; the subsequent write loop dereferences
        that NULL pointer for an otherwise valid character. There is no safe
        ENOMEM return path in this implementation.

        Stream orientation is neither established nor checked. Conversion of
        signed negative wchar_t values is not consistently rejected by the
        underlying converter; pass only values in the stated scalar domain.

    SEE ALSO
        fputwc(), getwc(), wcrtomb(), fwide(), ferror(), fflush()

*****************************************************************************/
{
    return fputwc(wc, stream);
}
