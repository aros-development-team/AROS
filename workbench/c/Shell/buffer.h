/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.
 */

#ifndef BUFFER_H
#define BUFFER_H 1

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#include "state.h"

typedef struct {
    STRPTR buf;
    ULONG  len; /* write position */
    ULONG  cur; /* read position */
    ULONG  mem; /* allocated memory */
} Buffer;

LONG bufferAppend(STRPTR str, ULONG size, Buffer *out, ShellState *ss);
LONG bufferInsert(STRPTR str, ULONG size, Buffer *out, ShellState *ss);

/* read 'size' chars from 'in' and append them to 'out' */
LONG bufferCopy(Buffer *in, Buffer *out, ULONG size, ShellState *ss);

void bufferFree(Buffer *b, ShellState *ss);

LONG bufferReadItem(STRPTR buf, ULONG size, Buffer *in, ShellState *ss);

void bufferReset(Buffer *b);

#endif
