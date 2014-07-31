/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef CHARMAP_H
#define CHARMAP_H

#include <exec/types.h>

// FIXME:
// This wastes a lot of memory - most characters are likely to share the
// same fg/bg/flags. One option would be to optimize away the fgpen/bgpen/
// flags buffers if they're the same for every character in the line
struct charmap_line
{
    // FIXME: Replace with a MinNode?
    struct charmap_line *next;
    struct charmap_line *prev;

    ULONG size;
    char *text;
    BYTE *fgpen;
    BYTE *bgpen;
    BYTE *flags;
};

VOID charmap_dispose_lines(struct charmap_line *line);
struct charmap_line *charmap_dispose_line(struct charmap_line *line);
struct charmap_line *charmap_newline(struct charmap_line *next,
    struct charmap_line *prev);
VOID charmap_resize(struct charmap_line *line, ULONG newsize);

#endif
