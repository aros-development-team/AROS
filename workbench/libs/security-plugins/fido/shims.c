/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: The handful of C runtime functions libmbedcrypto.a needs, for a
          security.library plugin that has no runtime (usestartup=no).
          Memory comes from exec; SysBase is set by the plugin's Initialize.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void *memcpy(void *d, const void *s, size_t n)
{
    UBYTE *dp = d;
    const UBYTE *sp = s;

    while (n--)
        *dp++ = *sp++;
    return d;
}

void *memmove(void *d, const void *s, size_t n)
{
    UBYTE *dp = d;
    const UBYTE *sp = s;

    if (dp < sp)
    {
        while (n--)
            *dp++ = *sp++;
    }
    else if (dp > sp)
    {
        dp += n;
        sp += n;
        while (n--)
            *--dp = *--sp;
    }
    return d;
}

void *memset(void *d, int c, size_t n)
{
    UBYTE *dp = d;

    while (n--)
        *dp++ = (UBYTE)c;
    return d;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const UBYTE *ap = a, *bp = b;

    while (n--)
    {
        if (*ap != *bp)
            return (int)*ap - (int)*bp;
        ap++;
        bp++;
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t n = 0;

    while (s[n])
        n++;
    return n;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b)
    {
        a++;
        b++;
    }
    return (int)(UBYTE)*a - (int)(UBYTE)*b;
}

void *calloc(size_t n, size_t size)
{
    return AllocVec(n * size, MEMF_ANY | MEMF_CLEAR);
}

void free(void *p)
{
    if (p)
        FreeVec(p);
}

/* mbedTLS's platform_util refers to these; nothing here keeps time */
struct tm *gmtime_r(const time_t *t, struct tm *out)
{
    return NULL;
}

struct tm *gmtime(const time_t *t)
{
    return NULL;
}
