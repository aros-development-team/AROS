/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#ifndef BUFFER_H
#define BUFFER_H 1

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

typedef struct {
    STRPTR buf;
    ULONG  len; /* write position */
    ULONG  cur; /* read position */
    ULONG  mem; /* allocated memory */
} Buffer;

LONG bufferAppend(STRPTR str, ULONG size, Buffer *out);
LONG bufferInsert(STRPTR str, ULONG size, Buffer *out);

/* read 'size' chars from 'in' and append them to 'out' */
LONG bufferCopy(Buffer *in, Buffer *out, ULONG size);

void bufferFree(Buffer *b);

LONG bufferReadItem(STRPTR buf, ULONG size, Buffer *in, APTR DOSBase);

void bufferReset(Buffer *b);

#endif
