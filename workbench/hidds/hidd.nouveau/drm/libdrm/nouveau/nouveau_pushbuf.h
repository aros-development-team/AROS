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

#ifndef __NOUVEAU_PUSHBUF_H__
#define __NOUVEAU_PUSHBUF_H__

#include <assert.h>
#include <string.h>

#include "nouveau_bo.h"
#include "nouveau_grobj.h"

int
nouveau_pushbuf_flush(struct nouveau_channel *, unsigned min);

int
nouveau_pushbuf_marker_emit(struct nouveau_channel *chan,
			    unsigned wait_dwords, unsigned wait_relocs);

void
nouveau_pushbuf_marker_undo(struct nouveau_channel *chan);

int
nouveau_pushbuf_emit_reloc(struct nouveau_channel *, void *ptr,
			   struct nouveau_bo *, uint32_t data, uint32_t data2,
			   uint32_t flags, uint32_t vor, uint32_t tor);

int
nouveau_pushbuf_submit(struct nouveau_channel *chan, struct nouveau_bo *bo,
		       unsigned offset, unsigned length);

/* Push buffer access macros */
static __inline__ int
MARK_RING(struct nouveau_channel *chan, unsigned dwords, unsigned relocs)
{
	return nouveau_pushbuf_marker_emit(chan, dwords, relocs);
}

static __inline__ void
MARK_UNDO(struct nouveau_channel *chan)
{
	nouveau_pushbuf_marker_undo(chan);
}

static __inline__ void
OUT_RING(struct nouveau_channel *chan, unsigned data)
{
	*(chan->cur++) = (data);
}

static __inline__ void
OUT_RINGp(struct nouveau_channel *chan, const void *data, unsigned size)
{
	memcpy(chan->cur, data, size * 4);
	chan->cur += size;
}

static __inline__ void
OUT_RINGf(struct nouveau_channel *chan, float f)
{
	union { uint32_t i; float f; } c;
	c.f = f;
	OUT_RING(chan, c.i);
}

static __inline__ unsigned
AVAIL_RING(struct nouveau_channel *chan)
{
	return chan->end - chan->cur;
}

static __inline__ void
WAIT_RING(struct nouveau_channel *chan, unsigned size)
{
	if (chan->cur + size > chan->end)
		nouveau_pushbuf_flush(chan, size);
}

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
FIRE_RING(struct nouveau_channel *chan)
{
	nouveau_pushbuf_flush(chan, 0);
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

static __inline__ int
OUT_RELOC(struct nouveau_channel *chan, struct nouveau_bo *bo,
	  unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
	return nouveau_pushbuf_emit_reloc(chan, chan->cur++, bo,
					  data, 0, flags, vor, tor);
}

static __inline__ int
OUT_RELOC2(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned data, unsigned data2, unsigned flags,
	   unsigned vor, unsigned tor)
{
	return nouveau_pushbuf_emit_reloc(chan, chan->cur++, bo,
					  data, data2, flags, vor, tor);
}

/* Raw data + flags depending on FB/TT buffer */
static __inline__ int
OUT_RELOCd(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
	return OUT_RELOC(chan, bo, data, flags | NOUVEAU_BO_OR, vor, tor);
}

/* FB/TT object handle */
static __inline__ int
OUT_RELOCo(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned flags)
{
	return OUT_RELOC(chan, bo, 0, flags | NOUVEAU_BO_OR,
			 chan->vram->handle, chan->gart->handle);
}

/* Low 32-bits of offset */
static __inline__ int
OUT_RELOCl(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned delta, unsigned flags)
{
	return OUT_RELOC(chan, bo, delta, flags | NOUVEAU_BO_LOW, 0, 0);
}

/* Low 32-bits of offset + GPU linear access range info */
static __inline__ int
OUT_RELOCr(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned delta, unsigned size, unsigned flags)
{
	return OUT_RELOC2(chan, bo, delta, size, flags | NOUVEAU_BO_LOW, 0, 0);
}

/* High 32-bits of offset */
static __inline__ int
OUT_RELOCh(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned delta, unsigned flags)
{
	return OUT_RELOC(chan, bo, delta, flags | NOUVEAU_BO_HIGH, 0, 0);
}

#endif
