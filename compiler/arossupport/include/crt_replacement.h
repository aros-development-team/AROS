/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Replacements for some C runtime functions
*/

#ifndef CRT_REPLACEMENT_H
#define CRT_REPLACEMENT_H

#include <exec/types.h>

/* string.h replacments */

static inline char *StrCpy(char *dest, const char *src)
{
    char *ptr = dest;

    while ((*ptr = *src))
    {
        ptr ++;
        src ++;
    }

    return dest;
}


static inline ULONG Strlen(CONST_STRPTR string)
{
    CONST_STRPTR str_start = (CONST_STRPTR)string;

    while (*string++);

    return (ULONG)(((IPTR)string) - ((IPTR)str_start)) - 1;
}

#endif
