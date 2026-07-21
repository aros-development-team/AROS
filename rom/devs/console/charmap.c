/*
    Copyright (C) 2010-2020, The AROS Development Team. All rights reserved.

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
                FreeMem(line->text, line->capacity * 4);
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
    char *text = line->text;
    BYTE *fgpen = line->fgpen;
    BYTE *bgpen = line->bgpen;
    BYTE *flags = line->flags;
    ULONG size = line->size;
    ULONG capacity = line->capacity;

    if (newsize && newsize <= capacity)
    {
        if (newsize > size)
        {
            SetMem(line->text + size, 0, newsize - size);
            SetMem(line->fgpen + size, 0, newsize - size);
            SetMem(line->bgpen + size, 0, newsize - size);
            SetMem(line->flags + size, 0, newsize - size);
        }
        line->size = newsize;
        return;
    }

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

        buffer = AllocMem(newcapacity * 4, MEMF_ANY);
        if (!buffer)
            return;

        line->text = (char *)buffer;
        line->fgpen = (BYTE *)(buffer + newcapacity);
        line->bgpen = (BYTE *)(buffer + newcapacity * 2);
        line->flags = (BYTE *)(buffer + newcapacity * 3);
        SetMem(line->text, 0, newsize);
        SetMem(line->fgpen, 0, newsize);
        SetMem(line->bgpen, 0, newsize);
        SetMem(line->flags, 0, newsize);
        line->size = newsize;
        line->capacity = newcapacity;
    }
    else
    {
        line->text = 0;
        line->fgpen = 0;
        line->bgpen = 0;
        line->flags = 0;
        line->size = 0;
        line->capacity = 0;
    }

    if (text && line->text)
        memcpy(line->text, text, size);
    if (fgpen && line->fgpen)
        memcpy(line->fgpen, fgpen, size);
    if (bgpen && line->bgpen)
        memcpy(line->bgpen, bgpen, size);
    if (flags && line->flags)
        memcpy(line->flags, flags, size);

    if (text)
        FreeMem(text, capacity * 4);
}
