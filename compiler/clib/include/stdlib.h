#ifndef _STDLIB_H
#define _STDLIB_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file stdlib.h
    Lang: English
*/
#ifndef _SYS_TYPES_H
#   include <sys/types.h>
#endif

#define EXIT_SUCCESS	0 /* Success exit status */
#define EXIT_FAILURE	1 /* Failing exit status */

void __attribute__ ((noreturn)) exit (int code);
void __attribute__ ((noreturn)) abort (void);

int atexit(void (*func)(void));

int abs (int j);
long labs (long j);
double atof (const char *str);
int atoi (const char *str);
long atol (const char *str);
long strtol (const char *str, char **endptr, int base);
unsigned long strtoul (const char *str, char **endptr, int base);
double strtod(const char *str, char **endptr);

int rand (void);
void srand (unsigned int seed);

double drand48(void);
double erand48(unsigned short int xsubi[3]);
long int lrand48(void);
long int nrand48(unsigned short int xsubi[3]);
long int mrand48(void);
long int jrand48(unsigned short int xsubi[3]);
void srand48(long int seedval);
unsigned short int *seed48(unsigned short int seed16v[3]);
void lcong48(unsigned short int param[7]);

/* Max. number returned by rand() */
#ifndef RAND_MAX
#   define RAND_MAX	   2147483647
#endif

long random(void);
void srandom(unsigned seed);
char *initstate(unsigned seed, char *state, int n);
char *setstate(char *state);

void qsort(void * array, size_t count, size_t elementsize,
	int (*comparefunction)(const void * element1, const void * element2));
void *bsearch(const void * key, const void * base, size_t count,
	size_t size, int (*comparefunction)(const void *, const void *));

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *oldmem, size_t newsize);
void  free(void *memory);

char *getenv(const char *name);
int   setenv(const char *name, const char *value, int overwrite);
int   putenv(const char *string);
int   unsetenv(const char *name);

char *mktemp(char *buf);
int   system(const char *string);

char *gcvt(double number, size_t ndigit, char *buf);

div_t div(int numer, int denom);
ldiv_t ldiv(long int numer, long int denom);

#endif /* _STDLIB_H */
