/*
 * Copyright 2024 Advanced Micro Devices, Inc.
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
 */

#include <string.h>
#include <errno.h>
#include "xf86drm.h"
#include "amdgpu_drm.h"
#include "amdgpu_internal.h"

drm_public int
amdgpu_create_userqueue(amdgpu_device_handle dev,
			uint32_t ip_type,
			uint32_t doorbell_handle,
			uint32_t doorbell_offset,
			uint64_t queue_va,
			uint64_t queue_size,
			uint64_t wptr_va,
			uint64_t rptr_va,
			void *mqd_in,
			uint32_t flags,
			uint32_t *queue_id)
{
	int ret;
	union drm_amdgpu_userq userq;
	uint64_t mqd_size;

	if (!dev)
		return -EINVAL;

	switch (ip_type) {
	case AMDGPU_HW_IP_GFX:
		mqd_size = sizeof(struct drm_amdgpu_userq_mqd_gfx11);
		break;
	case AMDGPU_HW_IP_DMA:
		mqd_size = sizeof(struct drm_amdgpu_userq_mqd_sdma_gfx11);
		break;
	case AMDGPU_HW_IP_COMPUTE:
		mqd_size = sizeof(struct drm_amdgpu_userq_mqd_compute_gfx11);
		break;
	default:
		return -EINVAL;
	}

	memset(&userq, 0, sizeof(userq));

	userq.in.op = AMDGPU_USERQ_OP_CREATE;
	userq.in.ip_type = ip_type;

	userq.in.doorbell_handle = doorbell_handle;
	userq.in.doorbell_offset = doorbell_offset;

	userq.in.queue_va = queue_va;
	userq.in.queue_size = queue_size;
	userq.in.wptr_va = wptr_va;
	userq.in.rptr_va = rptr_va;

	userq.in.mqd = (uint64_t)mqd_in;
	userq.in.mqd_size = mqd_size;
	userq.in.flags = flags;

	ret = drmCommandWriteRead(dev->fd, DRM_AMDGPU_USERQ,
				  &userq, sizeof(userq));
	*queue_id = userq.out.queue_id;

	return ret;
}

drm_public int
amdgpu_free_userqueue(amdgpu_device_handle dev, uint32_t queue_id)
{
	union drm_amdgpu_userq userq;

	memset(&userq, 0, sizeof(userq));
	userq.in.op = AMDGPU_USERQ_OP_FREE;
	userq.in.queue_id = queue_id;

	return drmCommandWriteRead(dev->fd, DRM_AMDGPU_USERQ,
				   &userq, sizeof(userq));
}

drm_public int
amdgpu_userq_signal(amdgpu_device_handle dev,
		    struct drm_amdgpu_userq_signal *signal_data)
{
	int r;

	r = drmCommandWriteRead(dev->fd, DRM_AMDGPU_USERQ_SIGNAL,
				signal_data, sizeof(struct drm_amdgpu_userq_signal));

	return r;
}

drm_public int
amdgpu_userq_wait(amdgpu_device_handle dev,
		  struct drm_amdgpu_userq_wait *wait_data)
{
	int r;

	r = drmCommandWriteRead(dev->fd, DRM_AMDGPU_USERQ_WAIT,
				wait_data, sizeof(struct drm_amdgpu_userq_wait));

	return r;
}
