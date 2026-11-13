/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifndef CHARMAP_H
#define CHARMAP_H

#include <stddef.h>

#include <exec/types.h>

// FIXME:
// This wastes a lot of memory - most characters are likely to share the
// same fg/bg/flags. One option would be to optimize away the fgpen/bgpen/
// flags buffers if they're the same for every character in the line
/* Number of per-cell arrays of a line: text, fgpen, bgpen and flags */
#define CHARMAP_PLANES 4

struct charmap_line
{
    // FIXME: Replace with a MinNode?
    struct charmap_line *next;
    struct charmap_line *prev;

    UWORD size;
    UWORD capacity;

    /* The per-cell arrays, by name and as plane[] for code that treats all
       of them alike. They share one allocation of capacity bytes each, in
       the order of plane[]. */
    union
    {
        struct
        {
            char *text;
            BYTE *fgpen;
            BYTE *bgpen;
            BYTE *flags;
        };
        UBYTE *plane[CHARMAP_PLANES];
    };
};

_Static_assert(offsetof(struct charmap_line, text) ==
        offsetof(struct charmap_line, plane[0])
    && offsetof(struct charmap_line, fgpen) ==
        offsetof(struct charmap_line, plane[1])
    && offsetof(struct charmap_line, bgpen) ==
        offsetof(struct charmap_line, plane[2])
    && offsetof(struct charmap_line, flags) ==
        offsetof(struct charmap_line, plane[3])
    && sizeof(((struct charmap_line *)0)->plane) == 4 * sizeof(UBYTE *),
    "plane[] must cover text, fgpen, bgpen and flags");

VOID charmap_dispose_lines(struct charmap_line *line);
struct charmap_line *charmap_dispose_line(struct charmap_line *line);
struct charmap_line *charmap_newline(struct charmap_line *next,
    struct charmap_line *prev);
VOID charmap_resize(struct ConsoleBase *ConsoleDevice, struct charmap_line *line, ULONG newsize);

#endif
