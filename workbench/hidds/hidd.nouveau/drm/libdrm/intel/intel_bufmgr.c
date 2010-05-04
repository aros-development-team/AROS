/*
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <drm.h>
#include <i915_drm.h>
#include "intel_bufmgr.h"
#include "intel_bufmgr_priv.h"

/** @file intel_bufmgr.c
 *
 * Convenience functions for buffer management methods.
 */

drm_intel_bo *drm_intel_bo_alloc(drm_intel_bufmgr *bufmgr, const char *name,
				 unsigned long size, unsigned int alignment)
{
	return bufmgr->bo_alloc(bufmgr, name, size, alignment);
}

drm_intel_bo *drm_intel_bo_alloc_for_render(drm_intel_bufmgr *bufmgr,
					    const char *name,
					    unsigned long size,
					    unsigned int alignment)
{
	return bufmgr->bo_alloc_for_render(bufmgr, name, size, alignment);
}

drm_intel_bo *
drm_intel_bo_alloc_tiled(drm_intel_bufmgr *bufmgr, const char *name,
                        int x, int y, int cpp, uint32_t *tiling_mode,
                        unsigned long *pitch, unsigned long flags)
{
	return bufmgr->bo_alloc_tiled(bufmgr, name, x, y, cpp,
				      tiling_mode, pitch, flags);
}

void drm_intel_bo_reference(drm_intel_bo *bo)
{
	bo->bufmgr->bo_reference(bo);
}

void drm_intel_bo_unreference(drm_intel_bo *bo)
{
	if (bo == NULL)
		return;

	bo->bufmgr->bo_unreference(bo);
}

int drm_intel_bo_map(drm_intel_bo *buf, int write_enable)
{
	return buf->bufmgr->bo_map(buf, write_enable);
}

int drm_intel_bo_unmap(drm_intel_bo *buf)
{
	return buf->bufmgr->bo_unmap(buf);
}

int
drm_intel_bo_subdata(drm_intel_bo *bo, unsigned long offset,
		     unsigned long size, const void *data)
{
	int ret;

	if (bo->bufmgr->bo_subdata)
		return bo->bufmgr->bo_subdata(bo, offset, size, data);
	if (size == 0 || data == NULL)
		return 0;

	ret = drm_intel_bo_map(bo, 1);
	if (ret)
		return ret;
	memcpy((unsigned char *)bo->virtual + offset, data, size);
	drm_intel_bo_unmap(bo);
	return 0;
}

int
drm_intel_bo_get_subdata(drm_intel_bo *bo, unsigned long offset,
			 unsigned long size, void *data)
{
	int ret;
	if (bo->bufmgr->bo_subdata)
		return bo->bufmgr->bo_get_subdata(bo, offset, size, data);

	if (size == 0 || data == NULL)
		return 0;

	ret = drm_intel_bo_map(bo, 0);
	if (ret)
		return ret;
	memcpy(data, (unsigned char *)bo->virtual + offset, size);
	drm_intel_bo_unmap(bo);
	return 0;
}

void drm_intel_bo_wait_rendering(drm_intel_bo *bo)
{
	bo->bufmgr->bo_wait_rendering(bo);
}

void drm_intel_bufmgr_destroy(drm_intel_bufmgr *bufmgr)
{
	bufmgr->destroy(bufmgr);
}

int
drm_intel_bo_exec(drm_intel_bo *bo, int used,
		  drm_clip_rect_t * cliprects, int num_cliprects, int DR4)
{
	return bo->bufmgr->bo_exec(bo, used, cliprects, num_cliprects, DR4);
}

void drm_intel_bufmgr_set_debug(drm_intel_bufmgr *bufmgr, int enable_debug)
{
	bufmgr->debug = enable_debug;
}

int drm_intel_bufmgr_check_aperture_space(drm_intel_bo ** bo_array, int count)
{
	return bo_array[0]->bufmgr->check_aperture_space(bo_array, count);
}

int drm_intel_bo_flink(drm_intel_bo *bo, uint32_t * name)
{
	if (bo->bufmgr->bo_flink)
		return bo->bufmgr->bo_flink(bo, name);

	return -ENODEV;
}

int
drm_intel_bo_emit_reloc(drm_intel_bo *bo, uint32_t offset,
			drm_intel_bo *target_bo, uint32_t target_offset,
			uint32_t read_domains, uint32_t write_domain)
{
	return bo->bufmgr->bo_emit_reloc(bo, offset,
					 target_bo, target_offset,
					 read_domains, write_domain);
}

int drm_intel_bo_pin(drm_intel_bo *bo, uint32_t alignment)
{
	if (bo->bufmgr->bo_pin)
		return bo->bufmgr->bo_pin(bo, alignment);

	return -ENODEV;
}

int drm_intel_bo_unpin(drm_intel_bo *bo)
{
	if (bo->bufmgr->bo_unpin)
		return bo->bufmgr->bo_unpin(bo);

	return -ENODEV;
}

int drm_intel_bo_set_tiling(drm_intel_bo *bo, uint32_t * tiling_mode,
			    uint32_t stride)
{
	if (bo->bufmgr->bo_set_tiling)
		return bo->bufmgr->bo_set_tiling(bo, tiling_mode, stride);

	*tiling_mode = I915_TILING_NONE;
	return 0;
}

int drm_intel_bo_get_tiling(drm_intel_bo *bo, uint32_t * tiling_mode,
			    uint32_t * swizzle_mode)
{
	if (bo->bufmgr->bo_get_tiling)
		return bo->bufmgr->bo_get_tiling(bo, tiling_mode, swizzle_mode);

	*tiling_mode = I915_TILING_NONE;
	*swizzle_mode = I915_BIT_6_SWIZZLE_NONE;
	return 0;
}

int drm_intel_bo_disable_reuse(drm_intel_bo *bo)
{
	if (bo->bufmgr->bo_disable_reuse)
		return bo->bufmgr->bo_disable_reuse(bo);
	return 0;
}

int drm_intel_bo_busy(drm_intel_bo *bo)
{
	if (bo->bufmgr->bo_busy)
		return bo->bufmgr->bo_busy(bo);
	return 0;
}

int drm_intel_bo_madvise(drm_intel_bo *bo, int madv)
{
	if (bo->bufmgr->bo_madvise)
		return bo->bufmgr->bo_madvise(bo, madv);
	return -1;
}

int drm_intel_bo_references(drm_intel_bo *bo, drm_intel_bo *target_bo)
{
	return bo->bufmgr->bo_references(bo, target_bo);
}

int drm_intel_get_pipe_from_crtc_id(drm_intel_bufmgr *bufmgr, int crtc_id)
{
	if (bufmgr->get_pipe_from_crtc_id)
		return bufmgr->get_pipe_from_crtc_id(bufmgr, crtc_id);
	return -1;
}
