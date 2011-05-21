/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "buffer.h"

#define BUF_SIZE 512

LONG bufferAppend(STRPTR str, ULONG size, Buffer *out)
{
    ULONG newLength = out->len + size;

    if (newLength > out->mem)
    {
	ULONG newSize = BUF_SIZE;
	STRPTR tmp;

	while (newSize < newLength)
	    newSize += BUF_SIZE;

	if ((tmp = AllocMem(newSize + 1, MEMF_ANY)) == NULL)
	    return ERROR_NO_FREE_STORE;

	if (out->len > 0)
	    CopyMem(out->buf, tmp, out->len);

	if (out->mem > 0)
	    FreeMem(out->buf, out->mem);

	out->buf = tmp;
	out->mem = newSize;
    }

    CopyMem(str, out->buf + out->len, size);
    out->len = newLength;
    out->buf[newLength] = '\0';

    return 0;
}

LONG bufferCopy(Buffer *in, Buffer *out, ULONG size)
{
    STRPTR s = in->buf + in->cur;
    LONG ret = bufferAppend(s, size, out);
    in->cur += size;
    return ret;
}

void bufferFree(Buffer *b)
{
    if (b->mem <= 0)
	return;

    FreeMem(b->buf, b->mem + 1);

    b->buf = NULL;
    b->len = 0;
    b->cur = 0;
    b->mem = 0;
}

LONG bufferReadItem(STRPTR buf, ULONG size, Buffer *in)
{
    struct CSource tin = { in->buf, in->len, in->cur };
    LONG ret = ReadItem(buf, size, &tin);
    in->cur = tin.CS_CurChr;
    return ret;
}

void bufferReset(Buffer *b)
{
    if (b->mem > 0)
	b->buf[0] = '\0';

    b->len = 0;
    b->cur = 0;
}
