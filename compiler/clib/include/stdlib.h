#ifndef _STDLIB_H_
#define _STDLIB_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file stdlib.h
    Lang: English
*/

#include <sys/cdefs.h>
#include <sys/arosc.h>

/* It seems that also stdlib.h defines alloca() */
#include <alloca.h>

/* For wchar_t and size_t */
#include <stddef.h>

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

#ifndef NULL
#define NULL	    0
#endif

#define EXIT_SUCCESS	0  /* Success exit status */
#define EXIT_FAILURE	20 /* Failing exit status */

/* Gives the largest size of a multibyte character for the current locale */
#define MB_CUR_MAX      (__get_arosc_userdata()->acud_mb_cur_max)

__BEGIN_DECLS

/* String conversion functions */
double atof(const char *nptr);
int atoi(const char *nptr);
long int atol(const char *nptr);
#if defined AROS_HAVE_LONG_LONG
long long int atoll(const char *nptr);
#endif

double strtod(const char * restrict nptr, char ** restrict endptr);
float strtof(const char * restrict nptr, char ** restrict endptr);
long double strtold(const char * restrict nptr, char ** restrict endptr);
long int strtol(const char * restrict nptr,
		char ** restrict endptr,
		int base);
unsigned long int strtoul(const char * restrict nptr,
		char ** restrict endptr,
		int base);

#if defined AROS_HAVE_LONG_LONG
long long int strtoll(const char * restrict nptr,
		char ** restrict endptr,
		int base);
unsigned long long int strtoull(const char * restrict nptr,
		char ** restrict endptr,
		int base);
#endif

/* Pseudo-random number generation functions */
int rand (void);
void srand (unsigned int seed);

/* Max. number returned by rand() */
#ifndef RAND_MAX
#   define RAND_MAX	   2147483647
#endif

/* Unix pseudo-random functions */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
#ifndef AROS_NOFPU
double drand48(void);
double erand48(unsigned short int xsubi[3]);
#else
void drand48(void);
void erand48(unsigned short int xsubi[3]);
#endif
long int lrand48(void);
long int nrand48(unsigned short int xsubi[3]);
long int mrand48(void);
long int jrand48(unsigned short int xsubi[3]);
void srand48(long int seedval);
unsigned short int *seed48(unsigned short int seed16v[3]);
void lcong48(unsigned short int param[7]);

/* Thread safe random */
int       rand_r(unsigned int *);

long random(void);
void srandom(unsigned seed);
char *initstate(unsigned seed, char *state, int n);
char *setstate(char *state);
#endif /* !_ANSI_SOURCE  && !_POSIX_SOURCE */

/* Memory management functions */
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *oldmem, size_t newsize);
void *realloc_nocopy(void *oldmem, size_t newsize); /* AROS specific */
void  free(void *memory);

/* Communication with the environment */
void  abort (void) __noreturn;
int   atexit(void (*func)(void));
int   on_exit(void (*func)(int, void *), void *);
void  exit (int code) __noreturn;
int   system(const char *string);
char *getenv(const char *name);

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
int   putenv(const char *string);
int   setenv(const char *name, const char *value, int overwrite);
void  unsetenv(const char *name);
#endif

/* Searching and sorting utilities */
void qsort(void * array, size_t count, size_t elementsize,
	int (*comparefunction)(const void * element1, const void * element2));
void *bsearch(const void * key, const void * base, size_t count,
	size_t size, int (*comparefunction)(const void *, const void *));

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

/* Multibyte character functions */
size_t mblen(const char *s, size_t n);
int mbtowc(wchar_t * restrict pwc, const char * restrict s, size_t n);
int wctomb(char *s, wchar_t wchar);

/* Multibyte string functions */
size_t mbstowcs(wchar_t * restrict pwcs, const char * restrict s, size_t n);
size_t wcstombs(char * restrict s, const wchar_t * restrict pwcs, size_t n);

/* Miscellaneous BSD functions */
int getloadavg(double loadavg[], int n);

/* The following are POSIX/SUS additions */
#if !defined(_ANSI_SOURCE)
long  a64l(const char *);
#if 0 /* FIXME: not implemented */
char *ecvt(double, int, int *, int *); 
char *fcvt (double, int, int *, int *);
#endif
char *gcvt(double, int, char *);
int   getsubopt(char **, char *const *, char **);
int   grantpt(int);
char *l64a(long);
char *mktemp(char *);
int   mkstemp(char *);
char *ptsname(int);
char *realpath(const char *, char *);
void  setkey(const char *);
int   unlockpt(int);
#endif /* _ANSI_SOURCE */

__END_DECLS

#endif /* _STDLIB_H_ */
