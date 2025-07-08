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
        void       * data,
        wint_t     (* outwc)(wchar_t, void *),
        const wchar_t * format,
        va_list      args)

/*  FUNCTION
        Format a list of arguments and call a function for each wide char
        to print.

    INPUTS
        data - This is passed to the user callback outwc as its second argument.
        outc - Call this function for every character that should be
                emitted. The function should return EOF on error and
                > 0 otherwise.
        format - A printf() format string.
        args - A list of arguments for the format string.

    RESULT
        The number of characters written.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
#include "fmtprintf.c"
  return outcount;
}
