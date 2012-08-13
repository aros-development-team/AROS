/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/dos.h>
#include "Shell.h"

LONG l2a(LONG x, STRPTR buf) /* long to ascii */
{
    LONG a = (x < 0 ? -x : x);
    LONG i = 31, j, len = 0;
    TEXT tmp[32];

    do {
	tmp[i] = '0' + (a % 10);
	++len;
    } while (i-- && (a /= 10));

    if (x < 0)
    {
	tmp[i--] = '-';
	++len;
    }

    for (j = 0; j < len; ++j)
	buf[j] = tmp[++i];

    buf[j] = '\0';
    return len;
}

void cliVarNum(ShellState *ss, CONST_STRPTR name, LONG value)
{
    TEXT buf[32];
    LONG len = l2a(value, buf);
    SetVar(name, buf, len, GVF_LOCAL_ONLY);
}

