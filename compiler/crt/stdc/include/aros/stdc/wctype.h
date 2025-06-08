#ifndef _STDC_WCTYPE_H_
#define _STDC_WCTYPE_H_
/*
    Copyright © 2020-2025, The AROS Development Team. All rights reserved.
    $Id$

    Standard C Library: Extended multibyte and wide-character classification and mapping utilities.
*/

#include <aros/stdc/stdcnotimpl.h>
#include <aros/system.h>

#include <aros/types/wctrans_t.h>
#include <aros/types/wctype_t.h>
#include <aros/types/wint_t.h>

#include <stdarg.h>
#include <stdint.h>

__BEGIN_DECLS

STDC_WCTYPE_NOTIMPL(
int iswalnum(wint_t);
int iswalpha(wint_t);
int iswblank(wint_t);
int iswcntrl(wint_t);
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);
int iswctype(wint_t, wctype_t);
wint_t towctrans(wint_t, wctrans_t);
wint_t towlower(wint_t);
wint_t towupper(wint_t);
wctrans_t wctrans(const char *);
wctype_t wctype(const char *);
)

__END_DECLS

#endif /* _STDC_WCTYPE_H_ */
