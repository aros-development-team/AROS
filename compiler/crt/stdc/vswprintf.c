/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <stdarg.h>
#include <wchar.h>

static wint_t vswprintf_outwc(wchar_t wc, void *data)
{
    struct {
        wchar_t *buf;
        size_t max;
        size_t pos;
    } *ctx = data;

    if (ctx->pos + 1 < ctx->max) {
        ctx->buf[ctx->pos++] = wc;
        return wc;
    }
    return WEOF;
}

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdio.h>
#include <stdarg.h>

        int vswprintf (

/*  SYNOPSIS */
        wchar_t * restrict str,
        size_t   maxsize,
        const wchar_t * restrict format,
        va_list         args)

/*  FUNCTION
        Format a va_list of arguments according to a wide-character
        format string and store the result in the specified buffer.

    INPUTS
        s       - Destination buffer for the formatted wide-character string.
        maxsize - Maximum number of wide characters to store in the buffer,
                  including the null terminator.
        format  - A wide-character printf() format string.
        arg     - A va_list containing the arguments for the format string.

    RESULT
        The number of wide characters written to the buffer, excluding the
        terminating null character. A negative value is returned if an error
        occurs.

    NOTES
        The result is always null-terminated unless maxsize is zero.

    EXAMPLE

    BUGS

    SEE ALSO
        swprintf(), fwprintf(), vfwprintf(), wprintf(), vwprintf()

    INTERNALS
        Calls __vwformat() with a buffer-backed putwc() handler.

******************************************************************************/
{
    struct {
        wchar_t *buf;
        size_t max;
        size_t pos;
    } ctx = { str, maxsize, 0 };

    int result = __vwformat(&ctx, vswprintf_outwc, format, args);

    if (ctx.pos < ctx.max)
        ctx.buf[ctx.pos] = L'\0';
    else if (ctx.max > 0)
        ctx.buf[ctx.max - 1] = L'\0';

    return result;
}
