#ifndef _WCTYPE_H_
#define _WCTYPE_H_
/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Standard C Library: Wide character classification and mapping.
    Introduced in ISO 8879:1999 ("C99").
*/

#include <sys/_types.h>

#define __need_wchar_t
#define __need_wint_t
#include <stddef.h>

#ifndef WEOF
#define	WEOF	((wint_t)-1)
#endif

typedef int wctype_t;
typedef int wctrans_t;

int iswalnum(wint_t wc);
int iswalpha(wint_t wc);
int iswcntrl(wint_t wc);
int iswdigit(wint_t wc);
int iswgraph(wint_t wc);
int iswlower(wint_t wc);
int iswprint(wint_t wc);
int iswpunct(wint_t wc);
int iswspace(wint_t wc);
int iswupper(wint_t wc);
int iswxdigit(wint_t wc);

int iswctype(wint_t wc, wctype_t desc);
wctype_t wctype(const char *property);

wint_t towlower(wint_t wc);
wint_t towupper(wint_t wc);

wint_t towctrans(wint_t wc, wctrans_t desc);
wctrans_t wctrans(const char *property);

#endif /* _WCTYPE_H_ */
