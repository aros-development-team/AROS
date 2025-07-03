/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function vswscanf().
*/

#include <wchar.h>

struct swscanf_data {
    const wchar_t *str;
    size_t pos;
};

wint_t get_char(void *userdata) {
    struct swscanf_data *d = (struct swscanf_data *)userdata;
    wchar_t c = d->str[d->pos];
    if (c == L'\0') return WEOF;
    d->pos++;
    return c;
}

int unget_char(wint_t c, void *userdata) {
    struct swscanf_data *d = (struct swscanf_data *)userdata;
    if (c != WEOF && d->pos > 0) d->pos--;
    return c;
}

/*****************************************************************************

    NAME */

        int vswscanf (

/*  SYNOPSIS */
        const wchar_t * restrict ws,
        const wchar_t * restrict format,
        va_list      arg)

/*  FUNCTION
        Reads formatted wide-character input from the given wide-character string,
        parsing according to the given format string, and stores the results
        into the locations described by the argument list.

    INPUTS
        ws     - Read from this wide-character string
        format - How to convert the input into the arguments
        arg    - List of argument pointers to receive results

    RESULT
        The number of converted arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        vwscanf(), vfwscanf()

    INTERNALS
        Calls __vwscanf() using a string-based get/unget character
        mechanism for reading from wide-character strings.

******************************************************************************/
{
    struct swscanf_data data = { ws, 0 };

    return __vwscanf(&data, get_char, unget_char, format, arg);
}
