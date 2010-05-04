/*
 * Copyright (C) 2007 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "drmP.h"
#include "drm.h"
#include "nouveau_drv.h"

int
nouveau_notifier_init_channel(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct nouveau_bo *ntfy = NULL;
	uint32_t flags;
	int ret;

	if (nouveau_vram_notify)
		flags = TTM_PL_FLAG_VRAM;
	else
		flags = TTM_PL_FLAG_TT;

	ret = nouveau_gem_new(dev, NULL, PAGE_SIZE, 0, flags,
			      0, 0x0000, false, true, &ntfy);
	if (ret)
		return ret;

	ret = nouveau_bo_pin(ntfy, flags);
	if (ret)
		goto out_err;

	ret = nouveau_bo_map(ntfy);
	if (ret)
		goto out_err;

	ret = nouveau_mem_init_heap(&chan->notifier_heap, 0, ntfy->bo.mem.size);
	if (ret)
		goto out_err;

	chan->notifier_bo = ntfy;
out_err:
	if (ret) {
		mutex_lock(&dev->struct_mutex);
		drm_gem_object_unreference(ntfy->gem);
		mutex_unlock(&dev->struct_mutex);
	}

	return ret;
}

void
nouveau_notifier_takedown_channel(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;

	if (!chan->notifier_bo)
		return;

	nouveau_bo_unmap(chan->notifier_bo);
	mutex_lock(&dev->struct_mutex);
	nouveau_bo_unpin(chan->notifier_bo);
	drm_gem_object_unreference(chan->notifier_bo->gem);
	mutex_unlock(&dev->struct_mutex);
	nouveau_mem_takedown(&chan->notifier_heap);
}

static void
nouveau_notifier_gpuobj_dtor(struct drm_device *dev,
			     struct nouveau_gpuobj *gpuobj)
{
	NV_DEBUG(dev, "\n");

	if (gpuobj->priv)
		nouveau_mem_free_block(gpuobj->priv);
}

int
nouveau_notifier_alloc(struct nouveau_channel *chan, uint32_t handle,
		       int size, uint32_t *b_offset)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *nobj = NULL;
	struct mem_block *mem;
	uint32_t offset;
	int target, ret;

	if (!chan->notifier_heap) {
		NV_ERROR(dev, "Channel %d doesn't have a notifier heap!\n",
			 chan->id);
		return -EINVAL;
	}

	mem = nouveau_mem_alloc_block(chan->notifier_heap, size, 0,
				      (struct drm_file *)-2, 0);
	if (!mem) {
		NV_ERROR(dev, "Channel %d notifier block full\n", chan->id);
		return -ENOMEM;
	}

	offset = chan->notifier_bo->bo.mem.mm_node->start << PAGE_SHIFT;
	if (chan->notifier_bo->bo.mem.mem_type == TTM_PL_VRAM) {
		target = NV_DMA_TARGET_VIDMEM;
	} else
	if (chan->notifier_bo->bo.mem.mem_type == TTM_PL_TT) {
		if (dev_priv->gart_info.type == NOUVEAU_GART_SGDMA &&
		    dev_priv->card_type < NV_50) {
			ret = nouveau_sgdma_get_page(dev, offset, &offset);
			if (ret)
				return ret;
			target = NV_DMA_TARGET_PCI;
		} else {
			target = NV_DMA_TARGET_AGP;
			if (dev_priv->card_type >= NV_50)
				offset += dev_priv->vm_gart_base;
		}
	} else {
		NV_ERROR(dev, "Bad DMA target, mem_type %d!\n",
			 chan->notifier_bo->bo.mem.mem_type);
		return -EINVAL;
	}
	offset += mem->start;

	ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY, offset,
				     mem->size, NV_DMA_ACCESS_RW, target,
				     &nobj);
	if (ret) {
		nouveau_mem_free_block(mem);
		NV_ERROR(dev, "Error creating notifier ctxdma: %d\n", ret);
		return ret;
	}
	nobj->dtor   = nouveau_notifier_gpuobj_dtor;
	nobj->priv   = mem;

	ret = nouveau_gpuobj_ref_add(dev, chan, handle, nobj, NULL);
	if (ret) {
		nouveau_gpuobj_del(dev, &nobj);
		nouveau_mem_free_block(mem);
		NV_ERROR(dev, "Error referencing notifier ctxdma: %d\n", ret);
		return ret;
	}

	*b_offset = mem->start;
	return 0;
}

int
nouveau_notifier_offset(struct nouveau_gpuobj *nobj, uint32_t *poffset)
{
	if (!nobj || nobj->dtor != nouveau_notifier_gpuobj_dtor)
		return -EINVAL;

	if (poffset) {
		struct mem_block *mem = nobj->priv;

		if (*poffset >= mem->size)
			return false;

		*poffset += mem->start;
	}

	return 0;
}

int
nouveau_ioctl_notifier_alloc(struct drm_device *dev, void *data,
			     struct drm_file *file_priv)
{
	struct drm_nouveau_notifierobj_alloc *na = data;
	struct nouveau_channel *chan;
	int ret;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;
	NOUVEAU_GET_USER_CHANNEL_WITH_RETURN(na->channel, file_priv, chan);

	ret = nouveau_notifier_alloc(chan, na->handle, na->size, &na->offset);
	if (ret)
		return ret;

	return 0;
}
