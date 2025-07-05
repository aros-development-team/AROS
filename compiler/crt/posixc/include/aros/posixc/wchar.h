#ifndef _POSIXC_WCHAR_H_
#define _POSIXC_WCHAR_H_

/*
 *  Copyright © 2025 The AROS Developmemt Team. All rights reserved.
 *  $Id$
 *
 *  POSIX.1-2008 header file wchar.h
 */

/* C99 */
#include <aros/stdc/wchar.h>

__BEGIN_DECLS

size_t mbsnrtowcs(wchar_t *restrict, const char **restrict, size_t, size_t, mbstate_t *restrict);
size_t wcsnrtombs(char *restrict, const wchar_t **restrict, size_t, size_t, mbstate_t *restrict);

__END_DECLS

#endif /* _POSIXC_WCHAR_H_ */
