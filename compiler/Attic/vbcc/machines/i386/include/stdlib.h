#ifndef __STDLIB_H
#define __STDLIB_H 1

#include <stddef.h>
#include <limits.h>

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 20
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#define RAND_MAX 32768

void exit(int);
void *malloc(size_t);
void *calloc(size_t,size_t);
void *realloc(void *,size_t);
void free(void *);
int system(const char *);
int rand(void);
void srand(unsigned int);
double atof(const char *);
int atoi(const char *);
long atol(const char *);
double strtod(const char *,char **);
long strtol(const char *,char **,int);
unsigned long strtoul(const char *,char **,int);
void abort(void);
int atexit(void (*)(void));
char *getenv(const char *);
void *bsearch(const void *,const void *,size_t,size_t,int (*)(const void *,const void *));
void qsort(void *,size_t,size_t,int (*)(const void *,const void *));
int abs(int);
long labs(long);

typedef struct {
    int quot,rem;
} div_t;

typedef struct {
    long quot,rem;
} ldiv_t;

div_t div(int,int);
ldiv_t ldiv(long,long);

union _mheader {
    struct{
        union _mheader *ptr;
        size_t size;
    } s;
    long align;
};

extern size_t _nalloc;

#define atof(s) strtod((s),(char **)NULL)
#define atoi(s) (int)strtol((s),(char **)NULL,10)
#define atol(s) strtol((s),(char **)NULL,10)

struct __exitfuncs{
    struct __exitfuncs *next;
    void (*func)(void);
};

#ifdef __INLINE_ALL
#define __INLINE_ABS
#define __INLINE_LABS
#define __INLINE_DIV
#define __INLINE_LDIV
#endif

#ifdef __INLINE_ABS
#pragma only-inline on
#include "vbcc:libsrc/stdlib/abs.c"
#pragma only-inline off
#endif
#ifdef __INLINE_LABS
#pragma only-inline on
#include "vbcc:libsrc/stdlib/labs.c"
#pragma only-inline off
#endif
#ifdef __INLINE_DIV
#pragma only-inline on
#include "vbcc:libsrc/stdlib/div.c"
#pragma only-inline off
#endif
#ifdef __INLINE_LDIV
#pragma only-inline on
#include "vbcc:libsrc/stdlib/ldiv.c"
#pragma only-inline off
#endif


#endif

