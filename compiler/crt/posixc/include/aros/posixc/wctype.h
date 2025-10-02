#ifndef _POSIXC_WCTYPE_H_
#define _POSIXC_WCTYPE_H_
/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

   POSIX.1-2008 header file wctype.h
 */

#include <aros/posixc/locale.h>
/* C99 */
#include <aros/stdc/wctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _POSIX_C_SOURCE >= 200809L

/* Locale-specific wide character classification */
int iswalnum_l(wint_t wc, locale_t locale);
int iswalpha_l(wint_t wc, locale_t locale);
int iswblank_l(wint_t wc, locale_t locale);
int iswcntrl_l(wint_t wc, locale_t locale);
int iswdigit_l(wint_t wc, locale_t locale);
int iswgraph_l(wint_t wc, locale_t locale);
int iswlower_l(wint_t wc, locale_t locale);
int iswprint_l(wint_t wc, locale_t locale);
int iswpunct_l(wint_t wc, locale_t locale);
int iswspace_l(wint_t wc, locale_t locale);
int iswupper_l(wint_t wc, locale_t locale);
int iswxdigit_l(wint_t wc, locale_t locale);
int iswctype_l(wint_t wc, wctype_t desc, locale_t locale);

/* Locale-specific wide character property lookup */
wctype_t wctype_l(const char *property, locale_t locale);

/* Locale-specific wide character conversion */
wint_t towlower_l(wint_t wc, locale_t locale);
wint_t towupper_l(wint_t wc, locale_t locale);
wint_t towctrans_l(wint_t wc, wctrans_t desc, locale_t locale);
wctrans_t wctrans_l(const char *property, locale_t locale);

#endif /* _POSIX_C_SOURCE >= 200809L */

#ifdef __cplusplus
}
#endif

#endif /* _POSIXC_WCTYPE_H_ */
