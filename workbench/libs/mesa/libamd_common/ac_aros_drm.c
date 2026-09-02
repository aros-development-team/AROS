/*
 * Copyright © 2026 The AROS Development Team. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * AROS replacement for ac_linux_drm.c
 *
 * On Linux, this file wraps libdrm_amdgpu ioctls. On AROS, the GPU is
 * accessed directly via pci.hidd (MapPCI). We provide stub implementations
 * of all ac_drm_* functions here — the actual GPU communication happens
 * in the AROS radeon_winsys adapter in the radeonsi HIDD.
 */

#include "ac_linux_drm.h"
#include "ac_gpu_info.h"

#include <stddef.h>

/*
 * Stub implementations for all ac_drm_* functions.
 * These return error codes (-1, -EINVAL, NULL) to indicate
 * "no DRM device available". The AROS winsys adapter bypasses
 * these entirely and talks directly to hardware.
 */

int ac_drm_device_initialize(int fd, bool is_virtio,
                             uint32_t *major_version, uint32_t *minor_version,
                             ac_drm_device **device_handle)
{
    (void)fd; (void)is_virtio;
    if (major_version) *major_version = 0;
    if (minor_version) *minor_version = 0;
    if (device_handle) *device_handle = NULL;
    return -1;
}

struct util_sync_provider *ac_drm_device_get_sync_provider(ac_drm_device *dev)
{
    (void)dev;
    return NULL;
}

uintptr_t ac_drm_device_get_cookie(ac_drm_device *dev)
{
    (void)dev;
    return 0;
}

void ac_drm_device_deinitialize(ac_drm_device *dev) { (void)dev; }

int ac_drm_device_get_fd(ac_drm_device *dev) { (void)dev; return -1; }

int ac_drm_bo_set_metadata(ac_drm_device *dev, uint32_t bo_handle,
                           struct amdgpu_bo_metadata *info)
{ (void)dev; (void)bo_handle; (void)info; return -1; }

int ac_drm_bo_query_info(ac_drm_device *dev, uint32_t bo_handle,
                         struct amdgpu_bo_info *info)
{ (void)dev; (void)bo_handle; (void)info; return -1; }

int ac_drm_bo_wait_for_idle(ac_drm_device *dev, ac_drm_bo bo,
                            uint64_t timeout_ns, bool *busy)
{ (void)dev; (void)timeout_ns; if (busy) *busy = false; return -1; }

int ac_drm_bo_va_op(ac_drm_device *dev, uint32_t bo_handle, uint64_t offset,
                    uint64_t size, uint64_t addr, uint64_t flags, uint32_t ops)
{ (void)dev; (void)bo_handle; (void)offset; (void)size; (void)addr; (void)flags; (void)ops; return -1; }

int ac_drm_bo_va_op_raw(ac_drm_device *dev, uint32_t bo_handle, uint64_t offset,
                        uint64_t size, uint64_t addr, uint64_t flags, uint32_t ops)
{ (void)dev; (void)bo_handle; (void)offset; (void)size; (void)addr; (void)flags; (void)ops; return -1; }

int ac_drm_bo_va_op_raw2(ac_drm_device *dev, uint32_t bo_handle, uint64_t offset,
                         uint64_t size, uint64_t addr, uint64_t flags, uint32_t ops,
                         uint32_t vm_timeline_syncobj_out, uint64_t vm_timeline_point,
                         uint64_t input_fence_syncobj_handles, uint32_t num_syncobj_handles)
{ return -1; }

int ac_drm_cs_ctx_create2(ac_drm_device *dev, uint32_t priority, uint32_t *ctx_id)
{ (void)dev; (void)priority; (void)ctx_id; return -1; }

int ac_drm_cs_ctx_free(ac_drm_device *dev, uint32_t ctx_id)
{ (void)dev; (void)ctx_id; return -1; }

int ac_drm_cs_ctx_stable_pstate(ac_drm_device *dev, uint32_t ctx_id, uint32_t op,
                                uint32_t flags, uint32_t *out_flags)
{ return -1; }

int ac_drm_cs_query_reset_state2(ac_drm_device *dev, uint32_t ctx_id, uint64_t *flags)
{ (void)dev; (void)ctx_id; (void)flags; return -1; }

int ac_drm_cs_query_fence_status(ac_drm_device *dev, uint32_t ctx_id, uint32_t ip_type,
                                 uint32_t ip_instance, uint32_t ring, uint64_t fence_seq_no,
                                 uint64_t timeout_ns, uint64_t flags, uint32_t *expired)
{ return -1; }

int ac_drm_cs_create_syncobj2(ac_drm_device *dev, uint32_t flags, uint32_t *handle)
{ (void)dev; (void)flags; (void)handle; return -1; }

int ac_drm_cs_destroy_syncobj(ac_drm_device *dev, uint32_t handle)
{ (void)dev; (void)handle; return -1; }

int ac_drm_cs_syncobj_wait(ac_drm_device *dev, uint32_t *handles, unsigned num_handles,
                           int64_t timeout_nsec, unsigned flags, uint32_t *first_signaled)
{ return -1; }

int ac_drm_cs_syncobj_query2(ac_drm_device *dev, uint32_t *handles, uint64_t *points,
                             unsigned num_handles, uint32_t flags)
{ return -1; }

int ac_drm_cs_import_syncobj(ac_drm_device *dev, int shared_fd, uint32_t *handle)
{ (void)dev; (void)shared_fd; (void)handle; return -1; }

int ac_drm_cs_syncobj_export_sync_file(ac_drm_device *dev, uint32_t syncobj, int *sync_file_fd)
{ (void)dev; (void)syncobj; (void)sync_file_fd; return -1; }

int ac_drm_cs_syncobj_import_sync_file(ac_drm_device *dev, uint32_t syncobj, int sync_file_fd)
{ (void)dev; (void)syncobj; (void)sync_file_fd; return -1; }

int ac_drm_cs_syncobj_export_sync_file2(ac_drm_device *dev, uint32_t syncobj, uint64_t point,
                                        uint32_t flags, int *sync_file_fd)
{ return -1; }

int ac_drm_cs_syncobj_transfer(ac_drm_device *dev, uint32_t dst_handle, uint64_t dst_point,
                               uint32_t src_handle, uint64_t src_point, uint32_t flags)
{ return -1; }

int ac_drm_cs_submit_raw2(ac_drm_device *dev, uint32_t ctx_id, uint32_t bo_list_handle,
                          int num_chunks, struct drm_amdgpu_cs_chunk *chunks, uint64_t *seq_no)
{ return -1; }

void ac_drm_cs_chunk_fence_info_to_data(uint32_t bo_handle, uint64_t offset,
                                        struct drm_amdgpu_cs_chunk_data *data)
{ (void)bo_handle; (void)offset; (void)data; }

int ac_drm_cs_syncobj_timeline_wait(ac_drm_device *dev, uint32_t *handles, uint64_t *points,
                                    unsigned num_handles, int64_t timeout_nsec, unsigned flags,
                                    uint32_t *first_signaled)
{ return -1; }

int ac_drm_query_info(ac_drm_device *dev, unsigned info_id, unsigned size, void *value)
{ (void)dev; (void)info_id; (void)size; (void)value; return -1; }

int ac_drm_read_mm_registers(ac_drm_device *dev, unsigned dword_offset, unsigned count,
                             uint32_t instance, uint32_t flags, uint32_t *values)
{ return -1; }

int ac_drm_query_hw_ip_count(ac_drm_device *dev, unsigned type, uint32_t *count)
{ (void)dev; (void)type; if (count) *count = 0; return -1; }

int ac_drm_query_hw_ip_info(ac_drm_device *dev, unsigned type, unsigned ip_instance,
                            struct drm_amdgpu_info_hw_ip *info)
{ return -1; }

int ac_drm_query_firmware_version(ac_drm_device *dev, unsigned fw_type, unsigned ip_instance,
                                  unsigned index, uint32_t *version, uint32_t *feature)
{ return -1; }

int ac_drm_query_uq_fw_area_info(ac_drm_device *dev, unsigned type, unsigned ip_instance,
                                 struct drm_amdgpu_info_uq_metadata *info)
{ return -1; }

int ac_drm_query_gpu_info(ac_drm_device *dev, struct amdgpu_gpu_info *info)
{ (void)dev; (void)info; return -1; }

int ac_drm_query_heap_info(ac_drm_device *dev, uint32_t heap, uint32_t flags,
                           struct amdgpu_heap_info *info)
{ return -1; }

int ac_drm_query_sensor_info(ac_drm_device *dev, unsigned sensor_type, unsigned size, void *value)
{ return -1; }

int ac_drm_query_video_caps_info(ac_drm_device *dev, unsigned cap_type, unsigned size, void *value)
{ return -1; }

int ac_drm_query_gpuvm_fault_info(ac_drm_device *dev, unsigned size, void *value)
{ return -1; }

int ac_drm_vm_reserve_vmid(ac_drm_device *dev, uint32_t flags)
{ (void)dev; (void)flags; return -1; }

int ac_drm_vm_unreserve_vmid(ac_drm_device *dev, uint32_t flags)
{ (void)dev; (void)flags; return -1; }

const char *ac_drm_get_marketing_name(ac_drm_device *device)
{ (void)device; return NULL; }

int ac_drm_query_sw_info(ac_drm_device *dev, enum amdgpu_sw_info info, void *value)
{ (void)dev; (void)info; (void)value; return -1; }

int ac_drm_bo_alloc(ac_drm_device *dev, struct amdgpu_bo_alloc_request *alloc_buffer,
                    ac_drm_bo *bo)
{ (void)dev; (void)alloc_buffer; (void)bo; return -1; }

int ac_drm_bo_export(ac_drm_device *dev, ac_drm_bo bo,
                     enum amdgpu_bo_handle_type type, uint32_t *shared_handle)
{ return -1; }

int ac_drm_bo_import(ac_drm_device *dev, enum amdgpu_bo_handle_type type,
                     uint32_t shared_handle, struct ac_drm_bo_import_result *output)
{ return -1; }

int ac_drm_create_bo_from_user_mem(ac_drm_device *dev, void *cpu, uint64_t size, ac_drm_bo *bo)
{ (void)dev; (void)cpu; (void)size; (void)bo; return -1; }

int ac_drm_bo_free(ac_drm_device *dev, ac_drm_bo bo)
{ (void)dev; return -1; }

int ac_drm_bo_cpu_map(ac_drm_device *dev, ac_drm_bo bo, void **cpu)
{ (void)dev; (void)cpu; return -1; }

int ac_drm_bo_cpu_unmap(ac_drm_device *dev, ac_drm_bo bo)
{ (void)dev; return -1; }

int ac_drm_va_range_alloc(ac_drm_device *dev, enum amdgpu_gpu_va_range va_range_type,
                          uint64_t size, uint64_t va_base_alignment, uint64_t va_base_required,
                          uint64_t *va_base_allocated, amdgpu_va_handle *va_range_handle,
                          uint64_t flags)
{ return -1; }

int ac_drm_va_range_free(amdgpu_va_handle va_range_handle)
{ return -1; }

int ac_drm_va_range_query(ac_drm_device *dev, enum amdgpu_gpu_va_range type,
                          uint64_t *start, uint64_t *end)
{ return -1; }

int ac_drm_create_userqueue(ac_drm_device *dev, uint32_t ip_type, uint32_t doorbell_handle,
                            uint32_t doorbell_offset, uint64_t queue_va, uint64_t queue_size,
                            uint64_t wptr_va, uint64_t rptr_va, void *mqd_in, uint32_t flags,
                            uint32_t *queue_id)
{ return -1; }

int ac_drm_free_userqueue(ac_drm_device *dev, uint32_t queue_id)
{ (void)dev; (void)queue_id; return -1; }

int ac_drm_userq_signal(ac_drm_device *dev, struct drm_amdgpu_userq_signal *signal_data)
{ (void)dev; (void)signal_data; return -1; }

int ac_drm_userq_wait(ac_drm_device *dev, struct drm_amdgpu_userq_wait *wait_data)
{ (void)dev; (void)wait_data; return -1; }

int ac_drm_query_pci_bus_info(ac_drm_device *dev, struct radeon_info *info)
{ (void)dev; (void)info; return -1; }

void ac_drm_query_has_vm_always_valid(ac_drm_device *dev, struct radeon_info *info)
{ (void)dev; (void)info; }
