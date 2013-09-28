/*
 * Copyright 2010 Red Hat Inc.
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
 * Authors: Ben Skeggs
 */

#include "drmP.h"
#include "nouveau_drv.h"
#include "nouveau_mm_renamed.h"

/* 0 = unsupported
 * 1 = non-compressed
 * 3 = compressed
 */
static const u8 types[256] = {
	1, 1, 3, 3, 3, 3, 0, 3, 3, 3, 3, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3,
	3, 3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0, 1, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 3, 3, 3, 3, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3,
	3, 3, 0, 0, 0, 0, 0, 0, 3, 0, 0, 3, 0, 3, 0, 3,
	3, 0, 3, 3, 3, 3, 3, 0, 0, 3, 0, 3, 0, 3, 3, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 1, 1, 0
};

bool
nvc0_vram_flags_valid(struct drm_device *dev, u32 tile_flags)
{
	u8 memtype = (tile_flags & NOUVEAU_GEM_TILE_LAYOUT_MASK) >> 8;
	return likely((types[memtype] == 1));
}

int
nvc0_vram_new(struct drm_device *dev, u64 size, u32 align, u32 ncmin,
	      u32 type, struct nouveau_mem **pmem)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_mm *mm = &dev_priv->engine.vram.mm;
	struct nouveau_mm_node *r;
	struct nouveau_mem *mem;
	int ret;

	size  >>= 12;
	align >>= 12;
	ncmin >>= 12;

#ifdef __AROS__
    /* Workaround which forces allocation to be contiguous in VRAM.
       Without this some gfx cards cause display corruption, because
       displaying of non contiguous bitmap does not work for whatever
       reason. */

    if (ncmin < size) ncmin = size;
#endif
	mem = kzalloc(sizeof(*mem), GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	INIT_LIST_HEAD(&mem->regions);
	mem->dev = dev_priv->dev;
	mem->memtype = (type & 0xff);
	mem->size = size;

	mutex_lock(&mm->mutex);
	do {
		ret = nouveau_mm_get(mm, 1, size, ncmin, align, &r);
		if (ret) {
			mutex_unlock(&mm->mutex);
			nv50_vram_del(dev, &mem);
			return ret;
		}

		list_add_tail(&r->rl_entry, &mem->regions);
		size -= r->length;
	} while (size);
	mutex_unlock(&mm->mutex);

	r = list_first_entry(&mem->regions, struct nouveau_mm_node, rl_entry);
	mem->offset = (u64)r->offset << 12;
	*pmem = mem;
	return 0;
}

int
nvc0_vram_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_vram_engine *vram = &dev_priv->engine.vram;
	const u32 rsvd_head = ( 256 * 1024) >> 12; /* vga memory */
	const u32 rsvd_tail = (1024 * 1024) >> 12; /* vbios etc */
	u32 parts = nv_rd32(dev, 0x121c74);
	u32 bsize = nv_rd32(dev, 0x10f20c);
	u32 offset, length;
	bool uniform = true;
	int ret, i;

	NV_DEBUG(dev, "0x100800: 0x%08x\n", nv_rd32(dev, 0x100800));
	NV_DEBUG(dev, "parts 0x%08x bcast_mem_amount 0x%08x\n", parts, bsize);

	/* read amount of vram attached to each memory controller */
	for (i = 0; i < parts; i++) {
		u32 psize = nv_rd32(dev, 0x11020c + (i * 0x1000));
		if (psize != bsize) {
			if (psize < bsize)
				bsize = psize;
			uniform = false;
		}

		NV_DEBUG(dev, "%d: mem_amount 0x%08x\n", i, psize);

		dev_priv->vram_size += (u64)psize << 20;
	}

	/* if all controllers have the same amount attached, there's no holes */
	if (uniform) {
		offset = rsvd_head;
		length = (dev_priv->vram_size >> 12) - rsvd_head - rsvd_tail;
		return nouveau_mm_init(&vram->mm, offset, length, 1);
	}

	/* otherwise, address lowest common amount from 0GiB */
	ret = nouveau_mm_init(&vram->mm, rsvd_head, (bsize << 8) * parts, 1);
	if (ret)
		return ret;

	/* and the rest starting from (8GiB + common_size) */
	offset = (0x0200000000ULL >> 12) + (bsize << 8);
	length = (dev_priv->vram_size >> 12) - (bsize << 8) - rsvd_tail;

	ret = nouveau_mm_init(&vram->mm, offset, length, 0);
	if (ret) {
		nouveau_mm_fini(&vram->mm);
		return ret;
	}

	return 0;
}
