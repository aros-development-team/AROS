/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Replacements for some C runtime functions
*/

#ifndef CRT_REPLACEMENT_H
#define CRT_REPLACEMENT_H

#include <exec/types.h>
#include <stddef.h>

/* string.h replacments */

static inline int Strcmp(const char *str1, const char *str2)
{
    int diff;

    while (!(diff = *(unsigned char*) str1 - *(unsigned char*) str2) && *str1)
    {
        str1 ++;
        str2 ++;
    }

    return diff;
}

static inline char *Strcpy(char *dest, const char *src)
{
    char *ptr = dest;

    while ((*ptr = *src))
    {
        ptr ++;
        src ++;
    }

    return dest;
}

static inline char *Strncpy(char *dest, const char *src, size_t n)
{
    char * ptr = dest;

    while (n && (*ptr = *src))
    {
        ptr ++;
        src ++;
        n--;
    }

    while (n--)
        *ptr++ = '\0';

    return dest;
}

static inline ULONG Strlen(CONST_STRPTR string)
{
    CONST_STRPTR str_start = (CONST_STRPTR)string;

    while (*string++);

    return (ULONG)(((IPTR)string) - ((IPTR)str_start)) - 1;
}

#endif
