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

#ifndef __NOUVEAU_LOCAL_H__
#define __NOUVEAU_LOCAL_H__

#if !defined(__AROS__)
#include "compiler.h"
#include "xf86_OSproc.h"
#endif

#include <nouveau.h>

/* Debug output */
#define NOUVEAU_MSG(fmt,args...) ErrorF(fmt, ##args)
#define NOUVEAU_ERR(fmt,args...) \
	ErrorF("%s:%d - "fmt, __func__, __LINE__, ##args)
#if 0
#define NOUVEAU_FALLBACK(fmt,args...) do {    \
	NOUVEAU_ERR("FALLBACK: "fmt, ##args); \
	return FALSE;                         \
} while(0)
#else
#define NOUVEAU_FALLBACK(fmt,args...) do {    \
	return FALSE;                         \
} while(0)
#endif

#if defined(__AROS__)
#include <proto/dos.h>
#include <aros/debug.h>
#define NOT_IMPLEMENTED_STOP            { bug("NOT IMPLEMENTED STOP %s, %d\n", __func__, __LINE__); while(1){Delay(1);}; }
#define NOT_IMPLEMENTED_CONTINUE        { bug("NOT IMPLEMENTED %s, %d\n", __func__, __LINE__); }
#endif

#define NOUVEAU_ALIGN(x,bytes) (((x) + ((bytes) - 1)) & ~((bytes) - 1))

#define NV50_TILE_PITCH(m) (64 << ((m) & 0xf))
#define NV50_TILE_HEIGHT(m) (4 << ((m) >> 4))
#define NVC0_TILE_PITCH(m) (64 << ((m) & 0xf))
#define NVC0_TILE_HEIGHT(m) (8 << ((m) >> 4))

static inline int log2i(int i)
{
	int r = 0;

	if (i & 0xffff0000) {
		i >>= 16;
		r += 16;
	}
	if (i & 0x0000ff00) {
		i >>= 8;
		r += 8;
	}
	if (i & 0x000000f0) {
		i >>= 4;
		r += 4;
	}
	if (i & 0x0000000c) {
		i >>= 2;
		r += 2;
	}
	if (i & 0x00000002) {
		r += 1;
	}
	return r;
}

static inline int round_down_pow2(int x)
{
	return 1 << log2i(x);
}

static inline int round_up_pow2(int x)
{
   int r = round_down_pow2(x);
   if (r < x)
      r <<= 1;
   return r;
}

#define SWAP(x, y) do {			\
		typeof(x) __z = (x);	\
		(x) = (y);		\
		(y) = __z;		\
	} while (0)

static inline uint32_t
PUSH_AVAIL(struct nouveau_pushbuf *push)
{
	return push->end - push->cur;
}

static inline Bool
PUSH_SPACE(struct nouveau_pushbuf *push, uint32_t size)
{
	if (PUSH_AVAIL(push) < size)
		return nouveau_pushbuf_space(push, size, 0, 0) == 0;
	return TRUE;
}

static inline void
PUSH_DATA(struct nouveau_pushbuf *push, uint32_t data)
{
	*push->cur++ = data;
}

static inline void
PUSH_DATAp(struct nouveau_pushbuf *push, const void *data, uint32_t size)
{
	memcpy(push->cur, data, size * 4);
	push->cur += size;
}

static inline void
PUSH_RELOC(struct nouveau_pushbuf *push, struct nouveau_bo *bo, uint32_t offset,
	   uint32_t flags, uint32_t vor, uint32_t tor)
{
	nouveau_pushbuf_reloc(push, bo, offset, flags, vor, tor);
}

static inline void
PUSH_KICK(struct nouveau_pushbuf *push)
{
	nouveau_pushbuf_kick(push, push->channel);
}

static inline struct nouveau_bufctx *
BUFCTX(struct nouveau_pushbuf *push)
{
	return push->user_priv;
}

static inline void
PUSH_RESET(struct nouveau_pushbuf *push)
{
	nouveau_bufctx_reset(BUFCTX(push), 0);
}

static inline void
PUSH_REFN(struct nouveau_pushbuf *push, struct nouveau_bo *bo, uint32_t access)
{
	nouveau_bufctx_refn(BUFCTX(push), 0, bo, access);
}

static inline void
PUSH_MTHDl(struct nouveau_pushbuf *push, int subc, int mthd,
	   struct nouveau_bo *bo, uint32_t offset, uint32_t access)
{
	nouveau_bufctx_mthd(BUFCTX(push), 0, (1 << 18) | (subc << 13) | mthd,
			    bo, offset, access | NOUVEAU_BO_LOW, 0, 0);
	PUSH_DATA (push, bo->offset + offset);
}

static inline void
PUSH_MTHDo(struct nouveau_pushbuf *push, int subc, int mthd,
	   struct nouveau_bo *bo, uint32_t access, uint32_t vor, uint32_t tor)
{
	nouveau_bufctx_mthd(BUFCTX(push), 0, (1 << 18) | (subc << 13) | mthd,
			    bo, 0, access | NOUVEAU_BO_OR, vor, tor);
	if (bo->flags & NOUVEAU_BO_VRAM)
		PUSH_DATA (push, vor);
	else
		PUSH_DATA (push, tor);
}

static inline void
PUSH_MTHDs(struct nouveau_pushbuf *push, int subc, int mthd,
	   struct nouveau_bo *bo, uint32_t data, uint32_t access,
	   uint32_t vor, uint32_t tor)
{
	nouveau_bufctx_mthd(BUFCTX(push), 0, (1 << 18) | (subc << 13) | mthd,
			    bo, data, access | NOUVEAU_BO_OR, vor, tor);
	if (bo->flags & NOUVEAU_BO_VRAM)
		PUSH_DATA (push, data | vor);
	else
		PUSH_DATA (push, data | tor);
}

static inline void
PUSH_MTHD(struct nouveau_pushbuf *push, int subc, int mthd,
	  struct nouveau_bo *bo, uint32_t data, uint32_t access,
	  uint32_t vor, uint32_t tor)
{
	nouveau_bufctx_mthd(BUFCTX(push), 0, (1 << 18) | (subc << 13) | mthd,
			    bo, data, access | NOUVEAU_BO_OR, vor, tor);
	if (access & NOUVEAU_BO_LOW)
		data += bo->offset;
	if (access & NOUVEAU_BO_OR) {
		if (bo->flags & NOUVEAU_BO_VRAM)
			data |= vor;
		else
			data |= tor;
	}
	PUSH_DATA (push, data);
}

static inline void
PUSH_DATAf(struct nouveau_pushbuf *push, float v)
{
	union { float f; uint32_t i; } d = { .f = v };
	PUSH_DATA (push, d.i);
}

static inline void
BEGIN_NV04(struct nouveau_pushbuf *push, int subc, int mthd, int size)
{
	PUSH_DATA (push, 0x00000000 | (size << 18) | (subc << 13) | mthd);
}

static inline void
BEGIN_NI04(struct nouveau_pushbuf *push, int subc, int mthd, int size)
{
	PUSH_DATA (push, 0x40000000 | (size << 18) | (subc << 13) | mthd);
}

static inline void
BEGIN_NVC0(struct nouveau_pushbuf *push, int subc, int mthd, int size)
{
	PUSH_DATA (push, 0x20000000 | (size << 16) | (subc << 13) | (mthd / 4));
}

static inline void
BEGIN_NIC0(struct nouveau_pushbuf *push, int subc, int mthd, int size)
{
	PUSH_DATA (push, 0x60000000 | (size << 16) | (subc << 13) | (mthd / 4));
}

static inline void
IMMED_NVC0(struct nouveau_pushbuf *push, int subc, int mthd, int data)
{
	PUSH_DATA (push, 0x80000000 | (data << 16) | (subc << 13) | (mthd / 4));
}

static inline void
BEGIN_1IC0(struct nouveau_pushbuf *push, int subc, int mthd, int size)
{
	PUSH_DATA (push, 0xa0000000 | (size << 16) | (subc << 13) | (mthd / 4));
}

#define NV01_SUBC(subc, mthd) SUBC_##subc((NV01_SUBCHAN_##mthd))
#define NV11_SUBC(subc, mthd) SUBC_##subc((NV11_SUBCHAN_##mthd))
#define NV84_SUBC(subc, mthd) SUBC_##subc((NV84_SUBCHAN_##mthd))

#define NV04_GRAPH(subc, mthd) SUBC_##subc((NV04_GRAPH_##mthd))
#define NV50_GRAPH(subc, mthd) SUBC_##subc((NV50_GRAPH_##mthd))
#define NVC0_GRAPH(subc, mthd) SUBC_##subc((NVC0_GRAPH_##mthd))

#endif
