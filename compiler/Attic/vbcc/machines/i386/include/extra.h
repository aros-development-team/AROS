#ifndef __EXTRA_H
#define __EXTRA_H 1

#include <time.h>
#include <string.h>

int chdir(const char *);
clock_t clock(void);
int getch(void);
int isseparator(int);
int iswhitespace(int);
int stricmp(const char *,const char *);
int strnicmp(const char *,const char *,size_t);

#ifdef __INLINE_ALL
#define __INLINE_ISWHITESPACE
#define __INLINE_ISSEPARATOR
#define __INLINE_STRICMP
#define __INLINE_STRNICMP
#endif

#ifdef __INLINE_ISWHITESPACE
#pragma only-inline on
#include "vbcc:libsrc/extra/iswhitespace.c"
#pragma only-inline off
#endif
#ifdef __INLINE_ISSEPARATOR
#pragma only-inline on
#include "vbcc:libsrc/extra/isseparator.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRICMP
#pragma only-inline on
#include "vbcc:libsrc/extra/stricmp.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRNICMP
#pragma only-inline on
#include "vbcc:libsrc/extra/strnicmp.c"
#pragma only-inline off
#endif

#endif

