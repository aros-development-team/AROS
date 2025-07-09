#ifndef _POSIXC_CTYPE_H_
#define _POSIXC_CTYPE_H_

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Posix header file ctype.h
    Lang: english
*/

#include <aros/features.h>
#include <aros/system.h>
#include <aros/stdc/ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal helper macro */
#define __get_lctype(loc, c) \
    (((unsigned)(c) < (loc)->__lc_tbl_size) ? (loc)->__lc_tbl_clsfy[(unsigned char)(c)] : 0)

#if !defined(POSIXC_NOINLINE) && !defined(POSIXC_NOINLINE_CTYPE)
#define __ctype_make_func_l(__name__, __body__)    \
__header_inline int __name__(int c, locale_t locale)  \
{ __body__ }
#else
#define __ctype_make_func_l(__name__, __body__)    \
int __name__(int c, locale_t locale);
#endif

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L
#include <aros/types/locale_t.h>
#if !defined(POSIXC_NOINLINE) && !defined(POSIXC_NOINLINE_CTYPE)
#include <aros/types/locale_s.h>
#endif

/* Classification functions */
__ctype_make_func_l(isalpha_l,  return (__get_lctype(locale, c) & _ctype_alpha) != 0;)
__ctype_make_func_l(isdigit_l, return (__get_lctype(locale, c) & _ctype_digit) != 0;)
__ctype_make_func_l(isalnum_l, return (__get_lctype(locale, c) & (_ctype_alpha | _ctype_digit)) != 0;)
__ctype_make_func_l(isspace_l, return (__get_lctype(locale, c) & _ctype_space) != 0;)
__ctype_make_func_l(islower_l, return (__get_lctype(locale, c) & _ctype_lower) != 0;)
__ctype_make_func_l(isupper_l, return (__get_lctype(locale, c) & _ctype_upper) != 0;)
__ctype_make_func_l(iscntrl_l, return (__get_lctype(locale, c) & _ctype_cntrl) != 0;)
__ctype_make_func_l(ispunct_l, return (__get_lctype(locale, c) & _ctype_punct) != 0;)
__ctype_make_func_l(isxdigit_l, return (__get_lctype(locale, c) & _ctype_xdigit) != 0;)
__ctype_make_func_l(isprint_l, return (__get_lctype(locale, c) & (_ctype_alpha | _ctype_digit | _ctype_punct | _ctype_space)) != 0;)
__ctype_make_func_l(isgraph_l, return (__get_lctype(locale, c) & (_ctype_alpha | _ctype_digit | _ctype_punct)) != 0;)

/* Conversion functions */
__ctype_make_func_l(toupper_l, if ((unsigned)c < locale->__lc_tbl_size && locale->__lc_tbl_l2u)
        return locale->__lc_tbl_l2u[(unsigned char)c];
    return c;)
__ctype_make_func_l(tolower_l, if ((unsigned)c < locale->__lc_tbl_size && locale->__lc_tbl_u2l) \
        return locale->__lc_tbl_u2l[(unsigned char)c]; \
    return c;)

#endif

#ifdef __cplusplus
}
#endif

#endif /* _POSIXC_CTYPE_H_ */
