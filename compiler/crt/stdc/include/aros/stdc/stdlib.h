#ifndef _STDC_STDLIB_H_
#define _STDC_STDLIB_H_

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS implementation of the C Standard Library Header (C89/C99/GNU)
*/

#include <aros/stdc/stdcnotimpl.h>
#include <aros/system.h>

#include <aros/types/size_t.h>
#include <aros/types/wchar_t.h>
#include <aros/types/null.h>

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
    long long int quot;
    long long int rem;
} lldiv_t;
#endif

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    20

#ifndef MB_CUR_MAX
__BEGIN_DECLS
int __stdc_mb_cur_max(void);
__END_DECLS
#define MB_CUR_MAX      (__stdc_mb_cur_max())
#endif
#define RAND_MAX        2147483647

__BEGIN_DECLS

/* Numeric conversion functions */
double atof(const char *nptr);
int atoi(const char *nptr);
long int atol(const char *nptr);
#if defined AROS_HAVE_LONG_LONG
long long int atoll(const char *nptr);
#endif

double strtod(const char * restrict nptr, char ** restrict endptr);
float strtof(const char * restrict nptr, char ** restrict endptr);
long double strtold(const char * restrict nptr, char ** restrict endptr);
long int strtol(const char * restrict nptr, char ** restrict endptr, int base);
#if defined AROS_HAVE_LONG_LONG
long long int strtoll(const char * restrict nptr, char ** restrict endptr, int base);
#endif
unsigned long int strtoul(const char * restrict nptr, char ** restrict endptr, int base);
#if defined AROS_HAVE_LONG_LONG
unsigned long long int strtoull(const char * restrict nptr, char ** restrict endptr, int base);
#endif

/* Pseudo-random sequence generation */
int rand(void);
void srand(unsigned int seed);

/* Memory management */
void *calloc(size_t count, size_t size);
void free(void *memory);
void *malloc(size_t size);
void *realloc(void *oldmem, size_t newsize);

/* Environment interaction */
void abort(void) __noreturn;
int atexit(void (*func)(void));
void exit(int code) __noreturn;
void _Exit(int status) __noreturn;

#if !defined(__STRICT_ANSI__)
char *getenv(const char *name);
int system(const char *string);
#endif

/* Searching and sorting */
void *bsearch(const void * key, const void * base, size_t count, size_t size,
              int (*comparefunction)(const void *, const void *));
void qsort(void * array, size_t count, size_t elementsize,
           int (*comparefunction)(const void *, const void *));

/* Integer arithmetic */
int abs(int j);
long labs(long j);
#if defined AROS_HAVE_LONG_LONG
long long int llabs(long long int j);
#endif

div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);
#if defined AROS_HAVE_LONG_LONG
lldiv_t lldiv(long long int numer, long long int denom);
#endif

/* Multibyte/wide character conversion */
int mblen(const char *s, size_t n);
int mbtowc(wchar_t * restrict pwc, const char * restrict s, size_t n);
int wctomb(char *s, wchar_t wchar);

/* Multibyte/wide string conversion */
size_t mbstowcs(wchar_t * restrict pwcs, const char * restrict s, size_t n);
size_t wcstombs(char * restrict s, const wchar_t * restrict pwcs, size_t n);

void *aligned_alloc(size_t alignment, size_t size);

/* AROS-specific extensions */
void *realloc_nocopy(void *oldmem, size_t newsize);
int on_exit(void (*func)(int, void *), void *);

__END_DECLS

#endif /* _STDC_STDLIB_H_ */
