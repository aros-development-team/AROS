#ifndef _STDC_CTYPE_H_
#define _STDC_CTYPE_H_

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file ctype.h
    Lang: english
*/

#include <aros/system.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _ctype_upper    0x0001  /* Uppercase letter (Unicode category 'Lu') */
#define _ctype_lower    0x0002  /* Lowercase letter (Unicode category 'Ll') */
#define _ctype_alpha    0x0004  /* Alphabetic letter (all Unicode letter categories 'L*') */
#define _ctype_digit    0x0008  /* Decimal digit number (Unicode category 'Nd') */
#define _ctype_xdigit   0x0010  /* Hexadecimal digit: 0-9, a-f, A-F (ASCII only) */
#define _ctype_space    0x0020  /* Whitespace characters (Unicode White_Space property: space, tab, newline, etc.) */
#define _ctype_print    0x0040  /* Printable characters including space (all visible Unicode chars plus space) */
#define _ctype_graph    0x0080  /* Printable characters excluding space */
#define _ctype_blank    0x0100  /* Horizontal whitespace: space and tab (ASCII ' ' and '\\t') */
#define _ctype_cntrl    0x0200  /* Control characters (Unicode category 'Cc', e.g. 0-31, 127) */
#define _ctype_punct    0x0400  /* Punctuation characters (Unicode categories 'Pc', 'Pd', 'Ps', 'Pe', 'Pi', 'Pf', 'Po') */
#define _ctype_alnum    (_ctype_alpha | _ctype_digit)  /* Alphabetic or digit (letters and decimal digits) */

extern const unsigned short int * const * const __ctype_b_ptr;
extern const unsigned char     * const * const __ctype_toupper_ptr;
extern const unsigned char     * const * const __ctype_tolower_ptr;

#define _istype(c, type) \
    ((*__ctype_b_ptr)[(unsigned char)(c)] & (unsigned short)(type))

#if !defined(STDC_NOINLINE) && !defined(STDC_NOINLINE_CTYPE)
#define __ctype_make_func(__name__, __body__)    \
__header_inline int __name__(int c)  \
{ return __body__; }
#else
#define __ctype_make_func(__name__, __body__)    \
int __name__(int c);
#endif

/* ISO C Standard Functions */
__ctype_make_func(isupper,  _istype(c, _ctype_upper))
__ctype_make_func(islower,  _istype(c, _ctype_lower))
__ctype_make_func(isalpha,  _istype(c, _ctype_alpha))
__ctype_make_func(isdigit,  _istype(c, _ctype_digit))
__ctype_make_func(isxdigit, _istype(c, _ctype_xdigit))
__ctype_make_func(isspace,  _istype(c, _ctype_space))
__ctype_make_func(isprint,  _istype(c, _ctype_print))
__ctype_make_func(isgraph,  _istype(c, _ctype_graph))
__ctype_make_func(iscntrl,  _istype(c, _ctype_cntrl))
__ctype_make_func(ispunct,  _istype(c, _ctype_punct))
__ctype_make_func(isalnum,  _istype(c, _ctype_alnum))

__ctype_make_func(toupper,  (*__ctype_toupper_ptr)[(unsigned char)(c)])
__ctype_make_func(tolower,  (*__ctype_tolower_ptr)[(unsigned char)(c)])

/* POSIX and GNU Extensions */
#if defined(_POSIX_C_SOURCE) || defined(_GNU_SOURCE)

# ifndef isblank
__ctype_make_func(isblank,  _istype(c, _ctype_blank))
# endif

# ifndef isascii
__ctype_make_func(isascii,  ((unsigned char)(c) <= 0x7f))
# endif

# ifndef toascii
__ctype_make_func(toascii,  ((c) & 0x7f))
# endif

#endif /* POSIX/GNU/AROS */

#define _toupper(c) toupper(c)
#define _tolower(c) tolower(c)

#ifdef __cplusplus
}
#endif

#endif /* _STDC_CTYPE_H_ */
