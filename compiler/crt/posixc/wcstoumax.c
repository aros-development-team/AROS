/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcstoumax().
*/

#include <stdlib.h>
#include <wchar.h>

/*****************************************************************************

    NAME */
#include <inttypes.h>

        uintmax_t wcstoumax(

/*  SYNOPSIS */
        const wchar_t *nptr,
        wchar_t       **endptr,
        int            base)

/*  FUNCTION
        Converts a wide-character string of digits into an unsigned integer
        according to the given base. This function is similar to wcstoul(),
        but returns a value of type uintmax_t.

    INPUTS
        nptr    - Pointer to the wide-character string to be converted.
        endptr  - If non-NULL, the address of the first character after the
                  number is stored here.
        base    - The numeric base for the conversion (e.g., 10 for decimal,
                  16 for hexadecimal).

    RESULT
        The converted numeric value as a uintmax_t. If no valid digits are found,
        0 is returned, and if endptr is non-NULL, it is set to nptr.

    NOTES
        The function does not set errno on overflow or underflow, contrary to
        the requirements of the C99 standard.

    EXAMPLE
        wchar_t *str = L"FF";
        wchar_t *end;
        uintmax_t value = wcstoumax(str, &end, 16);

    BUGS
        errno is not set on range errors as required by the C99 standard.

    SEE ALSO
        wcstoul(), wcstoull(), strtoumax(), strtoul()

    INTERNALS
        Internally calls wcstoull() or equivalent, returning the result
        as uintmax_t.

******************************************************************************/
{
    /* TODO: Implement errno handling in wcstoumax() */
#if 0 && defined(AROS_HAVE_LONG_LONG)
    return (uintmax_t) wcstoull(nptr, endptr, base);
#else
    return (uintmax_t) wcstoul(nptr, endptr, base);
#endif
}
