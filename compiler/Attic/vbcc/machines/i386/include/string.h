#ifndef __STRING_H
#define __STRING_H 1

#include <stddef.h>

void *memcpy(void *,const void *,size_t);
void *memmove(void *,const void *,size_t);
char *strcpy(char *,const char *);
char *strncpy(char *,const char *,size_t);
char *strcat(char *,const char *);
char *strncat(char *,const char *,size_t);
int memcmp(const void *,const void *,size_t);
int strcmp(const char *,const char *);
int strncmp(const char *,const char *,size_t);
void *memchr(const void *,int,size_t);
char *strchr(const char *,int);
size_t strcspn(const char *,const char *);
char *strpbrk(const char *,const char *);
char *strrchr(const char *,int);
size_t strspn(const char *,const char *);
char *strstr(const char *,const char *);
void *memset(void *,int,size_t);
size_t strlen(const char *);
char *strtok(char *,const char *);
char *strerror(size_t);

#ifdef __INLINE_ALL
#define __INLINE_MEMCHR
#define __INLINE_MEMCMP
#define __INLINE_MEMCPY
#define __INLINE_MEMMOVE
#define __INLINE_MEMSET
#define __INLINE_STRCAT
#define __INLINE_STRCHR
#define __INLINE_STRCMP
#define __INLINE_STRCPY
#define __INLINE_STRCSPN
#define __INLINE_STRERROR
#define __INLINE_STRLEN
#define __INLINE_STRNCAT
#define __INLINE_STRNCMP
#define __INLINE_STRNCPY
#define __INLINE_STRPBRK
#define __INLINE_STRRCHR
#define __INLINE_STRSPN
#define __INLINE_STRSTR
#define __INLINE_STRTOL
#endif

#ifdef __INLINE_MEMCHR
#pragma only-inline on
#include "vbcc:libsrc/string/memchr.c"
#pragma only-inline off
#endif
#ifdef __INLINE_MEMCMP
#pragma only-inline on
#include "vbcc:libsrc/string/memcmp.c"
#pragma only-inline off
#endif
#ifdef __INLINE_MEMCPY
#pragma only-inline on
#include "vbcc:libsrc/string/memcpy.c"
#pragma only-inline off
#endif
#ifdef __INLINE_MEMMOVE
#pragma only-inline on
#include "vbcc:libsrc/string/memmove.c"
#pragma only-inline off
#endif
#ifdef __INLINE_MEMSET
#pragma only-inline on
#include "vbcc:libsrc/string/memset.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRCAT
#pragma only-inline on
#include "vbcc:libsrc/string/strcat.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRCHR
#pragma only-inline on
#include "vbcc:libsrc/string/strchr.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRCMP
#pragma only-inline on
#include "vbcc:libsrc/string/strcmp.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRCPY
#pragma only-inline on
#include "vbcc:libsrc/string/strcpy.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRCSPN
#pragma only-inline on
#include "vbcc:libsrc/string/strcspn.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRERROR
#pragma only-inline on
#include "vbcc:libsrc/string/strerror.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRLEN
#pragma only-inline on
#include "vbcc:libsrc/string/strlen.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRNCAT
#pragma only-inline on
#include "vbcc:libsrc/string/strncat.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRNCMP
#pragma only-inline on
#include "vbcc:libsrc/string/strncmp.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRNCPY
#pragma only-inline on
#include "vbcc:libsrc/string/strncpy.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRPBRK
#pragma only-inline on
#include "vbcc:libsrc/string/strpbrk.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRRCHR
#pragma only-inline on
#include "vbcc:libsrc/string/strrchr.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRSPN
#pragma only-inline on
#include "vbcc:libsrc/string/strspn.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRSTR
#pragma only-inline on
#include "vbcc:libsrc/string/strstr.c"
#pragma only-inline off
#endif
#ifdef __INLINE_STRTOK
#pragma only-inline on
#include "vbcc:libsrc/string/strtok.c"
#pragma only-inline off
#endif



#endif

