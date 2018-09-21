/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Function to format a string like printf().
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifndef AROS_NO_LIMITS_H
#	include <limits.h>
#else
#	define ULONG_MAX   4294967295UL
#endif
#include <math.h>
#include <float.h>

#include <ctype.h>

#ifndef STDC_STATIC
#define FULL_SPECIFIERS
#endif

const unsigned char *const __stdc_char_decimalpoint = ".";

/* support macros for FMTPRINTF */
#define FMTPRINTF_COUT(c)  do                           \
                { if((*outc)((unsigned char)(c),data)==EOF)   \
                    return outcount;	     \
                  outcount++;		     \
                }while(0)

#define FMTPRINTF_STRLEN(str) strlen(str)

#define FMTPRINTF_DECIMALPOINT	__stdc_char_decimalpoint

#include "fmtprintf_pre.c"

/*****************************************************************************

    NAME */

        int __vcformat (

/*  SYNOPSIS */
        void	   * data,
        int	  (* outc)(int, void *),
        const char * format,
        va_list      args)

/*  FUNCTION
        Format a list of arguments and call a function for each char
        to print.

    INPUTS
        data - This is passed to the user callback outc as its second argument.
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
