/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NV04_PUSHBUF_H__
#define __NV04_PUSHBUF_H__

#include "nouveau_pushbuf.h"

static __inline__ void
BEGIN_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr,
	   unsigned mthd, unsigned size)
{
	if (gr->bound == NOUVEAU_GROBJ_UNBOUND)
		nouveau_grobj_autobind(gr);
	chan->subc[gr->subc].sequence = chan->subc_sequence++;

	WAIT_RING(chan, size + 1);
	OUT_RING(chan, (gr->subc << 13) | (size << 18) | mthd);
}

/* non-incrementing BEGIN_RING */
static __inline__ void
BEGIN_RING_NI(struct nouveau_channel *chan, struct nouveau_grobj *gr,
	   unsigned mthd, unsigned size)
{
	BEGIN_RING(chan, gr, mthd | 0x40000000, size);
}

static __inline__ void
BIND_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr, unsigned sc)
{
	struct nouveau_subchannel *subc = &gr->channel->subc[sc];

	if (subc->gr) {
		if (subc->gr->bound == NOUVEAU_GROBJ_BOUND_EXPLICIT)
			assert(0);
		subc->gr->bound = NOUVEAU_GROBJ_UNBOUND;
	}
	subc->gr = gr;
	subc->gr->subc = sc;
	subc->gr->bound = NOUVEAU_GROBJ_BOUND_EXPLICIT;

	BEGIN_RING(chan, gr, 0x0000, 1);
	OUT_RING  (chan, gr->handle);
}

#endif
