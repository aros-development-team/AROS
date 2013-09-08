#ifndef _POSIXC_STDLIB_H_
#define _POSIXC_STDLIB_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 header file stdlib.h
*/

/* C99 */
#include <aros/stdc/stdlib.h>

/* It seems that also stdlib.h defines alloca() */
#include <alloca.h>

__BEGIN_DECLS

/* NOTIMPL long a64l(const char *); */
double drand48(void);
double erand48(unsigned short [3]);
/* NOTIMPL int getsubopt(char **, char *const *, char **); */
/* NOTIMPL int grantpt(int); */
char *initstate(unsigned, char *, int);
long int jrand48(unsigned short int [3]);
/* NOTIMPL char *l64a(long); */
void lcong48(unsigned short int [7]);
long int lrand48(void);
/* NOTIMPL char *mkdtemp(char *); */
int mkstemp(char *);
long int mrand48(void);
long int nrand48(unsigned short int [3]);
int posix_memalign(void **memptr, size_t alignment, size_t size);
/* NOTIMPL int posix_openpt(int); */
/* NOTIMPL char *ptsname(int); */
int putenv(const char *);
/* NOTIMPL int rand_r(unsigned int *); */
long random(void);
/* NOTIMPL char *realpath(const char * restrict , char * restrict); */
unsigned short int *seed48(unsigned short int [3]);
int setenv(const char *, const char *, int);
/* NOTIMPL void setkey(const char *); */
char *setstate(char *);
void srand48(long int);
void srandom(unsigned);
/* NOTIMPL int unlockpt(int); */
void unsetenv(const char *);

/* The following are deprecated POSIX functions */
char *mktemp(char *);

/* BSD */
int getloadavg(double loadavg[], int n);

__END_DECLS

#endif /* _POSIXC_STDLIB_H_ */
