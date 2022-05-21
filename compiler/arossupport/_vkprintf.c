/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc: Formats a message and makes sure the user will see it.
*/

#include <aros/config.h>
#include <aros/arossupportbase.h>
#include <stdarg.h>

#include <aros/system.h>
#include <proto/exec.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/arossupport.h>
#undef vkprintf
#include <exec/execbase.h>

/*
  The floating point math code pulls in symbols
  that cannot be used in the amiga rom
  */
#if !defined(__m68k__)
#define FULL_SPECIFIERS
#endif

#if defined(FULL_SPECIFIERS)
#include <float.h>
#endif

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
#define isnan(x)                1
#define isinf(x)                1
#define log10(x)                1
#define log10l(x)               1
#define pow(x,y)        1
#define powl(x,y)               1

/* string.h ... */
static inline int _vkprintf_strlen(const char *c)
{
    int i = 0;
    while (*(c++)) i++;
    return i;
}

/* limits.h ... */
#define ULONG_MAX       4294967295UL

/* support macros for FMTPRINTF */
#define FMTPRINTF_COUT(c)       RawPutChar(c)
#define FMTPRINTF_STRLEN(str)   _vkprintf_strlen(str)

#if defined(FULL_SPECIFIERS)
#define FMTPRINTF_DECIMALPOINT  __arossupport_char_decimalpoint
#endif

#include "fmtprintf_pre.c"

#include <proto/arossupport.h>

/******************************************************************************/
int _vkprintf (const char * format, va_list args)
{
#if defined(FULL_SPECIFIERS)
        const unsigned char *const __arossupport_char_decimalpoint = ".";
#endif

#include "fmtprintf.c"

  return outcount;
} /* vkprintf */
