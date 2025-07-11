/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcstoimax().
*/

#include <stdlib.h>
#include <wchar.h>

/*****************************************************************************

    NAME */
#include <inttypes.h>

        intmax_t wcstoimax(

/*  SYNOPSIS */
        const wchar_t * restrict nptr,
        wchar_t       ** restrict endptr,
        int                     base)

/*  FUNCTION
        Converts a wide-character string of digits into an integer of type
        intmax_t, according to the specified base. This function behaves like
        wcstol() but returns an intmax_t result.

    INPUTS
        nptr    - Pointer to the wide-character string to convert.
        endptr  - If non-NULL, stores the address of the first character
                  after the number.
        base    - The base for conversion (e.g., 10 for decimal, 16 for hex).

    RESULT
        Returns the converted value as an intmax_t. If no valid conversion could
        be performed, returns 0 and stores nptr in *endptr (if endptr is non-NULL).

    NOTES
        Does not set errno on overflow or underflow, contrary to the C99 standard.
        Behavior is otherwise compatible with the standard library definition.

    EXAMPLE
        wchar_t *str = L"12345";
        wchar_t *end;
        intmax_t value = wcstoimax(str, &end, 10);

    BUGS
        errno is not set on conversion errors or range overflows.

    SEE ALSO
        wcstol(), wcstoll(), strtoimax(), strtol()

    INTERNALS
        Internally calls wcstoll() or equivalent and casts the result
        to intmax_t.

******************************************************************************/
{
    /* TODO: Implement errno handling in wcstoimax() */
#if defined(AROS_HAVE_LONG_LONG)
    return (intmax_t) wcstoll(nptr, endptr, base);
#else
    return (intmax_t) wcstol(nptr, endptr, base);
#endif
}
