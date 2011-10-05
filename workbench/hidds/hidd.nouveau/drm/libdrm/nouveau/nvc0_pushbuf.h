/*
 * Copyright 2010 Nouveau Project
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

#ifndef __NVC0_PUSHBUF_H__
#define __NVC0_PUSHBUF_H__

#include "nouveau_pushbuf.h"

#define SUBC_BIND(chan, gr) do {                                               \
	if (gr->bound == NOUVEAU_GROBJ_UNBOUND)                                \
		nouveau_grobj_autobind(gr);                                    \
	chan->subc[gr->subc].sequence = chan->subc_sequence++;                 \
} while (0)

/* incremental methods */
static __inline__ void
BEGIN_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr,
	   unsigned mthd, unsigned size)
{
	SUBC_BIND(chan, gr);
	WAIT_RING(chan, size + 1);
	OUT_RING (chan, (0x2 << 28) | (size << 16) | (gr->subc << 13) | (mthd >> 2));
}

/* non-incremental */
static __inline__ void
BEGIN_RING_NI(struct nouveau_channel *chan, struct nouveau_grobj *gr,
	      unsigned mthd, unsigned size)
{
	SUBC_BIND(chan, gr);
	WAIT_RING(chan, size + 1);
	OUT_RING (chan, (0x6 << 28) | (size << 16) | (gr->subc << 13) | (mthd >> 2));
}

/* increment-once */
static __inline__ void
BEGIN_RING_1I(struct nouveau_channel *chan, struct nouveau_grobj *gr,
	      unsigned mthd, unsigned size)
{
	SUBC_BIND(chan, gr);
	WAIT_RING(chan, size + 1);
	OUT_RING (chan, (0xa << 28) | (size << 16) | (gr->subc << 13) | (mthd >> 2));
}

/* inline-data */
static __inline__ void
IMMED_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr,
	   unsigned mthd, unsigned data)
{
	SUBC_BIND(chan, gr);
	WAIT_RING(chan, 1);
	OUT_RING (chan, (0x8 << 28) | (data << 16) | (gr->subc << 13) | (mthd >> 2));
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
	OUT_RING  (chan, gr->grclass);
}

#endif
