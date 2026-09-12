/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: Dirty tracking for coherent buffer objects, AROS flavour.

    Linux watches the CPU writes to a coherent buffer through the page
    tables of its userspace mapping and uploads only the pages that
    changed. There is no such mapping here - the hidd writes straight
    into the buffer's kernel map - so nothing can tell which pages were
    touched. Every scan therefore reports the whole buffer dirty; the
    device gets a full image update instead of a partial one, which is
    correct if not economical.
*/

#include <linux/bitmap.h>

#include "vmwgfx_bo.h"
#include "vmwgfx_drv.h"

struct vmw_bo_dirty {
	struct   kref ref_count;
	unsigned long start;
	unsigned long end;
	unsigned int change_count;
	unsigned long bitmap_size;
	unsigned long bitmap[];
};

bool vmw_bo_is_dirty(struct vmw_bo *vbo)
{
	return vbo->dirty && (vbo->dirty->start < vbo->dirty->end);
}

/* every page could have been written since the last look */
static void vmw_bo_dirty_mark_all(struct vmw_bo_dirty *dirty)
{
	bitmap_fill(&dirty->bitmap[0], dirty->bitmap_size);
	dirty->start = 0;
	dirty->end = dirty->bitmap_size;
}

void vmw_bo_dirty_scan(struct vmw_bo *vbo)
{
	vmw_bo_dirty_mark_all(vbo->dirty);
}

void vmw_bo_dirty_unmap(struct vmw_bo *vbo, pgoff_t start, pgoff_t end)
{
}

int vmw_bo_dirty_add(struct vmw_bo *vbo)
{
	struct vmw_bo_dirty *dirty = vbo->dirty;
	pgoff_t num_pages = PFN_UP(vbo->tbo.resource->size);
	size_t size;

	if (dirty) {
		kref_get(&dirty->ref_count);
		return 0;
	}

	size = sizeof(*dirty) + BITS_TO_LONGS(num_pages) * sizeof(long);
	dirty = kvzalloc(size, GFP_KERNEL);
	if (!dirty)
		return -ENOMEM;

	dirty->bitmap_size = num_pages;
	kref_init(&dirty->ref_count);
	vmw_bo_dirty_mark_all(dirty);

	vbo->dirty = dirty;

	return 0;
}

static void vmw_bo_dirty_free(struct kref *kref)
{
	struct vmw_bo_dirty *dirty = container_of(kref, struct vmw_bo_dirty, ref_count);

	kvfree(dirty);
}

void vmw_bo_dirty_release(struct vmw_bo *vbo)
{
	struct vmw_bo_dirty *dirty = vbo->dirty;

	if (dirty && kref_put(&dirty->ref_count, vmw_bo_dirty_free))
		vbo->dirty = NULL;
}

/* the dirty pages within a resource's range, handed to the resource */
void vmw_bo_dirty_transfer_to_res(struct vmw_resource *res)
{
	struct vmw_bo *vbo = res->guest_memory_bo;
	struct vmw_bo_dirty *dirty = vbo->dirty;
	pgoff_t start, cur, end;
	unsigned long res_start = res->guest_memory_offset;
	unsigned long res_end = res->guest_memory_offset + res->guest_memory_size;

	WARN_ON_ONCE(res_start & ~PAGE_MASK);
	res_start >>= PAGE_SHIFT;
	res_end = DIV_ROUND_UP(res_end, PAGE_SIZE);

	if (res_start >= dirty->end || res_end <= dirty->start)
		return;

	cur = max(res_start, dirty->start);
	res_end = min(res_end, dirty->end);
	while (cur < res_end) {
		unsigned long num;

		start = find_next_bit(&dirty->bitmap[0], res_end, cur);
		if (start >= res_end)
			break;

		end = find_next_zero_bit(&dirty->bitmap[0], res_end, start + 1);
		cur = end + 1;
		num = end - start;
		bitmap_clear(&dirty->bitmap[0], start, num);
		vmw_resource_dirty_update(res, start, end);
	}

	if (res_start <= dirty->start && res_end > dirty->start)
		dirty->start = res_end;
	if (res_start < dirty->end && res_end >= dirty->end)
		dirty->end = res_start;
}

void vmw_bo_dirty_clear(struct vmw_bo *vbo)
{
	struct vmw_bo_dirty *dirty = vbo->dirty;

	bitmap_zero(&dirty->bitmap[0], dirty->bitmap_size);
	dirty->start = dirty->bitmap_size;
	dirty->end = 0;
}

void vmw_bo_dirty_clear_res(struct vmw_resource *res)
{
	unsigned long res_start = res->guest_memory_offset;
	unsigned long res_end = res->guest_memory_offset + res->guest_memory_size;
	struct vmw_bo *vbo = res->guest_memory_bo;
	struct vmw_bo_dirty *dirty = vbo->dirty;

	res_start >>= PAGE_SHIFT;
	res_end = DIV_ROUND_UP(res_end, PAGE_SIZE);

	if (res_start >= dirty->end || res_end <= dirty->start)
		return;

	res_start = max(res_start, dirty->start);
	res_end = min(res_end, dirty->end);
	bitmap_clear(&dirty->bitmap[0], res_start, res_end - res_start);

	if (res_start <= dirty->start && res_end > dirty->start)
		dirty->start = res_end;
	if (res_start < dirty->end && res_end >= dirty->end)
		dirty->end = res_start;
}

/* there are no page faults to serve */
vm_fault_t vmw_bo_vm_mkwrite(struct vm_fault *vmf)
{
	return VM_FAULT_SIGBUS;
}

vm_fault_t vmw_bo_vm_fault(struct vm_fault *vmf)
{
	return VM_FAULT_SIGBUS;
}
