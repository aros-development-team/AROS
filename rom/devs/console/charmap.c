/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    Desc: Code for CONU_CHARMAP console units.
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include <string.h>

#include "console_gcc.h"
#include "charmap.h"

struct charmap_line *charmap_dispose_line(struct charmap_line *line)
{
    struct charmap_line *next = NULL;
    if (line)
    {
        next = line->next;
        if (line->capacity)
        {
            if (line->text)
                FreeMem(line->text, line->capacity * CHARMAP_PLANES);
        }
        FreeMem(line, sizeof(struct charmap_line));
    }
    return next;
}

VOID charmap_dispose_lines(struct charmap_line *line)
{
    while ((line = charmap_dispose_line(line)));
}

struct charmap_line *charmap_newline(struct charmap_line *next,
    struct charmap_line *prev)
{
    struct charmap_line *newline =
        (struct charmap_line *)AllocMem(sizeof(struct charmap_line),
        MEMF_ANY);
    if (!newline)
        return NULL;

    newline->next = next;
    newline->prev = prev;
    if (next)
        next->prev = newline;
    if (prev)
        prev->next = newline;
    newline->text = 0;
    newline->fgpen = 0;
    newline->bgpen = 0;
    newline->flags = 0;
    newline->size = 0;
    newline->capacity = 0;
    return newline;
}


VOID charmap_resize(struct ConsoleBase *ConsoleDevice, struct charmap_line *line, ULONG newsize)
{
    UBYTE *old[CHARMAP_PLANES];
    ULONG size = line->size;
    ULONG capacity = line->capacity;
    LONG i;

    if (newsize && newsize <= capacity)
    {
        if (newsize > size)
        {
            for (i = 0; i < CHARMAP_PLANES; i++)
                SetMem(line->plane[i] + size, 0, newsize - size);
        }
        line->size = newsize;
        return;
    }

    for (i = 0; i < CHARMAP_PLANES; i++)
        old[i] = line->plane[i];

    if (newsize)
    {
        ULONG growth = capacity / 2;
        ULONG newcapacity;
        UBYTE *buffer;

        if (newsize > 0xffff)
            return;
        if (growth < 8)
            growth = 8;
        newcapacity = capacity + growth;
        if (newcapacity < newsize)
            newcapacity = newsize;
        if (newcapacity > 0xffff)
            newcapacity = 0xffff;

        buffer = AllocMem(newcapacity * CHARMAP_PLANES, MEMF_ANY);
        if (!buffer)
            return;

        /* The arrays follow each other in the one allocation */
        for (i = 0; i < CHARMAP_PLANES; i++)
        {
            line->plane[i] = buffer;
            SetMem(buffer, 0, newsize);
            buffer += newcapacity;
        }
        line->size = newsize;
        line->capacity = newcapacity;
    }
    else
    {
        for (i = 0; i < CHARMAP_PLANES; i++)
            line->plane[i] = 0;
        line->size = 0;
        line->capacity = 0;
    }

    for (i = 0; i < CHARMAP_PLANES; i++)
    {
        if (old[i] && line->plane[i])
            memcpy(line->plane[i], old[i], size);
    }

    /* text is the start of the old allocation */
    if (old[0])
        FreeMem(old[0], capacity * CHARMAP_PLANES);
}
