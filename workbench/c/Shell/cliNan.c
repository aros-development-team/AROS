/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
 */

#include "Shell.h"

BOOL cliNan(CONST_STRPTR s)
{
    if (*s == '+' || *s == '-')
    {
        ++s;
        if (*s == '\0')
            return TRUE;
    }

    for (; *s != '\0'; ++s)
        if (*s < '0' || *s > '9')
            return TRUE;
    return FALSE;
}

