/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Function to format a wide string like printf().
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef AROS_NO_LIMITS_H
#       include <limits.h>
#else
#       define ULONG_MAX   4294967295UL
#endif
#include <math.h>
#include <float.h>

#include <wchar.h>
#include <wctype.h>

#ifndef STDC_STATIC
#define FULL_SPECIFIERS
#endif

const wchar_t *const __stdc_wchar_decimalpoint = L".";

/* support macros for FMTPRINTF */
#define FMTPRINTF_TYPE          wchar_t
#define FMTPRINTF_UTYPE         wchar_t
#define FMTPRINTF_STR(str)      L ## str
#define FMTPRINTF_STRLEN(str)   wcslen(str)
#define FMTPRINTF_DECIMALPOINT  __stdc_wchar_decimalpoint
#define FMTPRINTF_ISDIGIT(c)    iswdigit(c)
#define FMTPRINTF_TOLOWER(c)    towlower(c)
#define FMTPRINTF_OUT(c,ctx)  do                            \
                { if((*outwc)((wchar_t)(c),data)==WEOF) \
                    return outcount;                        \
                  outcount++;                               \
                }while(0)

#include "fmtprintf_pre.c"

/*****************************************************************************

    NAME */

        int __vwformat (

/*  SYNOPSIS */
        void          * data,
        wint_t       (* outwc)(wchar_t, void *),
        const wchar_t * format,
        va_list         args)

/*  FUNCTION
        Formats a list of arguments according to the provided wide-character
        format string and emits each resulting wide character through a
        callback function.

    INPUTS
        data   - A user-supplied pointer passed as the second argument to
                 the outwc() callback function.
        outwc  - A callback function invoked for each wide character to output.
                 The callback receives the character and the data pointer.
                 It should return EOF on error or a positive value on success.
        format - A wide-character printf() format string containing
                 formatting directives.
        args   - A va_list containing the arguments for the format string.

    RESULT
        The number of wide characters successfully written, or a negative
        value on error.

    NOTES
        This is a generic backend used to implement wide-character printf
        functions. It handles parsing of printf-style format specifiers
        and passes formatted output one character at a time to the caller's
        output function.

    EXAMPLE

    BUGS
        Some format specifiers may not yet be implemented or fully support
        wide-character variations.

    SEE ALSO
        swprintf(), vswprintf(), fwprintf(), vfwprintf(), wprintf(), vwprintf()

    INTERNALS
        The core printf formatting logic is shared with the narrow-character
        version where possible but adapted for wide characters. Supports
        numeric formatting, string output, and most standard printf specifiers.

******************************************************************************/
{
#include "fmtprintf.c"
  return outcount;
}
