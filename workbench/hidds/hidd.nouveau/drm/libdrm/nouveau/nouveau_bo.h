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

#ifndef __NOUVEAU_BO_H__
#define __NOUVEAU_BO_H__

/* Relocation/Buffer type flags */
#define NOUVEAU_BO_VRAM   (1 << 0)
#define NOUVEAU_BO_GART   (1 << 1)
#define NOUVEAU_BO_RD     (1 << 2)
#define NOUVEAU_BO_WR     (1 << 3)
#define NOUVEAU_BO_RDWR   (NOUVEAU_BO_RD | NOUVEAU_BO_WR)
#define NOUVEAU_BO_MAP    (1 << 4)
#define NOUVEAU_BO_LOW    (1 << 6)
#define NOUVEAU_BO_HIGH   (1 << 7)
#define NOUVEAU_BO_OR     (1 << 8)
#define NOUVEAU_BO_INVAL  (1 << 12)
#define NOUVEAU_BO_NOSYNC (1 << 13)
#define NOUVEAU_BO_NOWAIT (1 << 14)
#define NOUVEAU_BO_IFLUSH (1 << 15)
#define NOUVEAU_BO_DUMMY  (1 << 31)

#define NOUVEAU_BO_TILE_LAYOUT_MASK 0x0000ff00
#define NOUVEAU_BO_TILE_16BPP       0x00000001
#define NOUVEAU_BO_TILE_32BPP       0x00000002
#define NOUVEAU_BO_TILE_ZETA        0x00000004
#define NOUVEAU_BO_TILE_SCANOUT     0x00000008

struct nouveau_bo {
	struct nouveau_device *device;
	uint32_t handle;

	uint64_t size;
	void *map;

	uint32_t tile_mode;
	uint32_t tile_flags;
};

int
nouveau_bo_new(struct nouveau_device *, uint32_t flags, int align, int size,
	       struct nouveau_bo **);

int
nouveau_bo_new_tile(struct nouveau_device *, uint32_t flags, int align,
		    int size, uint32_t tile_mode, uint32_t tile_flags,
		    struct nouveau_bo **);

int
nouveau_bo_user(struct nouveau_device *, void *ptr, int size,
		struct nouveau_bo **);

int
nouveau_bo_wrap(struct nouveau_device *, uint32_t handle, struct nouveau_bo **);

int
nouveau_bo_handle_get(struct nouveau_bo *, uint32_t *);

int
nouveau_bo_handle_ref(struct nouveau_device *, uint32_t handle,
		      struct nouveau_bo **);

int
nouveau_bo_ref(struct nouveau_bo *, struct nouveau_bo **);

int
nouveau_bo_map_range(struct nouveau_bo *, uint32_t delta, uint32_t size,
		     uint32_t flags);

void
nouveau_bo_map_flush(struct nouveau_bo *, uint32_t delta, uint32_t size);

int
nouveau_bo_map(struct nouveau_bo *, uint32_t flags);

void
nouveau_bo_unmap(struct nouveau_bo *);

int
nouveau_bo_busy(struct nouveau_bo *, uint32_t access);

uint32_t
nouveau_bo_pending(struct nouveau_bo *);

#endif
