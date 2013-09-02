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

#ifndef __NOUVEAU_CHANNEL_H__
#define __NOUVEAU_CHANNEL_H__

struct nouveau_subchannel {
	struct nouveau_grobj *gr;
	unsigned sequence;
};

struct nouveau_channel {
	uint32_t *cur;
	uint32_t *end;

	struct nouveau_device *device;
	int id;

	struct nouveau_grobj *nullobj;
	struct nouveau_grobj *vram;
	struct nouveau_grobj *gart;

	void *user_private;
	void (*hang_notify)(struct nouveau_channel *);
	void (*flush_notify)(struct nouveau_channel *);

	struct nouveau_subchannel subc[8];
	unsigned subc_sequence;
};

int
nouveau_channel_alloc(struct nouveau_device *, uint32_t fb, uint32_t tt,
		      int pushbuf_size, struct nouveau_channel **);

void
nouveau_channel_free(struct nouveau_channel **);

#endif
