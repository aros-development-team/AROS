/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Formats a message and makes sure the user will see it.
    Lang: english
*/

#include <aros/config.h>
#include <aros/arossupportbase.h>
#include <stdarg.h>

#include <aros/system.h>
#include <proto/exec.h>
#include <proto/arossupport.h>
#undef kprintf
#undef vkprintf
#include <exec/execbase.h>

#if defined(__AROSEXEC_SMP__)
#include <aros/atomic.h>
#include <asm/cpu.h>
extern volatile ULONG   safedebug;
#endif

#include <float.h>

/* provide inline versions of clib functions
   used in FMTPRINTF */
   
/* ctype.h ... */
static inline int isdigit(int c)
{
	return '0' <= c && c <= '9';
}

static inline unsigned char tolower(unsigned char c)
{
	if ((int)c >= (int)'A')
		c -= 'A'-'a';
	return c;
}

#define isprint(x)      (((x) >= ' ' && (x) <= 128) || (x) >= 160)

/* math.h ... */
#define isnan(x)		1
#define isinf(x)		1
#define log10(x)		1
#define log10l(x)		1
#define pow(x,y)      	1
#define powl(x,y)		1

/* string.h ... */
static inline int kprintf_strlen(const char *c)
{
    int i = 0;
    while (*(c++)) i++;
    return i;
}

/* limits.h ... */
#define ULONG_MAX   	4294967295UL

static const unsigned char *const __arossupport_char_decimalpoint = ".";

#define FULL_SPECIFIERS

/* support macros for FMTPRINTF */
#define FMTPRINTF_COUT(c)       RawPutChar(c)
#define FMTPRINTF_STRLEN(str)   kprintf_strlen(str)

#define FMTPRINTF_DECIMALPOINT	__arossupport_char_decimalpoint

#include "fmtprintf_pre.c"

/*****************************************************************************

    NAME */
        #include <proto/arossupport.h>

        int kprintf (

/*  SYNOPSIS */
        const char * fmt,
        ...)

/*  FUNCTION
        Formats fmt with the specified arguments like printf() (and *not*
        like RawDoFmt()) and uses a secure way to deliver the message to
        the user; ie. the user *will* see this message no matter what.

    INPUTS
        fmt - printf()-style format string

    RESULT
        The number of characters output.

    NOTES
        This function is not part of a library and may thus be called
        any time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        24-12-95    digulla created

******************************************************************************/
{
    va_list	 ap;
    int		 result;

    va_start (ap, fmt);
    result = vkprintf (fmt, ap);
    va_end (ap);

    return result;
} /* kprintf */

/******************************************************************************/
int vkprintf (const char * format, va_list args)
{
#if defined(__AROSEXEC_SMP__)
    if (safedebug & 1)
    {
        while (bit_test_and_set_long((ULONG*)&safedebug, 1)) { asm volatile("pause"); };
    }
#endif
#include "fmtprintf.c"
#if defined(__AROSEXEC_SMP__)
    if (safedebug & 1)
    {
        __AROS_ATOMIC_AND_L(safedebug, ~(1 << 1));
    }
#endif
  return outcount;
} /* vkprintf */
