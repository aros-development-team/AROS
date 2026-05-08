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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "nouveau_private.h"

#define PB_BUFMGR_DWORDS   (4096 / 2)
#define PB_MIN_USER_DWORDS  2048

static int
nouveau_pushbuf_space(struct nouveau_channel *chan, unsigned min)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	struct nouveau_bo *bo;
	int ret;

	if (min < PB_MIN_USER_DWORDS)
		min = PB_MIN_USER_DWORDS;

	nvpb->current_offset = chan->cur - nvpb->pushbuf;
	if (chan->cur + min + 2 <= chan->end)
		return 0;

	nvpb->current++;
	if (nvpb->current == CALPB_BUFFERS)
		nvpb->current = 0;
	bo = nvpb->buffer[nvpb->current];

	ret = nouveau_bo_map(bo, NOUVEAU_BO_WR);
	if (ret)
		return ret;

	nvpb->size = (bo->size - 8) / 4;
	nvpb->pushbuf = bo->map;
	nvpb->current_offset = 0;

	chan->cur = nvpb->pushbuf;
	chan->end = nvpb->pushbuf + nvpb->size;

	nouveau_bo_unmap(bo);
	return 0;
}

static void
nouveau_pushbuf_fini_call(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	int i;

	for (i = 0; i < CALPB_BUFFERS; i++)
		nouveau_bo_ref(NULL, &nvpb->buffer[i]);
	nvpb->pushbuf = NULL;
}

static int
nouveau_pushbuf_init_call(struct nouveau_channel *chan, int buf_size)
{
	struct drm_nouveau_gem_pushbuf req;
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	struct nouveau_device *dev = chan->device;
	uint32_t flags = 0;
	int i, ret;

	if (nvchan->drm.pushbuf_domains & NOUVEAU_GEM_DOMAIN_GART)
		flags |= NOUVEAU_BO_GART;
	else
		flags |= NOUVEAU_BO_VRAM;

	req.channel = chan->id;
	req.nr_push = 0;
	ret = drmCommandWriteRead(nouveau_device(dev)->fd,
				  DRM_NOUVEAU_GEM_PUSHBUF, &req, sizeof(req));
	if (ret)
		return ret;

	for (i = 0; i < CALPB_BUFFERS; i++) {
		ret = nouveau_bo_new(dev, flags | NOUVEAU_BO_MAP,
				     0, buf_size, &nvpb->buffer[i]);
		if (ret) {
			nouveau_pushbuf_fini_call(chan);
			return ret;
		}
	}

	nvpb->cal_suffix0 = req.suffix0;
	nvpb->cal_suffix1 = req.suffix1;
	return 0;
}

int
nouveau_pushbuf_init(struct nouveau_channel *chan, int buf_size)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	int ret;

	ret = nouveau_pushbuf_init_call(chan, buf_size);
	if (ret)
		return ret;

	ret = nouveau_pushbuf_space(chan, 0);
	if (ret)
		return ret;

	nvpb->buffers = calloc(NOUVEAU_GEM_MAX_BUFFERS,
			       sizeof(struct drm_nouveau_gem_pushbuf_bo));
	nvpb->relocs = calloc(NOUVEAU_GEM_MAX_RELOCS,
			      sizeof(struct drm_nouveau_gem_pushbuf_reloc));
	return 0;
}

void
nouveau_pushbuf_fini(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	nouveau_pushbuf_fini_call(chan);
	free(nvpb->buffers);
	free(nvpb->relocs);
}

static int
nouveau_pushbuf_bo_add(struct nouveau_channel *chan, struct nouveau_bo *bo,
		       unsigned offset, unsigned length)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	struct drm_nouveau_gem_pushbuf_push *p = &nvpb->push[nvpb->nr_push++];
	struct drm_nouveau_gem_pushbuf_bo *pbbo;
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

	pbbo = nouveau_bo_emit_buffer(chan, bo);
	if (!pbbo)
		return -ENOMEM;
	pbbo->valid_domains &= nvchan->drm.pushbuf_domains;
	pbbo->read_domains |= nvchan->drm.pushbuf_domains;
	nvbo->pending_refcnt++;

	p->bo_index = pbbo - nvpb->buffers;
	p->offset = offset;
	p->length = length;
	return 0;
}

int
nouveau_pushbuf_submit(struct nouveau_channel *chan, struct nouveau_bo *bo,
		       unsigned offset, unsigned length)
{
	struct nouveau_pushbuf_priv *nvpb = &nouveau_channel(chan)->pb;
	int ret, len;

	if ((AVAIL_RING(chan) + nvpb->current_offset) != nvpb->size) {
		if (nvpb->cal_suffix0 || nvpb->cal_suffix1) {
			*(chan->cur++) = nvpb->cal_suffix0;
			*(chan->cur++) = nvpb->cal_suffix1;
		}

		len = (chan->cur - nvpb->pushbuf) - nvpb->current_offset;

		ret = nouveau_pushbuf_bo_add(chan, nvpb->buffer[nvpb->current],
					     nvpb->current_offset * 4, len * 4);
		if (ret)
			return ret;

		nvpb->current_offset += len;
	}

	return bo ? nouveau_pushbuf_bo_add(chan, bo, offset, length) : 0;
}

static void
nouveau_pushbuf_bo_unref(struct nouveau_pushbuf_priv *nvpb, int index)
{
	struct drm_nouveau_gem_pushbuf_bo *pbbo = &nvpb->buffers[index];
	struct nouveau_bo *bo = (void *)(unsigned long)pbbo->user_priv;
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

	if (--nvbo->pending_refcnt)
		return;

	if (pbbo->presumed.valid == 0) {
		nvbo->domain = pbbo->presumed.domain;
		nvbo->offset = pbbo->presumed.offset;
	}

	nvbo->pending = NULL;
	nouveau_bo_ref(NULL, &bo);

	/* we only ever remove from the tail of the pending lists,
	 * so this is safe.
	 */
	nvpb->nr_buffers--;
}

int
nouveau_pushbuf_flush(struct nouveau_channel *chan, unsigned min)
{
	struct nouveau_device_priv *nvdev = nouveau_device(chan->device);
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	struct drm_nouveau_gem_pushbuf req;
	unsigned i;
	int ret;

	ret = nouveau_pushbuf_submit(chan, NULL, 0, 0);
	if (ret)
		return ret;

	if (!nvpb->nr_push)
		return 0;

	req.channel = chan->id;
	req.nr_push = nvpb->nr_push;
	req.push = (uint64_t)(unsigned long)nvpb->push;
	req.nr_buffers = nvpb->nr_buffers;
	req.buffers = (uint64_t)(unsigned long)nvpb->buffers;
	req.nr_relocs = nvpb->nr_relocs;
	req.relocs = (uint64_t)(unsigned long)nvpb->relocs;
	req.suffix0 = nvpb->cal_suffix0;
	req.suffix1 = nvpb->cal_suffix1;

	do {
		ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_GEM_PUSHBUF,
					  &req, sizeof(req));
	} while (ret == -EAGAIN);
	nvpb->cal_suffix0 = req.suffix0;
	nvpb->cal_suffix1 = req.suffix1;
	nvdev->base.vm_vram_size = req.vram_available;
	nvdev->base.vm_gart_size = req.gart_available;

	/* Update presumed offset/domain for any buffers that moved.
	 * Dereference all buffers on validate list
	 */
	for (i = 0; i < nvpb->nr_relocs; i++) {
		nouveau_pushbuf_bo_unref(nvpb, nvpb->relocs[i].bo_index);
		nouveau_pushbuf_bo_unref(nvpb, nvpb->relocs[i].reloc_bo_index);
	}

	for (i = 0; i < nvpb->nr_push; i++)
		nouveau_pushbuf_bo_unref(nvpb, nvpb->push[i].bo_index);

	nvpb->nr_buffers = 0;
	nvpb->nr_relocs = 0;
	nvpb->nr_push = 0;

	/* Allocate space for next push buffer */
	assert(!nouveau_pushbuf_space(chan, min));

	if (chan->flush_notify)
		chan->flush_notify(chan);

	nvpb->marker = NULL;
	return ret;
}

int
nouveau_pushbuf_marker_emit(struct nouveau_channel *chan,
			    unsigned wait_dwords, unsigned wait_relocs)
{
	struct nouveau_pushbuf_priv *nvpb = &nouveau_channel(chan)->pb;

	if (AVAIL_RING(chan) < wait_dwords)
		return nouveau_pushbuf_flush(chan, wait_dwords);

	if (nvpb->nr_relocs + wait_relocs >= NOUVEAU_GEM_MAX_RELOCS)
		return nouveau_pushbuf_flush(chan, wait_dwords);

	nvpb->marker = chan->cur;
	nvpb->marker_offset = nvpb->current_offset;
	nvpb->marker_push = nvpb->nr_push;
	nvpb->marker_relocs = nvpb->nr_relocs;
	return 0;
}

void
nouveau_pushbuf_marker_undo(struct nouveau_channel *chan)
{
	struct nouveau_pushbuf_priv *nvpb = &nouveau_channel(chan)->pb;
	unsigned i;

	if (!nvpb->marker)
		return;

	/* undo any relocs/buffers added to the list since last marker */
	for (i = nvpb->marker_relocs; i < nvpb->nr_relocs; i++) {
		nouveau_pushbuf_bo_unref(nvpb, nvpb->relocs[i].bo_index);
		nouveau_pushbuf_bo_unref(nvpb, nvpb->relocs[i].reloc_bo_index);
	}
	nvpb->nr_relocs = nvpb->marker_relocs;

	for (i = nvpb->marker_push; i < nvpb->nr_push; i++)
		nouveau_pushbuf_bo_unref(nvpb, nvpb->push[i].bo_index);
	nvpb->nr_push = nvpb->marker_push;

	/* reset pushbuf back to last marker */
	chan->cur = nvpb->marker;
	nvpb->current_offset = nvpb->marker_offset;
	nvpb->marker = NULL;
}

int
nouveau_pushbuf_emit_reloc(struct nouveau_channel *chan, void *ptr,
			   struct nouveau_bo *bo, uint32_t data, uint32_t data2,
			   uint32_t flags, uint32_t vor, uint32_t tor)
{
	struct nouveau_pushbuf_priv *nvpb = &nouveau_channel(chan)->pb;
	int ret;

	ret = nouveau_reloc_emit(chan, nvpb->buffer[nvpb->current],
				 (char *)ptr - (char *)nvpb->pushbuf, ptr,
				 bo, data, data2, flags, vor, tor);
	if (ret)
		return ret;

	return 0;
}

