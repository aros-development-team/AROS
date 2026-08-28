/*
 * Copyright 2014 Red Hat Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs <bskeggs@redhat.com>
 */

#include "nouveau_copy.h"

#include "hwdefs/nv_object.xml.h"
#include "nvc0_accel.h"

Bool
nouveau_copya0b5_rect(struct nouveau_pushbuf *push, struct nouveau_object *copy,
		      int w, int h, int cpp,
		      struct nouveau_bo *src, uint32_t src_off, int src_dom,
		      int src_pitch, int src_h, int src_x, int src_y,
		      struct nouveau_bo *dst, uint32_t dst_off, int dst_dom,
		      int dst_pitch, int dst_h, int dst_x, int dst_y)
{
	struct nouveau_pushbuf_refn refs[] = {
		{ src, src_dom | NOUVEAU_BO_RD },
		{ dst, dst_dom | NOUVEAU_BO_WR },
	};
	unsigned exec;

	if (nouveau_pushbuf_space(push, 64, 0, 0) ||
	    nouveau_pushbuf_refn (push, refs, 2))
		return FALSE;

	exec = 0x00000206;
	if (!src->config.nvc0.memtype) {
		src_off += src_y * src_pitch + src_x * cpp;
		exec |= 0x00000080;
	}
	if (!dst->config.nvc0.memtype) {
		dst_off += dst_y * dst_pitch + dst_x * cpp;
		exec |= 0x00000100;
	}

	BEGIN_NVC0(push, SUBC_COPY(0x0728), 6);
	PUSH_DATA (push, 0x00001000 | src->config.nvc0.tile_mode);
	PUSH_DATA (push, src_pitch);
	PUSH_DATA (push, src_h);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, (src_y << 16) | src_x * cpp);
	BEGIN_NVC0(push, SUBC_COPY(0x070c), 6);
	PUSH_DATA (push, 0x000001000 | dst->config.nvc0.tile_mode);
	PUSH_DATA (push, dst_pitch);
	PUSH_DATA (push, dst_h);
	PUSH_DATA (push, 1);
	PUSH_DATA (push, 0);
	PUSH_DATA (push, (dst_y << 16) | dst_x * cpp);
	BEGIN_NVC0(push, SUBC_COPY(0x0400), 8);
	PUSH_DATA (push, (src->offset + src_off) >> 32);
	PUSH_DATA (push, (src->offset + src_off));
	PUSH_DATA (push, (dst->offset + dst_off) >> 32);
	PUSH_DATA (push, (dst->offset + dst_off));
	PUSH_DATA (push, src_pitch);
	PUSH_DATA (push, dst_pitch);
	PUSH_DATA (push, w * cpp);
	PUSH_DATA (push, h);
	BEGIN_NVC0(push, SUBC_COPY(0x0300), 1);
	PUSH_DATA (push, exec);
	return TRUE;
}

Bool
nouveau_copya0b5_init(NVPtr pNv)
{
	struct nouveau_pushbuf *push = pNv->ce_pushbuf;
	if (PUSH_SPACE(push, 8)) {
		BEGIN_NVC0(push, NV01_SUBC(COPY, OBJECT), 1);
		PUSH_DATA (push, pNv->NvCopy->handle);
		pNv->ce_rect = nouveau_copya0b5_rect;
		return TRUE;
	}
	return FALSE;
}
