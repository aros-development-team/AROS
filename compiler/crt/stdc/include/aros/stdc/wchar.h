#ifndef _STDC_WCHAR_H_
#define _STDC_WCHAR_H_
/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS implementations of the Standard C Library extended multibyte and
          wide character handling.
*/

#include <aros/stdc/stdcnotimpl.h>
#include <aros/system.h>

#include <aros/types/wchar_t.h>
#include <aros/types/size_t.h>
#include <aros/types/mbstate_t.h>
#include <aros/types/wint_t.h>

#include <stdarg.h>
#include <stdint.h>

struct tm;

#include <aros/types/null.h>
#define	WCHAR_MIN	(-__WCHAR_MAX__ - 1l)
#define WCHAR_MAX	__WCHAR_MAX__

#ifndef WEOF
#define WEOF		((wint_t)-1)
#endif

/*  wchar.h should not include stdio.h */
struct __sFILE;


__BEGIN_DECLS

int __vwscanf(void *data, wint_t (*get_char)(void *), int (*unget_char)(wint_t, void *), const wchar_t * format, va_list args);
int __vwformat(void *data, wint_t (*outwc)(wchar_t, void *), const wchar_t * format, va_list args);
/*
 * stdc.library (45 funcs)
 */

/* Formatted wide-character input/output functions */
int vwscanf(const wchar_t * restrict format,
	va_list arg);
int vswscanf(const wchar_t * restrict s,
	const wchar_t * restrict format, va_list arg);
int swscanf(const wchar_t * restrict s,
	const wchar_t * restrict format, ...);
int wscanf(const wchar_t * restrict format, ...);
int swprintf(wchar_t * restrict s, size_t n,
	const wchar_t * restrict format, ...);
int vswprintf(wchar_t * restrict s, size_t n,
	const wchar_t * restrict format, va_list arg);
int vwprintf(const wchar_t * restrict format,
	va_list arg);
int wprintf(const wchar_t * restrict format, ...);
/* General wide-string utilities */
double wcstod(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr);
float wcstof(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr);
long double wcstold(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr);

long int wcstol(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr, int base);
unsigned long int wcstoul(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr,
	int base);
long long int wcstoll(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr, int base);
unsigned long long int wcstoull(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr, int base);
    
wchar_t *wcscat(wchar_t * restrict s1, const wchar_t * restrict s2);
wchar_t *wcsncat(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
int wcscmp(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcscpy(wchar_t *s1, const wchar_t *s2);
int wcscoll(const wchar_t *s1, const wchar_t *s2);
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wcsncpy(wchar_t *s1, const wchar_t *s2, size_t n);
size_t wcsxfrm(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
size_t wcscspn(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcschr(const wchar_t *s, wchar_t c);
size_t wcslen(const wchar_t *s);
wchar_t *wcspbrk(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
wchar_t *wcsstr(const wchar_t *s1, const wchar_t *s2);
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);
wchar_t *wmemcpy(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
wchar_t *wmemmove(wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);
size_t wcsspn(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcstok(wchar_t * restrict s1, const wchar_t * restrict s2,
	wchar_t ** restrict ptr);
int wmemcmp(const wchar_t * s1, const wchar_t * s2, size_t n);
/* Wide-character time conversion utilities */
size_t wcsftime(wchar_t * restrict s, size_t maxsize,
	const wchar_t * restrict format,
	const struct tm * restrict timeptr);
/* Extended multibyte and wide character utilities */
wint_t btowc(int c);
int wctob(wint_t c);

#if !defined(STDC_NOINLINE) && !defined(STDC_NOINLINE_WCHAR)
__header_inline int wcwidth(wchar_t wc) {
    if (wc == 0)
        return 0;
    if (wc < 32 || (wc >= 0x7f && wc < 0xa0))
        return -1; // control characters

#if defined(__WCHAR_MAX__) && (__WCHAR_MAX__ > 255)
    // Wide (East Asian Wide/Fullwidth) range
    if ((wc >= 0x1100 && wc <= 0x115F) || // Hangul Jamo
        (wc >= 0x2329 && wc <= 0x232A) ||
        (wc >= 0x2E80 && wc <= 0xA4CF) || // CJK...
        (wc >= 0xAC00 && wc <= 0xD7A3) ||
        (wc >= 0xF900 && wc <= 0xFAFF) ||
        (wc >= 0xFE10 && wc <= 0xFE19) ||
        (wc >= 0xFE30 && wc <= 0xFE6F) ||
        (wc >= 0xFF00 && wc <= 0xFF60) ||
        (wc >= 0xFFE0 && wc <= 0xFFE6))
        return 2;
#endif

    return 1;
}

__header_inline int wcswidth(const wchar_t *pwcs, size_t n) {
    int width = 0;
    size_t i;

    for (i = 0; i < n && pwcs[i] != L'\0'; ++i) {
        int w = wcwidth(pwcs[i]);
        if (w < 0)
            return -1;
        width += w;
    }
    return width;
}

__header_inline int mbsinit(const mbstate_t *ps)
{
    return (ps == NULL) || (ps->__state == 0 && ps->__count == 0 && ps->__value == 0);
}
#else
int wcwidth(wchar_t wc);
int wcswidth(const wchar_t *pwcs, size_t n);
int mbsinit(const mbstate_t *ps);
#endif
size_t mbrlen(const char * restrict s, size_t n, mbstate_t * restrict ps);
size_t mbrtowc(wchar_t * restrict pwc, const char * restrict s, size_t n,
	mbstate_t * restrict ps);
size_t wcrtomb(char * restrict s, wchar_t wc, mbstate_t * restrict ps);
size_t mbsrtowcs(wchar_t * restrict dst, const char ** restrict src,
	size_t len, mbstate_t * restrict ps);
size_t wcsrtombs(char * restrict dst, const wchar_t ** restrict src,
	size_t len, mbstate_t * restrict ps);
/*
 * stdcio.library
 */
/* Formatted wide-character input/output functions */
int fwprintf(struct __sFILE * restrict stream,
        const wchar_t * restrict format, ...);
int vfwprintf(struct __sFILE * restrict stream,
	const wchar_t * restrict format, va_list arg);
int fwscanf(struct __sFILE * restrict stream,
        const wchar_t * restrict format, ...);
int vfwscanf(struct __sFILE * restrict stream,
	const wchar_t * restrict format, va_list arg);

/* Wide-character input/output functions. */
wint_t getwchar(void);
wint_t putwchar(wchar_t c);
wint_t ungetwc(wint_t c, struct __sFILE *stream);

wint_t fgetwc(struct __sFILE *stream);
wchar_t *fgetws(wchar_t * restrict s,
	int n, struct __sFILE * restrict stream);
wint_t fputwc(wint_t c, struct __sFILE *stream);
wint_t fputws(const wchar_t *ws, struct __sFILE * restrict stream);
int fwide(struct __sFILE *stream, int mode);
wint_t getwc(struct __sFILE *stream);
wint_t putwc(wchar_t c, struct __sFILE *stream);

#if defined(_GNU_SOURCE) || defined(__BSD_VISIBLE)
wchar_t *wcswcs(const wchar_t *haystack, const wchar_t *needle);
#endif

__END_DECLS

#endif /* _STDC_WCHAR_H_ */
