/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <string.h>
#include "Shell.h"

/* subsitute one script argument and leaves the input after .ket */
LONG convertArg(ShellState *ss, Buffer *in, Buffer *out, BOOL *quoted)
{
    STRPTR s = in->buf + in->cur;
    STRPTR p = s;
    STRPTR q = ++s;
    LONG i;
    BOOL scriptarg = FALSE;

    if (s[0] == ss->dollar && s[1] == ss->dollar && s[2] == ss->ket)
    {
        TEXT buf[16];
        LONG len = l2a(ss->cliNumber, buf);
        bufferAppend(buf, len, out, SysBase);
        in->cur += 4;
        return 0;
    }

    if (!*quoted && *p == '<' && *q == '>') /* Run <>NIL: ... */
        return convertRedir(ss, in, out);

    for (; *q != ss->ket && *q != ss->dollar && *q != '\0'; ++q)
    {
        if (!*quoted)
            switch (*q)
            {
            case '"':
            case ' ':
                if (*p == '<') /* input redirection */
                    return convertRedir(ss, in, out);

                bufferAppend(s, q - s, out, SysBase);
                return 0;
            }
    }

    for (i = 0; i < ss->argcount; ++i)
    {
        struct SArg *a = ss->args + i;
        STRPTR arg = NULL;
        IPTR val = ss->arg[i];
        UBYTE t = a->type;
        LONG j, len = a->namelen;

        if (q - s != len)
            continue;

        if (strncmp(s, a->name, len) != 0)
            continue;
        else
            scriptarg = TRUE;

        if (val)
        {
            if (t & SWITCH)
                arg = val ? a->name : NULL;
            else if (t & TOGGLE)
            {
                arg = val ? "1" : "0";
                len = 1;
            }
            else if (t & MULTIPLE)
            {
                STRPTR *m = (STRPTR *) val;

                for (j = 0; (arg = m[j]); ++j)
                {
                    if (j > 0)
                        bufferAppend(" ", 1, out, SysBase);

                    len = cliLen(arg);
                    bufferAppend(arg, len, out, SysBase);
                }
            }
            else
            {
                arg = (STRPTR) val;
                len = a->len;
            }
        }
        else if (s[len] == ss->dollar) /* default arg */
        {
            arg = s + len + 1;
            len = q - arg;
        }
        else
        {
            arg = (STRPTR) a->def;
            len = a->deflen;
        }

        if (arg)
            bufferAppend(arg, len, out, SysBase);
        break;
    }

    if(scriptarg)
    {
        while (*q != '\0' && *q++ != ss->ket);
        in->cur = q - in->buf;
    }
    else
        bufferCopy(in, out, 1, SysBase);

    return 0;
}
