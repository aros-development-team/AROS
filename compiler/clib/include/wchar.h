#ifndef _WCHAR_H_
#define _WCHAR_H_
/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Standard C Library: Extended multibyte and wide character utilities.
*/

#include <sys/_types.h>
#include <sys/cdefs.h>

#define __need_size_t
#define __need_wchar_t
#define __need_wint_t
#define __need_NULL
#include <stddef.h>
#include <stdarg.h>

#include <stdint.h>

__BEGIN_DECLS

/* Users are not allowed to access this type. */
typedef union
{
    char    _mbs[32];
    int64_t _mbs_align;
} mbstate_t;

struct tm;

#define	WCHAR_MIN	(-__WCHAR_MAX__ - 1l)
#define WCHAR_MAX	__WCHAR_MAX__

#ifndef WEOF
#define WEOF		((wint_t)-1)
#endif

/*  wchar.h should not include stdio.h */
struct __sFILE;


/* Formatted wide-character input/output functions */
int fwprintf(struct __sFILE * restrict stream,
        const wchar_t * restrict format, ...);
int fwscanf(struct __sFILE * restrict stream,
        const wchar_t * restrict format, ...);
int swprintf(wchar_t * restrict s, size_t n,
	const wchar_t * restrict format, ...);
int swscanf(const wchar_t * restrict s,
	const wchar_t * restrict format, ...);
int vfwprintf(struct __sFILE * restrict stream,
	const wchar_t * restrict format, va_list arg);
int vfwscanf(struct __sFILE * restrict stream,
	const wchar_t * restrict format, va_list arg);
int vswprintf(wchar_t * restrict s, size_t n,
	const wchar_t * restrict format, va_list arg);
int vswscanf(const wchar_t * restrict s,
	const wchar_t * restrict format, va_list arg);
int vwprintf(const wchar_t * restrict format,
	va_list arg);
int vwscanf(const wchar_t * restrict format,
	va_list arg);
int wprintf(const wchar_t * restrict format, ...);
int wscanf(const wchar_t * restrict format, ...);

/* Wide-character input/output functions. */
wint_t fgetwc(struct __sFILE *stream);
wchar_t *fgetws(wchar_t * restrict s,
	int n, struct __sFILE * restrict stream);
wint_t fputwc(wchar_t c, struct __sFILE *stream);
int fputws(const wchar_t * restrict s,
	struct __sFILE * restrict stream);
int fwide(struct __sFILE *stream, int mode);
wint_t getwc(struct __sFILE *stream);
wint_t getwchar(void);
wint_t putwc(wchar_t c, struct __sFILE *stream);
wint_t putwchar(wchar_t c);
wint_t ungetwc(wint_t c, struct __sFILE *stream);

/* General wide-string utilities */
double wcstod(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr);
float wcstof(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr);
#if C99
long double wcstold(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr);
#endif

long int wcstol(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr, int base);
unsigned long int wcstoul(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr,
	int base);
#if C99
long long int wcstoll(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr, int base);
unsigned long long int wcstoull(const wchar_t * restrict nptr,
	wchar_t ** restrict endptr, int base);
#endif

wchar_t *wcscat(wchar_t * restrict s1, const wchar_t * restrict s2);
wchar_t *wcsncat(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
int wcscmp(const wchar_t *s1, const wchar_t *s2);
int wcscoll(const wchar_t *s1, const wchar_t *s2);
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);
size_t wcsxfrm(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
wchar_t *wcschr(const wchar_t *s, wchar_t c);
size_t wcscspn(const wchar_t *s1, const wchar_t *s2);
size_t wcslen(const wchar_t *s);
wchar_t *wcspbrk(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
size_t wcsspn(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcsstr(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcstok(wchar_t * restrict s1, const wchar_t * restrict s2,
	wchar_t ** restrict ptr);
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);
int wmemcmp(const wchar_t * s1, const wchar_t * s2, size_t n);
wchar_t *wmemcpy(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
wchar_t *wmemmove(wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);

/* Wide-character time conversion utilities */
size_t wcsftime(wchar_t * restrict s, size_t maxsize,
	const wchar_t * restrict format,
	const struct tm * restrict timeptr);

/* Extended multibyte and wide character utilities */
wint_t btowc(int c);
int wctob(wint_t c);
int mbsinit(const mbstate_t *ps);
size_t mbrlen(const char * restrict s, size_t n, mbstate_t * restrict ps);
size_t mbrtowc(wchar_t * restrict pwc, const char * restrict s, size_t n,
	mbstate_t * restrict ps);
size_t wcrtomb(char * restrict s, wchar_t wc, mbstate_t * restrict ps);
size_t mbsrtowcs(wchar_t * restrict dst, const char ** restrict src,
	size_t len, mbstate_t * restrict ps);
size_t wcsrtombs(char * restrict dst, const wchar_t ** restrict src,
	size_t len, mbstate_t * restrict ps);

__END_DECLS

#endif /* _WCHAR_T_ */
