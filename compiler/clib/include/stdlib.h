#ifndef _STDLIB_H
#define _STDLIB_H

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI-C header file stdlib.h
    Lang: english
*/
#ifndef _SYS_TYPES_H
#   include <sys/types.h>
#endif

#define EXIT_SUCCESS	0 /* Success exit status */
#define EXIT_FAILURE	1 /* Failing exit status */

void __attribute__ ((noreturn)) exit (int code);
void __attribute__ ((noreturn)) abort (void);

int abs (int j);
long labs (long j);
double atof (const char * str);
int atoi (const char * str);
long atol (const char * str);
long strtol (const char * str, char ** endptr, int base);
unsigned long strtoul (const char * str, char ** endptr, int base);
double strtod(const char * str, char ** endptr);

int rand (void);
void srand (unsigned int seed);
/* Max. number returned by rand() */
#ifndef RAND_MAX
#   define RAND_MAX	   2147483647
#endif

void qsort (void * array, size_t count, size_t elementsize,
	int (*comparefunction)(const void * element1, const void * element2));
void * bsearch (const void * key, const void * base, size_t count,
	size_t size, int (*comparefunction)(const void *, const void *));

void * malloc (size_t size);
void * calloc (size_t count, size_t size);
void * realloc (void * oldmem, size_t newsize);
void free (void * memory);

#endif /* _STDLIB_H */
