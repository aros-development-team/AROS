#ifndef _STDC_STDLIB_H_
#define _STDC_STDLIB_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file stdlib.h
*/

#include <aros/system.h>

#include <aros/types/size_t.h>
#include <aros/types/wchar_t.h>

/* Types for div and ldiv */
typedef struct div_t {
    int quot;
    int rem;
} div_t;

typedef struct ldiv_t {
    long int quot;
    long int rem;
} ldiv_t;

#if defined AROS_HAVE_LONG_LONG
typedef struct lldiv_t {
    long long int   quot;
    long long int   rem;
} lldiv_t;
#endif

#include <aros/types/null.h>

#define EXIT_SUCCESS	0  /* Success exit status */
#define EXIT_FAILURE	20 /* Failing exit status */

/* Gives the largest size of a multibyte character for the current locale */
#define MB_CUR_MAX      (__stdc_mb_cur_max())

#define RAND_MAX	   2147483647


__BEGIN_DECLS

/* Numeric conversion functions */
double atof(const char *nptr);
int atoi(const char *nptr);
long int atol(const char *nptr);
#if defined AROS_HAVE_LONG_LONG
/* NOTIMPL long long int atoll(const char *nptr); */
#endif

double strtod(const char * restrict nptr, char ** restrict endptr);
/* NOTIMPL float strtof(const char * restrict nptr, char ** restrict endptr); */
/* NOTIMPL long double strtold(const char * restrict nptr, char ** restrict endptr); */

long int strtol(const char * restrict nptr,
		char ** restrict endptr,
		int base);
#if defined AROS_HAVE_LONG_LONG
long long int strtoll(const char * restrict nptr,
		char ** restrict endptr,
		int base);
#endif
unsigned long int strtoul(const char * restrict nptr,
		char ** restrict endptr,
		int base);
#if defined AROS_HAVE_LONG_LONG
unsigned long long int strtoull(const char * restrict nptr,
		char ** restrict endptr,
		int base);
#endif

/* Pseudo-random sequence generation functions */
int rand (void);
void srand (unsigned int seed);

/* Memory management functions */
void *calloc(size_t count, size_t size);
void free(void *memory);
void *malloc(size_t size);
void *realloc(void *oldmem, size_t newsize);

/* Communication with the environment */
void abort (void) __noreturn;
int atexit(void (*func)(void));
void exit(int code) __noreturn;
/* NOTIMPL void _Exit(int status); */
char *getenv(const char *name);
int system(const char *string);

/* Searching and sorting utilities */
void *bsearch(const void * key, const void * base, size_t count,
	size_t size, int (*comparefunction)(const void *, const void *));
void qsort(void * array, size_t count, size_t elementsize,
	int (*comparefunction)(const void * element1, const void * element2));

/* Integer arithmetic functions */
int abs (int j);
long labs (long j);
#if defined AROS_HAVE_LONG_LONG
long long int llabs(long long int j);
#endif

div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);
#if defined AROS_HAVE_LONG_LONG
lldiv_t lldiv(long long int numer, long long int denom);
#endif

/* Multibyte/wide character conversion functions */
int mblen(const char *s, size_t n);
/* INLINE int mbtowc(wchar_t * restrict pwc, const char * restrict s, size_t n); */
/* INLINE int wctomb(char *s, wchar_t wchar); */

/* Multibyte/wide string conversion functions */
/* INLINE size_t mbstowcs(wchar_t * restrict pwcs, const char * restrict s, size_t n); */
/* INLINE size_t wcstombs(char * restrict s, const wchar_t * restrict pwcs, size_t n); */

/* AROS extra */
int __stdc_mb_cur_max(void);
void *malloc_align(size_t size, size_t alignment); /* AROS specific */
void *realloc_nocopy(void *oldmem, size_t newsize); /* AROS specific */
int   on_exit(void (*func)(int, void *), void *);

/* inline code */
/* The multi-byte character functions are implemented inline so that they adapt to the
   size of wchar_t used by the compiler. This would not be possible if the code would
   be compiled in the shared library or even the static link library.
*/

#if !defined(_STDC_NOINLINE) && !defined(_STDC_NOINLINE_STDLIB)
/* This is name space pollution */
#include <ctype.h>


#if !defined(_STDC_NOINLINE_MBTOWC)
static __inline__
int mbtowc(wchar_t * restrict pwc, const char * restrict s, size_t n)
{
    if (s == NULL)
        /* No state-dependent multi-byte character encoding */
        return 0;
    else
    {
        if (isascii(*s))
        {
            if (pwc)
                *pwc = (wchar_t)*s;
            if (*s == 0)
                return 0;
            else
                return 1;
        }
        else
            return -1;
    }
}
#endif /* !_STDC_NOINLINE_MBTOWC */


#if !defined(_STDC_NOINLINE_WCTOMB)
static __inline__
int wctomb(char *s, wchar_t wchar)
{
    if (s == NULL)
        /* No state dependent encodings */
        return 0;
    else if (isascii((int)wchar))
    {
        *s = (char)wchar;
        if (wchar == 0)
            return 0;
        else
            return 1;
    }
    else
        return -1;
}
#endif /* !_STDC_NOINLINE_WCTOMB */


#if !defined(_STDC_NOINLINE_MBSTOWCS)
static __inline__
size_t mbstowcs(wchar_t * restrict pwcs, const char * restrict s, size_t n)
{
    size_t l;

    for(l = 0; n > 0; s++, pwcs++, n--)
    {
        *pwcs = (wchar_t)*s;

        if (*s == '\0')
            break;

        l++;
    }

    return l;
}
#endif /* !_STDC_NOINLINE_MBSTOWCS */


#if !defined(_STDC_NOINLINE_WCSTOMBS)
static __inline__
size_t wcstombs(char * restrict s, const wchar_t * restrict pwcs, size_t n)
{
    size_t l;

    for(l = 0; n > 0; s++, pwcs++, n--)
    {
        if (!isascii((int)*pwcs))
            return (size_t)-1;

        *s = (char)*pwcs;

        if (*s == '\0')
            break;

        l++;
    }

    return l;
}
#endif /* !_STDC_NOINLINE_MBTOWC */


#endif /* !_STDC_NOINLINE && !_STDC_NOINLINE_STDLIB */

__END_DECLS

#endif /* _STDC_STDLIB_H_ */
