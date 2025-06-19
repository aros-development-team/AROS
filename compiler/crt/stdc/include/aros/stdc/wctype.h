#ifndef _STDC_WCTYPE_H_
#define _STDC_WCTYPE_H_
/*
    Copyright © 2020-2025, The AROS Development Team. All rights reserved.
    $Id$

    Standard C Library: Extended multibyte and wide-character classification and mapping utilities.
*/

#include <aros/system.h>

#include <aros/types/wctrans_t.h>
#include <aros/types/wctype_t.h>
#include <aros/types/wint_t.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WCTRANS_NONE       0
#define WCTRANS_TOLOWER    1
#define WCTRANS_TOUPPER    2

/* Classification bitmask flags                     */
/* Wide character classification flags for wctype_t */
#define _WCTYPE_UPPER    0x0001  /* Uppercase letter */
#define _WCTYPE_LOWER    0x0002  /* Lowercase letter */
#define _WCTYPE_ALPHA    (_WCTYPE_UPPER | _WCTYPE_LOWER)

#define _WCTYPE_DIGIT    0x0004  /* Decimal digit */
#define _WCTYPE_ALNUM    (_WCTYPE_ALPHA | _WCTYPE_DIGIT)

#define _WCTYPE_SPACE    0x0008  /* Whitespace (space, tab, newline, etc.) */
#define _WCTYPE_PUNCT    0x0010  /* Punctuation */
#define _WCTYPE_CNTRL    0x0020  /* Control character */
#define _WCTYPE_BLANK    0x0040  /* Space or tab */
#define _WCTYPE_XDIGIT   0x0080  /* Hexadecimal digit */

#define _WCTYPE_GRAPH    0x0100  /* Visible characters (not space) */
#define _WCTYPE_PRINT    0x0200  /* Printable characters (including space) */

/* Future-proofing: Reserved flags */
#define _WCTYPE_RESERVED1 0x0400
#define _WCTYPE_RESERVED2 0x0800

#define _istype(c, type) \
    ((*__wctype_b_ptr)[(unsigned char)(c)] & (unsigned short)(type))

#if !defined(STDC_NOINLINE) && !defined(STDC_NOINLINE_WCTYPE)
#define __wctype_make_func(__name__, __body__)    \
__header_inline int __name__(wint_t wc)  \
{ return __body__; }
#else
#define __wctype_make_func(__name__, __body__)    \
__BEGIN_DECLS                          \
int __name__(wint_t wc); \
__END_DECLS
#endif

typedef struct __wctrans *wctrans_t;  // Opaque handle

__BEGIN_DECLS

int iswctype(wint_t wc, wctype_t desc);

__END_DECLS

#if !defined(STDC_NOINLINE) && !defined(STDC_NOINLINE_WCTYPE)
#if !defined(STDC_NOINLINE_WCTYPE_STRCMP)
__header_inline int wctype_strcmp(const char *s1, const char *s2) {
    unsigned char c1, c2;
    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 != c2)
            return c1 - c2;
    } while (c1 != '\0');
    return 0;
}
#else
#define wctype_strcmp strcmp
#endif

__header_inline wctype_t wctype(const char *prop)
{
    if (!prop) return 0;
    if (!wctype_strcmp(prop, "alnum"))   return _WCTYPE_ALNUM;
    if (!wctype_strcmp(prop, "alpha"))   return _WCTYPE_ALPHA;
    if (!wctype_strcmp(prop, "cntrl"))   return _WCTYPE_CNTRL;
    if (!wctype_strcmp(prop, "digit"))   return _WCTYPE_DIGIT;
    if (!wctype_strcmp(prop, "graph"))   return _WCTYPE_GRAPH;
    if (!wctype_strcmp(prop, "lower"))   return _WCTYPE_LOWER;
    if (!wctype_strcmp(prop, "print"))   return _WCTYPE_PRINT;
    if (!wctype_strcmp(prop, "punct"))   return _WCTYPE_PUNCT;
    if (!wctype_strcmp(prop, "space"))   return _WCTYPE_SPACE;
    if (!wctype_strcmp(prop, "upper"))   return _WCTYPE_UPPER;
    if (!wctype_strcmp(prop, "xdigit"))  return _WCTYPE_XDIGIT;
    return 0;
}

#else
__BEGIN_DECLS

wctype_t wctype(const char *prop);

__END_DECLS
#endif

__BEGIN_DECLS
wctrans_t wctrans(const char *prop);
wint_t towctrans(wint_t wc, wctrans_t desc);
wint_t towlower(wint_t wc);
wint_t towupper(wint_t wc);
__END_DECLS

__wctype_make_func(iswalnum, iswctype(wc, _WCTYPE_ALNUM))
__wctype_make_func(iswalpha, iswctype(wc, _WCTYPE_ALPHA))
__wctype_make_func(iswcntrl, iswctype(wc, _WCTYPE_CNTRL))
__wctype_make_func(iswdigit, iswctype(wc, _WCTYPE_DIGIT))
__wctype_make_func(iswgraph, iswctype(wc, _WCTYPE_GRAPH))
__wctype_make_func(iswlower, iswctype(wc, _WCTYPE_LOWER))
__wctype_make_func(iswprint, iswctype(wc, _WCTYPE_PRINT))
__wctype_make_func(iswpunct, iswctype(wc, _WCTYPE_PUNCT))
__wctype_make_func(iswspace, iswctype(wc, _WCTYPE_SPACE))
__wctype_make_func(iswupper, iswctype(wc, _WCTYPE_UPPER))
__wctype_make_func(iswxdigit, iswctype(wc, _WCTYPE_XDIGIT))
__wctype_make_func(iswblank, iswctype(wc, _WCTYPE_BLANK))

#ifdef __cplusplus
}
#endif

#endif /* _STDC_WCTYPE_H_ */
