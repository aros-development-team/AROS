/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function fgetwc().
*/

#include <wchar.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        wint_t fgetwc (

/*  SYNOPSIS */
        FILE * stream)

/*  FUNCTION
        Read the next wide character from the specified stream.
        Supports UTF-8 decoding if the stream is byte-oriented.
        Returns a pushed-back wide character if present via ungetwc.

    INPUTS
        stream - Pointer to the input stream.

    RESULT
        Returns the next wide character as a wint_t.
        Returns WEOF on end-of-file or error.

    NOTES
        The function handles UTF-8 sequences up to 4 bytes.
        If a wide character has been pushed back with ungetwc, it is returned first.

    EXAMPLE

    BUGS

    SEE ALSO
        ungetwc(), fputwc(), fwide()

    INTERNALS

******************************************************************************/
{
    if (!stream) {
        errno = EINVAL;      // Invalid stream pointer
        return WEOF;
    }

    // Return pushed-back wide char if present
    if (stream->flags & __STDCIO_STDIO_UNGETWC) {
        wint_t wc = stream->unget_wc;
        stream->flags &= ~__STDCIO_STDIO_UNGETWC;  // Clear flag
        return wc;
    }

    unsigned char buf[4];
    int bytes_needed, i;
    int c = fgetc(stream);
    if (c == EOF) return WEOF;  // End of file or error

    buf[0] = (unsigned char)c;

    // Determine UTF-8 sequence length from lead byte
    if ((buf[0] & 0x80) == 0)           // ASCII, single byte
        return (wint_t)buf[0];
    else if ((buf[0] & 0xE0) == 0xC0)  // 2-byte sequence
        bytes_needed = 2;
    else if ((buf[0] & 0xF0) == 0xE0)  // 3-byte sequence
        bytes_needed = 3;
    else if ((buf[0] & 0xF8) == 0xF0)  // 4-byte sequence
        bytes_needed = 4;
    else {
        errno = EILSEQ;  // Invalid lead byte
        return WEOF;
    }

    // Read continuation bytes and validate them
    for (i = 1; i < bytes_needed; ++i) {
        c = fgetc(stream);
        if (c == EOF || (c & 0xC0) != 0x80) { // Must start with '10'
            errno = EILSEQ; // Invalid UTF-8 continuation byte or premature EOF
            return WEOF;
        }
        buf[i] = (unsigned char)c;
    }

    // Decode UTF-8 bytes into a Unicode codepoint (wint_t)
    wint_t wc = 0;
    if (bytes_needed == 2)
        wc = ((buf[0] & 0x1F) << 6) | (buf[1] & 0x3F);
    else if (bytes_needed == 3)
        wc = ((buf[0] & 0x0F) << 12) | ((buf[1] & 0x3F) << 6) | (buf[2] & 0x3F);
    else if (bytes_needed == 4)
        wc = ((buf[0] & 0x07) << 18) | ((buf[1] & 0x3F) << 12) | ((buf[2] & 0x3F) << 6) | (buf[3] & 0x3F);

    return wc;
}
