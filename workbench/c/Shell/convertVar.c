/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/dos.h>

#include <ctype.h>

#include "Shell.h"

#include <aros/debug.h>

/* environment variables handling (locals and globals) */
LONG convertVar(ShellState *ss, Buffer *in, Buffer *out, BOOL *quoted)
{
    STRPTR p = in->buf + in->cur;
    STRPTR s = p + 1;
    LONG bra = (*s == '{' ? 2 : 0);
    TEXT varName[257];
    TEXT varValue[256];
    LONG i, len;

    if (bra)
	++s;

    for (i = 0; i < 256; ++i)
    {
	if (bra)
        {
	    if (*s == '\0' || *s == '}')
		break;
        }
        else
        {
            if (!isalnum(*s))
	        break;
	}

	varName[i] = *s++;
    }

    if (i >= 256) /* FIXME setup a VAR_MAX constant */
	return ERROR_LINE_TOO_LONG;

    varName[i] = '\0';

    if (bra)
    {
        if (*s == '}')
            s++;
        else
            bra--;
    }

    if ((bra != 1) && (i > 0) && ((len = GetVar(varName, varValue, 256, LV_VAR)) != -1))
    {
	D(bug("[Shell] found var: %s = %s\n", varName, varValue));
	bufferAppend(varValue, len, out, SysBase);
    }
    else
    {
	D(bug("[Shell] var not found: %s\n", varName));
	bufferAppend(p, ++i + bra, out, SysBase);
    }

    in->cur = s - in->buf;
    return 0;
}
