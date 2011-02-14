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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#if !defined(__AROS__)
#include <sys/mman.h>
#endif

#include "nouveau_private.h"

int
nouveau_bo_init(struct nouveau_device *dev)
{
	return 0;
}

void
nouveau_bo_takedown(struct nouveau_device *dev)
{
}

static int
nouveau_bo_info(struct nouveau_bo_priv *nvbo, struct drm_nouveau_gem_info *arg)
{
	nvbo->handle = nvbo->base.handle = arg->handle;
	nvbo->domain = arg->domain;
	nvbo->size = arg->size;
	nvbo->offset = arg->offset;
	nvbo->map_handle = arg->map_handle;
	nvbo->base.tile_mode = arg->tile_mode;
	/* XXX - flag inverted for backwards compatibility */
	nvbo->base.tile_flags = arg->tile_flags ^ NOUVEAU_GEM_TILE_NONCONTIG;
	return 0;
}

static int
nouveau_bo_allocated(struct nouveau_bo_priv *nvbo)
{
	if (nvbo->sysmem || nvbo->handle)
		return 1;
	return 0;
}

static int
nouveau_bo_ualloc(struct nouveau_bo_priv *nvbo)
{
	if (nvbo->user || nvbo->sysmem) {
		assert(nvbo->sysmem);
		return 0;
	}

	nvbo->sysmem = malloc(nvbo->size);
	if (!nvbo->sysmem)
		return -ENOMEM;

	return 0;
}

static void
nouveau_bo_ufree(struct nouveau_bo_priv *nvbo)
{
	if (nvbo->sysmem) {
		if (!nvbo->user)
			free(nvbo->sysmem);
		nvbo->sysmem = NULL;
	}
}

static void
nouveau_bo_kfree(struct nouveau_bo_priv *nvbo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	struct drm_gem_close req;

	if (!nvbo->handle)
		return;

	if (nvbo->map) {
#if !defined(__AROS__)        
		munmap(nvbo->map, nvbo->size);
#else
        drmMUnmap(nvdev->fd, nvbo->handle);
#endif
		nvbo->map = NULL;
	}

	req.handle = nvbo->handle;
	nvbo->handle = 0;
	drmIoctl(nvdev->fd, DRM_IOCTL_GEM_CLOSE, &req);
}

static int
nouveau_bo_kalloc(struct nouveau_bo_priv *nvbo, struct nouveau_channel *chan)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);
	struct drm_nouveau_gem_new req;
	struct drm_nouveau_gem_info *info = &req.info;
	int ret;

	if (nvbo->handle)
		return 0;

	req.channel_hint = chan ? chan->id : 0;
	req.align = nvbo->align;


	info->size = nvbo->size;
	info->domain = 0;

	if (nvbo->flags & NOUVEAU_BO_VRAM)
		info->domain |= NOUVEAU_GEM_DOMAIN_VRAM;
	if (nvbo->flags & NOUVEAU_BO_GART)
		info->domain |= NOUVEAU_GEM_DOMAIN_GART;
	if (!info->domain) {
		info->domain |= (NOUVEAU_GEM_DOMAIN_VRAM |
				 NOUVEAU_GEM_DOMAIN_GART);
	}

	if (nvbo->flags & NOUVEAU_BO_MAP)
		info->domain |= NOUVEAU_GEM_DOMAIN_MAPPABLE;

	info->tile_mode = nvbo->base.tile_mode;
	info->tile_flags = nvbo->base.tile_flags;
	/* XXX - flag inverted for backwards compatibility */
	info->tile_flags ^= NOUVEAU_GEM_TILE_NONCONTIG;
	if (!nvdev->has_bo_usage)
		info->tile_flags &= NOUVEAU_GEM_TILE_LAYOUT_MASK;

	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_GEM_NEW,
				  &req, sizeof(req));
	if (ret)
		return ret;

	nouveau_bo_info(nvbo, &req.info);
	return 0;
}

static int
nouveau_bo_kmap(struct nouveau_bo_priv *nvbo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(nvbo->base.device);

	if (nvbo->map)
		return 0;

	if (!nvbo->map_handle)
		return -EINVAL;

#if !defined(__AROS__)
	nvbo->map = mmap(0, nvbo->size, PROT_READ | PROT_WRITE,
			 MAP_SHARED, nvdev->fd, nvbo->map_handle);
	if (nvbo->map == MAP_FAILED) {
		nvbo->map = NULL;
		return -errno;
	}
#else
    nvbo->map = drmMMap(nvdev->fd, nvbo->handle);
    if (nvbo->map == NULL)
        return -EINVAL;
#endif
	return 0;
}

int
nouveau_bo_new_tile(struct nouveau_device *dev, uint32_t flags, int align,
		    int size, uint32_t tile_mode, uint32_t tile_flags,
		    struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;
	int ret;

	if (!dev || !bo || *bo)
		return -EINVAL;

	nvbo = calloc(1, sizeof(struct nouveau_bo_priv));
	if (!nvbo)
		return -ENOMEM;
	nvbo->base.device = dev;
	nvbo->base.size = size;
	nvbo->base.tile_mode = tile_mode;
	nvbo->base.tile_flags = tile_flags;

	nvbo->refcount = 1;
	nvbo->flags = flags;
	nvbo->size = size;
	nvbo->align = align;

	if (flags & (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART)) {
		ret = nouveau_bo_kalloc(nvbo, NULL);
		if (ret) {
			nouveau_bo_ref(NULL, (void *)&nvbo);
			return ret;
		}
	}

	*bo = &nvbo->base;
	return 0;
}

int
nouveau_bo_new(struct nouveau_device *dev, uint32_t flags, int align,
	       int size, struct nouveau_bo **bo)
{
	return nouveau_bo_new_tile(dev, flags, align, size, 0, 0, bo);
}

int
nouveau_bo_user(struct nouveau_device *dev, void *ptr, int size,
		struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;
	int ret;

	ret = nouveau_bo_new(dev, NOUVEAU_BO_MAP, 0, size, bo);
	if (ret)
		return ret;
	nvbo = nouveau_bo(*bo);

	nvbo->sysmem = ptr;
	nvbo->user = 1;
	return 0;
}

int
nouveau_bo_wrap(struct nouveau_device *dev, uint32_t handle,
		struct nouveau_bo **bo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);
	struct drm_nouveau_gem_info req;
	struct nouveau_bo_priv *nvbo;
	int ret;

	ret = nouveau_bo_new(dev, 0, 0, 0, bo);
	if (ret)
		return ret;
	nvbo = nouveau_bo(*bo);

	req.handle = handle;
	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_GEM_INFO,
				  &req, sizeof(req));
	if (ret) {
		nouveau_bo_ref(NULL, bo);
		return ret;
	}

	nouveau_bo_info(nvbo, &req);
	nvbo->base.size = nvbo->size;
	return 0;
}

int
nouveau_bo_handle_get(struct nouveau_bo *bo, uint32_t *handle)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	int ret;
 
	if (!bo || !handle)
		return -EINVAL;

	if (!nvbo->global_handle) {
		struct drm_gem_flink req;
 
		ret = nouveau_bo_kalloc(nvbo, NULL);
		if (ret)
			return ret;

		req.handle = nvbo->handle;
		ret = drmIoctl(nvdev->fd, DRM_IOCTL_GEM_FLINK, &req);
		if (ret) {
			nouveau_bo_kfree(nvbo);
			return ret;
		}

		nvbo->global_handle = req.name;
	}
 
	*handle = nvbo->global_handle;
	return 0;
}
 
int
nouveau_bo_handle_ref(struct nouveau_device *dev, uint32_t handle,
		      struct nouveau_bo **bo)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);
	struct nouveau_bo_priv *nvbo;
	struct drm_gem_open req;
	int ret;

	req.name = handle;
	ret = drmIoctl(nvdev->fd, DRM_IOCTL_GEM_OPEN, &req);
	if (ret) {
		nouveau_bo_ref(NULL, bo);
		return ret;
	}

	ret = nouveau_bo_wrap(dev, req.handle, bo);
	if (ret) {
		nouveau_bo_ref(NULL, bo);
		return ret;
	}

	nvbo = nouveau_bo(*bo);
	nvbo->base.handle = nvbo->handle;
	return 0;
} 

static void
nouveau_bo_del(struct nouveau_bo **bo)
{
	struct nouveau_bo_priv *nvbo;

	if (!bo || !*bo)
		return;
	nvbo = nouveau_bo(*bo);
	*bo = NULL;

	if (--nvbo->refcount)
		return;

	if (nvbo->pending) {
		nvbo->pending = NULL;
		nouveau_pushbuf_flush(nvbo->pending_channel, 0);
	}

	nouveau_bo_ufree(nvbo);
	nouveau_bo_kfree(nvbo);
	free(nvbo);
}

int
nouveau_bo_ref(struct nouveau_bo *ref, struct nouveau_bo **pbo)
{
	if (!pbo)
		return -EINVAL;

	if (ref)
		nouveau_bo(ref)->refcount++;

	if (*pbo)
		nouveau_bo_del(pbo);

	*pbo = ref;
	return 0;
}

static int
nouveau_bo_wait(struct nouveau_bo *bo, int cpu_write, int no_wait, int no_block)
{
	struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct drm_nouveau_gem_cpu_prep req;
	int ret;

	if (!nvbo->global_handle && !nvbo->write_marker && !cpu_write)
		return 0;

	if (nvbo->pending &&
	    (nvbo->pending->write_domains || cpu_write)) {
		nvbo->pending = NULL;
		nouveau_pushbuf_flush(nvbo->pending_channel, 0);
	}

	req.handle = nvbo->handle;
	req.flags = 0;
	if (cpu_write)
		req.flags |= NOUVEAU_GEM_CPU_PREP_WRITE;
	if (no_wait)
		req.flags |= NOUVEAU_GEM_CPU_PREP_NOWAIT;

	do {
		ret = drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GEM_CPU_PREP,
				      &req, sizeof(req));
	} while (ret == -EAGAIN);
	if (ret)
		return ret;

	if (ret == 0)
		nvbo->write_marker = 0;
	return 0;
}

int
nouveau_bo_map_range(struct nouveau_bo *bo, uint32_t delta, uint32_t size,
		     uint32_t flags)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	int ret;

	if (!nvbo || bo->map)
		return -EINVAL;

	if (!nouveau_bo_allocated(nvbo)) {
		if (nvbo->flags & (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART)) {
			ret = nouveau_bo_kalloc(nvbo, NULL);
			if (ret)
				return ret;
		}

		if (!nouveau_bo_allocated(nvbo)) {
			ret = nouveau_bo_ualloc(nvbo);
			if (ret)
				return ret;
		}
	}

	if (nvbo->sysmem) {
		bo->map = (char *)nvbo->sysmem + delta;
	} else {
		ret = nouveau_bo_kmap(nvbo);
		if (ret)
			return ret;

		if (!(flags & NOUVEAU_BO_NOSYNC)) {
			ret = nouveau_bo_wait(bo, (flags & NOUVEAU_BO_WR),
					      (flags & NOUVEAU_BO_NOWAIT), 0);
			if (ret)
				return ret;

			nvbo->map_refcnt++;
		}

		bo->map = (char *)nvbo->map + delta;
	}

	return 0;
}

void
nouveau_bo_map_flush(struct nouveau_bo *bo, uint32_t delta, uint32_t size)
{
}

int
nouveau_bo_map(struct nouveau_bo *bo, uint32_t flags)
{
	return nouveau_bo_map_range(bo, 0, bo->size, flags);
}

void
nouveau_bo_unmap(struct nouveau_bo *bo)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);

	if (bo->map && !nvbo->sysmem && nvbo->map_refcnt) {
		struct nouveau_device_priv *nvdev = nouveau_device(bo->device);
		struct drm_nouveau_gem_cpu_fini req;

		req.handle = nvbo->handle;
		drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GEM_CPU_FINI,
				&req, sizeof(req));
		nvbo->map_refcnt--;
	}

	bo->map = NULL;
}

int
nouveau_bo_busy(struct nouveau_bo *bo, uint32_t access)
{
	return nouveau_bo_wait(bo, (access & NOUVEAU_BO_WR), 1, 1);
}

uint32_t
nouveau_bo_pending(struct nouveau_bo *bo)
{
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	uint32_t flags;

	if (!nvbo->pending)
		return 0;

	flags = 0;
	if (nvbo->pending->read_domains)
		flags |= NOUVEAU_BO_RD;
	if (nvbo->pending->write_domains)
		flags |= NOUVEAU_BO_WR;

	return flags;
}

struct drm_nouveau_gem_pushbuf_bo *
nouveau_bo_emit_buffer(struct nouveau_channel *chan, struct nouveau_bo *bo)
{
	struct nouveau_pushbuf_priv *nvpb = &nouveau_channel(chan)->pb;
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct drm_nouveau_gem_pushbuf_bo *pbbo;
	struct nouveau_bo *ref = NULL;
	int ret;

	if (nvbo->pending)
		return nvbo->pending;

	if (!nvbo->handle) {
		ret = nouveau_bo_kalloc(nvbo, chan);
		if (ret)
			return NULL;

		if (nvbo->sysmem) {
			void *sysmem_tmp = nvbo->sysmem;

			nvbo->sysmem = NULL;
			ret = nouveau_bo_map(bo, NOUVEAU_BO_WR);
			if (ret)
				return NULL;
			nvbo->sysmem = sysmem_tmp;

			memcpy(bo->map, nvbo->sysmem, nvbo->base.size);
			nouveau_bo_ufree(nvbo);
			nouveau_bo_unmap(bo);
		}
	}

	if (nvpb->nr_buffers >= NOUVEAU_GEM_MAX_BUFFERS)
		return NULL;
	pbbo = nvpb->buffers + nvpb->nr_buffers++;
	nvbo->pending = pbbo;
	nvbo->pending_channel = chan;
	nvbo->pending_refcnt = 0;

	nouveau_bo_ref(bo, &ref);
	pbbo->user_priv = (uint64_t)(unsigned long)ref;
	pbbo->handle = nvbo->handle;
	pbbo->valid_domains = NOUVEAU_GEM_DOMAIN_VRAM | NOUVEAU_GEM_DOMAIN_GART;
	pbbo->read_domains = 0;
	pbbo->write_domains = 0;
	pbbo->presumed.domain = nvbo->domain;
	pbbo->presumed.offset = nvbo->offset;
	pbbo->presumed.valid = 1;
	return pbbo;
}
