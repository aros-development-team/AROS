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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "nouveau_private.h"

static uint32_t
nouveau_reloc_calc(struct drm_nouveau_gem_pushbuf_bo *pbbo,
		   struct drm_nouveau_gem_pushbuf_reloc *r)
{
	uint32_t push = 0;

	if (r->flags & NOUVEAU_GEM_RELOC_LOW)
		push = (pbbo->presumed.offset + r->data);
	else
	if (r->flags & NOUVEAU_GEM_RELOC_HIGH)
		push = (pbbo->presumed.offset + r->data) >> 32;
	else
		push = r->data;

	if (r->flags & NOUVEAU_GEM_RELOC_OR) {
		if (pbbo->presumed.domain & NOUVEAU_GEM_DOMAIN_VRAM)
			push |= r->vor;
		else
			push |= r->tor;
	}

	return push;
}

int
nouveau_reloc_emit(struct nouveau_channel *chan, struct nouveau_bo *reloc_bo,
		   uint32_t reloc_offset, uint32_t *reloc_ptr,
		   struct nouveau_bo *bo, uint32_t data, uint32_t data2,
		   uint32_t flags, uint32_t vor, uint32_t tor)
{
	struct nouveau_pushbuf_priv *nvpb = &nouveau_channel(chan)->pb;
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct drm_nouveau_gem_pushbuf_reloc *r;
	struct drm_nouveau_gem_pushbuf_bo *pbbo, *rpbbo;
	uint32_t domains = 0;

	if (nvpb->nr_relocs >= NOUVEAU_GEM_MAX_RELOCS) {
		fprintf(stderr, "too many relocs!!\n");
		return -ENOMEM;
	}

	if (nvbo->user && (flags & NOUVEAU_BO_WR)) {
		fprintf(stderr, "write to user buffer!!\n");
		return -EINVAL;
	}

	/* We're about to reloc a user buffer, better make sure we don't cause
	 * a double migration.
	 */
	if (!(nvbo->flags & (NOUVEAU_BO_GART | NOUVEAU_BO_VRAM)))
		nvbo->flags |= (flags & (NOUVEAU_BO_GART | NOUVEAU_BO_VRAM));

	/* add buffer to validation list */
	pbbo = nouveau_bo_emit_buffer(chan, bo);
	if (!pbbo) {
		fprintf(stderr, "buffer emit fail :(\n");
		return -ENOMEM;
	}
	nouveau_bo(bo)->pending_refcnt++;

	if (flags & (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART)) {
		if (flags & NOUVEAU_BO_VRAM)
			domains |= NOUVEAU_GEM_DOMAIN_VRAM;
		if (flags & NOUVEAU_BO_GART)
			domains |= NOUVEAU_GEM_DOMAIN_GART;
	} else
		domains |= nvbo->domain;

	if (!(pbbo->valid_domains & domains)) {
		fprintf(stderr, "no valid domains remain!\n");
		return -EINVAL;
	}
	pbbo->valid_domains &= domains;

	assert(flags & NOUVEAU_BO_RDWR);
	if (flags & NOUVEAU_BO_RD) {
		pbbo->read_domains |= domains;
	}
	if (flags & NOUVEAU_BO_WR) {
		pbbo->write_domains |= domains;
		nvbo->write_marker = 1;
	}

	/* nvc0 gallium driver uses reloc_emit() with NULL target buffer
	 * to inform bufmgr of a buffer's use - however, we need something
	 * to track, so create a reloc for now, and hope it never triggers
	 * (it shouldn't, constant virtual address..)..
	 */
	if (!reloc_bo) {
		reloc_bo  = nvpb->buffer[nvpb->current];
		reloc_offset = 0;
		reloc_ptr = NULL;
	}

	/* add reloc target bo to validation list, and create the reloc */
	rpbbo = nouveau_bo_emit_buffer(chan, reloc_bo);
	if (!rpbbo)
		return -ENOMEM;
	nouveau_bo(reloc_bo)->pending_refcnt++;

	r = nvpb->relocs + nvpb->nr_relocs++;
	r->reloc_bo_index = rpbbo - nvpb->buffers;
	r->reloc_bo_offset = reloc_offset;
	r->bo_index = pbbo - nvpb->buffers;
	r->flags = 0;
	if (flags & NOUVEAU_BO_LOW)
		r->flags |= NOUVEAU_GEM_RELOC_LOW;
	if (flags & NOUVEAU_BO_HIGH)
		r->flags |= NOUVEAU_GEM_RELOC_HIGH;
	if (flags & NOUVEAU_BO_OR)
		r->flags |= NOUVEAU_GEM_RELOC_OR;
	r->data = data;
	r->vor = vor;
	r->tor = tor;

	if (reloc_ptr) {
		if (flags & NOUVEAU_BO_DUMMY)
			*reloc_ptr = 0;
		else
			*reloc_ptr = nouveau_reloc_calc(pbbo, r);
	}

	return 0;
}

