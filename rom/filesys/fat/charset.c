/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "fat_fs.h"
#include "charset.h"

#undef DOSBase

static ULONG readLine(struct Library *DOSBase, BPTR fh, char *buf,
    ULONG size)
{
    char *c;

    if ((c = FGets(fh, buf, size)) == NULL)
        return FALSE;

    for (; *c; c++)
    {
        if (*c == '\n' || *c == '\r')
        {
            *c = '\0';
            break;
        }
    }

    return TRUE;
}

void InitCharsetTables(struct Globals *glob)
{
    int i;

    for (i = 0; i < 65536; i++)
        if (i < 256)
        {
            glob->from_unicode[i] = i;
            glob->to_unicode[i] = i;
        }
        else
            glob->from_unicode[i] = '_';
}

// Reads a coding table
BOOL ReadUnicodeTable(struct Globals *glob, STRPTR name)
{
    BPTR fh;
    struct Library *DOSBase;

    if (!(DOSBase = TaggedOpenLibrary(TAGGEDOPEN_DOS)))
        return FALSE;

    fh = Open(name, MODE_OLDFILE);
    if (fh)
    {
        int i, n;
        char buf[512];

        while (readLine(DOSBase, fh, buf, 512 * sizeof(char)))
        {
            if (!isdigit(*buf))
                continue;
            else
            {
                char *p = buf;
                int fmt2 = 0;

                if ((*p == '=') || (fmt2 = ((*p == '0')
                    || (*(p + 1) == 'x'))))
                {
                    p++;
                    p += fmt2;

                    i = strtol((const char *)p, (char **)&p, 16);
                    if (i >= 0 && i < 256)
                    {
                        while (isspace(*p))
                            p++;

                        if (!strnicmp(p, "U+", 2))
                        {
                            p += 2;
                            n = strtol((const char *)p, (char **)&p, 16);
                        }
                        else
                        {
                            if (*p != '#')
                                n = strtol((const char *)p, (char **)&p, 0);
                            else
                                n = -1;
                        }
                        if (n >= 0 && n < 65536)
                        {
                            glob->from_unicode[n] = i;
                            glob->to_unicode[i] = n;
                        }
                    }
                }
            }
        }
        Close(fh);
    }

    CloseLibrary(DOSBase);

    return fh ? TRUE : FALSE;
}
