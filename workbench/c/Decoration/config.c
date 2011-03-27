/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"


static STRPTR SkipChars(STRPTR v)
{
    STRPTR c;
    c = strstr(v, "=");
    return ++c;
}

LONG GetInt(STRPTR v)
{
    STRPTR c;
    c = SkipChars(v);
    return (LONG) atol(c);
}

void GetIntegers(STRPTR v, LONG *v1, LONG *v2)
{
    STRPTR c;
    TEXT va1[32], va2[32];
    LONG cnt;
    c = SkipChars(v);
    if (c)
    {
        cnt = sscanf(c, "%s %s", va1, va2);
        if (cnt == 1)
        {
            *v1 = -1;
            *v2 = atol(va1);
        }
        else if (cnt == 2)
        {
            *v1 = atol(va1);
            *v2 = atol(va2);
        }
    }
}

void GetTripleIntegers(STRPTR v, LONG *v1, LONG *v2, LONG *v3)
{
    STRPTR ch;
    LONG a, b, c;
    LONG cnt;
    ch = SkipChars(v);
    if (ch)
    {
        cnt = sscanf(ch, "%x %x %d", &a, &b, &c);
        if (cnt == 3)
        {
            *v1 = a;
            *v2 = b;
            *v3 = c;
        }
    }
}

void GetColors(STRPTR v, LONG *v1, LONG *v2)
{
    STRPTR ch;
    LONG a, b;
    LONG cnt;
    ch = SkipChars(v);
    if (ch)
    {
        cnt = sscanf(ch, "%x %x", &a, &b);
        if (cnt == 2)
        {
            *v1 = a;
            *v2 = b;
        }
    }
}


BOOL GetBool(STRPTR v, STRPTR id)
{
    if (strstr(v, id)) return TRUE; else return FALSE;
}
