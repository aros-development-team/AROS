/*
    Copyright (C) 2010 Alain Greppin <agreppin@chilibi.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

/* read 'size' chars from 'in' and append them to 'out' */
LONG bufferCopy(Buffer *in, Buffer *out, ULONG size);

void bufferFree(Buffer *b);

LONG bufferReadItem(STRPTR buf, ULONG size, Buffer *in);

void bufferReset(Buffer *b);

#endif
