/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function getwc().
*/
#include <wchar.h>
#include <stdio.h>

/****************************************************************************
    NAME */
        wint_t getwc(

/*  SYNOPSIS */
        FILE *stream)

/*  FUNCTION
        Read a wide character by calling fgetwc(). Return a pending character
        from ungetwc() first, without reading bytes. Otherwise decode one
        one-to-four-byte UTF-8 sequence from the stream.

    INPUTS
        stream - Open input stream. The stream remains owned by the caller.

    RESULT
        The decoded or pushed-back value, or WEOF on end-of-file or error.
        NULL stream returns WEOF and sets errno to EINVAL. An invalid lead
        byte, invalid continuation byte or incomplete sequence sets EILSEQ.
        A stream not open for reading normally returns WEOF with EACCES and
        its error indicator set; a pending wide pushback is returned first.

    NOTES
        Include <wchar.h>. This is a function wrapper and evaluates its argument
        once. It does not close the stream or allocate a returned object.

        Decoding is fixed to UTF-8 and does not use the current locale's
        encoding. Bytes consumed before a decoding error are not restored,
        including an invalid continuation byte. A byte I/O error encountered
        after a multibyte lead byte is reported as EILSEQ by the decoder.

        feof() and ferror() reflect the underlying byte read. A decoding error
        does not itself set the stream error indicator; inspect errno when
        WEOF is returned. Successful reads do not clear earlier indicators or
        errno. Returning a pushed-back character does not clear an EOF flag
        left by this implementation of ungetwc().

        Use a valid stream in DOS process context with the C runtime active.
        Reading can block in the underlying file handler. No timeout or
        cancellation mechanism is supplied. Coordinate access to the stream;
        this wrapper adds no locking around a complete multibyte sequence.

    BUGS
        The decoder checks byte prefixes but does not reject overlong forms,
        surrogate code points or values above U+10FFFF. Do not use this call
        as a strict UTF-8 validator. Neither this wrapper nor fgetwc() sets or
        checks stream orientation; the byte-read path does not enforce it.

    SEE ALSO
        fgetwc(), ungetwc(), putwc(), feof(), ferror(), fwide()

*****************************************************************************/
{
    return fgetwc(stream);
}
