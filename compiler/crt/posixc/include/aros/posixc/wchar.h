#ifndef _POSIXC_WCHAR_H_
#define _POSIXC_WCHAR_H_

/*
 *  Copyright © 2025 The AROS Developmemt Team. All rights reserved.
 *  $Id$
 *
 *  POSIX.1-2008 header file wchar.h
 */

#include <aros/posixc/locale.h>
/* C99 */
#include <aros/stdc/wchar.h>

__BEGIN_DECLS

#if _POSIX_C_SOURCE >= 200809L

size_t mbsnrtowcs(wchar_t *restrict, const char **restrict, size_t, size_t, mbstate_t *restrict);
size_t wcsnrtombs(char *restrict, const wchar_t **restrict, size_t, size_t, mbstate_t *restrict);
size_t wcsxfrm_l(wchar_t *dest, const wchar_t *src, size_t n, locale_t loc);
int wcscoll_l(const wchar_t *ws1, const wchar_t *ws2, locale_t loc);

#endif /* _POSIX_C_SOURCE >= 200809L */

__END_DECLS

#endif /* _POSIXC_WCHAR_H_ */
