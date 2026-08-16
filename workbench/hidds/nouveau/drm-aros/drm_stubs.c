/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/iosys-map.h>

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>
#include <drm/drm_prime.h>
#include <drm/drm_vma_manager.h>
#include <drm/drm_vblank.h>
#include <drm/drm_crtc.h>
#include <drm/drm_connector.h>
#include <drm/drm_print.h>
#include <drm/drm_client.h>
#include <drm/drm_cache.h>
#include <drm/drm_gem_ttm_helper.h>
#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_range_manager.h>
#include <drm/ttm/ttm_backup.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gpuvm.h>
#include <drm/gpu_scheduler.h>
#include <linux/dma-fence.h>

/*
 * DRM core services that either do not apply here (userspace mmap
 * offsets, sysfs, debugfs, buffer sharing, vblank counting) or that live
 * in files this port does not import. Each answers as if the facility
 * were present and idle.
 */

/* --- mmap offsets: nobody maps by offset, hand out unique fake ones ---- */

void drm_vma_offset_manager_init(struct drm_vma_offset_manager *mgr, unsigned long page_offset, unsigned long size)
{
    memset(mgr, 0, sizeof(*mgr));
    rwlock_init(&mgr->vm_lock);
}

void drm_vma_offset_manager_destroy(struct drm_vma_offset_manager *mgr)
{
}

int drm_vma_offset_add(struct drm_vma_offset_manager *mgr, struct drm_vma_offset_node *node, unsigned long pages)
{
    static unsigned long next_start = 0x1000;

    if (!node->vm_node.size) {
        node->vm_node.start = next_start;
        node->vm_node.size = pages;
        next_start += pages ? pages : 1;
    }
    return 0;
}

void drm_vma_offset_remove(struct drm_vma_offset_manager *mgr, struct drm_vma_offset_node *node)
{
    node->vm_node.size = 0;
}

struct drm_vma_offset_node *drm_vma_offset_lookup_locked(struct drm_vma_offset_manager *mgr, unsigned long start, unsigned long pages)
{
    return NULL;
}

int drm_vma_node_allow(struct drm_vma_offset_node *node, struct drm_file *tag)
{
    return 0;
}

int drm_vma_node_allow_once(struct drm_vma_offset_node *node, struct drm_file *tag)
{
    return 0;
}

void drm_vma_node_revoke(struct drm_vma_offset_node *node, struct drm_file *tag)
{
}

bool drm_vma_node_is_allowed(struct drm_vma_offset_node *node, struct drm_file *tag)
{
    return true;
}

/* --- vblank: no counter, events complete at once ------------------------ */

int drm_vblank_init(struct drm_device *dev, unsigned int num_crtcs)
{
    dev->num_crtcs = num_crtcs;
    return 0;
}

int drm_crtc_vblank_get(struct drm_crtc *crtc)          { return 0; }
void drm_crtc_vblank_put(struct drm_crtc *crtc)         { }
void drm_crtc_vblank_on(struct drm_crtc *crtc)          { }
void drm_crtc_vblank_off(struct drm_crtc *crtc)         { }
void drm_crtc_vblank_reset(struct drm_crtc *crtc)       { }
void drm_crtc_vblank_restore(struct drm_crtc *crtc)     { }
void drm_crtc_wait_one_vblank(struct drm_crtc *crtc)    { }
void drm_crtc_vblank_on_config(struct drm_crtc *crtc, const struct drm_vblank_crtc_config *config) { }
u64 drm_crtc_vblank_count(struct drm_crtc *crtc)        { return 0; }
u64 drm_crtc_accurate_vblank_count(struct drm_crtc *crtc) { return 0; }
u64 drm_crtc_vblank_count_and_time(struct drm_crtc *crtc, ktime_t *vblanktime)
{
    if (vblanktime)
        *vblanktime = ktime_get();
    return 0;
}
bool drm_crtc_handle_vblank(struct drm_crtc *crtc)      { return false; }
bool drm_handle_vblank(struct drm_device *dev, unsigned int pipe) { return false; }
void drm_crtc_set_max_vblank_count(struct drm_crtc *crtc, u32 max_vblank_count) { }
wait_queue_head_t *drm_crtc_vblank_waitqueue(struct drm_crtc *crtc) { return NULL; }
struct drm_vblank_crtc *drm_crtc_vblank_crtc(struct drm_crtc *crtc) { return NULL; }
void drm_vblank_restore(struct drm_device *dev, unsigned int pipe) { }
bool drm_vblank_passed(u64 seq, u64 ref) { return true; }
void drm_calc_timestamping_constants(struct drm_crtc *crtc, const struct drm_display_mode *mode) { }
bool drm_crtc_vblank_helper_get_vblank_timestamp(struct drm_crtc *crtc, int *max_error, ktime_t *vblank_time, bool in_vblank_irq)
{
    *vblank_time = ktime_get();
    return true;
}
bool drm_crtc_vblank_helper_get_vblank_timestamp_internal(struct drm_crtc *crtc, int *max_error, ktime_t *vblank_time,
    bool in_vblank_irq, drm_vblank_get_scanout_position_func get_scanout_position)
{
    *vblank_time = ktime_get();
    return true;
}

void drm_crtc_send_vblank_event(struct drm_crtc *crtc, struct drm_pending_vblank_event *e)
{
    struct drm_device *dev = crtc->dev;

    e->pipe = drm_crtc_index(crtc);
    e->event.vbl.sequence = 0;
    drm_send_event_locked(dev, &e->base);
}

void drm_crtc_arm_vblank_event(struct drm_crtc *crtc, struct drm_pending_vblank_event *e)
{
    drm_crtc_send_vblank_event(crtc, e);
}

void drm_wait_one_vblank(struct drm_device *dev, unsigned int pipe) { }
void drm_vblank_work_init(struct drm_vblank_work *work, struct drm_crtc *crtc, void (*func)(struct kthread_work *work)) { }
int drm_vblank_work_schedule(struct drm_vblank_work *work, u64 count, bool nextonmiss) { return 0; }
bool drm_vblank_work_cancel_sync(struct drm_vblank_work *work) { return false; }
void drm_vblank_work_flush(struct drm_vblank_work *work) { }
void drm_vblank_work_flush_all(struct drm_crtc *crtc) { }

bool drm_dev_has_vblank(const struct drm_device *dev) { return false; }
int drm_crtc_next_vblank_start(struct drm_crtc *crtc, ktime_t *vblanktime) { *vblanktime = ktime_get(); return 0; }

/* --- framebuffers over gem objects (drm_gem_framebuffer_helper) ---------- */

int drm_gem_fb_create_handle(struct drm_framebuffer *fb, struct drm_file *file, unsigned int *handle)
{
    return drm_gem_handle_create(file, fb->obj[0], handle);
}

void drm_gem_fb_destroy(struct drm_framebuffer *fb)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(fb->obj); i++)
        drm_gem_object_put(fb->obj[i]);
    drm_framebuffer_cleanup(fb);
    kfree(fb);
}

int drm_get_panel_orientation_quirk(int width, int height) { return -1; }
void drm_gpuvm_bo_evict(struct drm_gpuvm_bo *vm_bo, bool evict) { }
void drm_sched_entity_fini(struct drm_sched_entity *entity) { }
int drm_prime_add_buf_handle(struct drm_prime_file_private *prime_fpriv, struct dma_buf *dma_buf, uint32_t handle) { return -ENOSYS; }
bool dma_fence_is_chain(struct dma_fence *fence) { return false; }

/* --- ttm pieces that live in files this port does not build -------------- */

struct file *ttm_backup_shmem_create(loff_t size) { return ERR_PTR(-ENOSYS); }
void ttm_backup_fini(struct file *backup) { }
vm_fault_t ttm_bo_vm_reserve(struct ttm_buffer_object *bo, struct vm_fault *vmf) { return VM_FAULT_SIGBUS; }
vm_fault_t ttm_bo_vm_fault_reserved(struct vm_fault *vmf, pgprot_t prot, pgoff_t num_prefault) { return VM_FAULT_SIGBUS; }
void ttm_bo_vm_open(struct vm_area_struct *vma) { }
void ttm_bo_vm_close(struct vm_area_struct *vma) { }
int ttm_bo_vm_access(struct vm_area_struct *vma, unsigned long addr, void *buf, int len, int write) { return -EIO; }

/* --- sysfs / debugfs / clients ------------------------------------------ */

int drm_sysfs_connector_add(struct drm_connector *connector) { return 0; }
int drm_sysfs_connector_add_late(struct drm_connector *connector) { return 0; }
void drm_sysfs_connector_remove_early(struct drm_connector *connector) { }
void drm_sysfs_connector_remove(struct drm_connector *connector) { }
void drm_sysfs_hotplug_event(struct drm_device *dev) { }
void drm_sysfs_connector_hotplug_event(struct drm_connector *connector) { }
void drm_sysfs_connector_property_event(struct drm_connector *connector, struct drm_property *property) { }
void drm_sysfs_connector_status_event(struct drm_connector *connector, struct drm_property *property) { }
void drm_sysfs_lease_event(struct drm_device *dev) { }
struct device *drm_sysfs_minor_alloc(struct drm_minor *minor) { return NULL; }
void drm_class_device_unregister(struct device *dev) { }
int drm_class_device_register(struct device *dev) { return 0; }

void drm_debugfs_create_files(const struct drm_info_list *files, int count, struct dentry *root, struct drm_minor *minor) { }
int drm_debugfs_remove_files(const struct drm_info_list *files, int count, struct dentry *root, struct drm_minor *minor) { return 0; }
void drm_debugfs_add_file(struct drm_device *dev, const char *name, int (*show)(struct seq_file *, void *), void *data) { }
void drm_debugfs_add_files(struct drm_device *dev, const struct drm_debugfs_info *files, int count) { }
void drm_debugfs_connector_add(struct drm_connector *connector) { }
void drm_debugfs_connector_remove(struct drm_connector *connector) { }
void drm_debugfs_crtc_add(struct drm_crtc *crtc) { }
void drm_debugfs_crtc_remove(struct drm_crtc *crtc) { }
void drm_debugfs_encoder_add(struct drm_encoder *encoder) { }
void drm_debugfs_encoder_remove(struct drm_encoder *encoder) { }
void drm_debugfs_crtc_crc_add(struct drm_crtc *crtc) { }
void drm_debugfs_dev_fini(struct drm_device *dev) { }
void drm_debugfs_dev_register(struct drm_device *dev) { }
int drm_debugfs_register(struct drm_minor *minor, int minor_id, struct dentry *root) { return 0; }
void drm_debugfs_unregister(struct drm_minor *minor) { }
void drm_debugfs_clients_add(struct drm_file *file) { }
void drm_debugfs_clients_remove(struct drm_file *file) { }
void drm_debugfs_root_init(void) { }
void drm_debugfs_root_fini(void) { }

void drm_client_setup(struct drm_device *dev, const struct drm_format_info *format) { }
void drm_client_setup_with_fourcc(struct drm_device *dev, u32 fourcc) { }
void drm_client_setup_with_color_mode(struct drm_device *dev, unsigned int color_mode) { }
void drm_client_dev_hotplug(struct drm_device *dev) { }
void drm_client_dev_restore(struct drm_device *dev) { }
void drm_client_dev_suspend(struct drm_device *dev, bool holds_console_lock) { }
void drm_client_dev_resume(struct drm_device *dev, bool holds_console_lock) { }
void drm_client_dev_unregister(struct drm_device *dev) { }
void drm_client_release(struct drm_client_dev *client) { }
int drm_client_init(struct drm_device *dev, struct drm_client_dev *client, const char *name, const struct drm_client_funcs *funcs) { return -ENODEV; }
void drm_client_register(struct drm_client_dev *client) { }
int drm_client_modeset_probe(struct drm_client_dev *client, unsigned int width, unsigned int height) { return -ENODEV; }
int drm_client_modeset_commit(struct drm_client_dev *client) { return -ENODEV; }
int drm_client_modeset_dpms(struct drm_client_dev *client, int mode) { return -ENODEV; }
bool drm_client_rotation(struct drm_mode_set *modeset, unsigned int *rotation) { return false; }

/* --- cache flushing: pages are not cached from the card's view ------------ */

void drm_clflush_pages(struct page *pages[], unsigned long num_pages) { }
void drm_clflush_sg(struct sg_table *st) { }
void drm_clflush_virt_range(void *addr, unsigned long length) { }
bool drm_need_swiotlb(int dma_bits) { return false; }
void drm_memcpy_from_wc(struct iosys_map *dst, const struct iosys_map *src, unsigned long len)
{
    if (src->is_iomem)
        memcpy_fromio(dst->vaddr, src->vaddr_iomem, len);
    else
        memcpy(dst->vaddr, src->vaddr, len);
}
void drm_memcpy_init_early(void) { }

/* --- ioctl plumbing not used: everything is a direct call --------------- */

int drm_invalid_op(struct drm_device *dev, void *data, struct drm_file *file_priv) { return -EINVAL; }
int drm_noop(struct drm_device *dev, void *data, struct drm_file *file_priv) { return 0; }
long drm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) { return -EINVAL; }
long drm_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) { return -EINVAL; }
bool drm_ioctl_flags(unsigned int nr, unsigned int *flags) { return false; }
int drm_open(struct inode *inode, struct file *filp) { return -ENODEV; }
int drm_release(struct inode *inode, struct file *filp) { return 0; }
int drm_release_noglobal(struct inode *inode, struct file *filp) { return 0; }
ssize_t drm_read(struct file *filp, char __user *buffer, size_t count, loff_t *offset) { return -EINVAL; }
__poll_t drm_poll(struct file *filp, struct poll_table_struct *wait) { return 0; }
bool drm_is_current_master(struct drm_file *fpriv) { return true; }
struct drm_master *drm_master_get(struct drm_master *master) { return master; }
void drm_master_put(struct drm_master **master) { }
struct drm_master *drm_file_get_master(struct drm_file *file_priv) { return NULL; }
void drm_master_release(struct drm_file *file_priv) { }
int drm_master_open(struct drm_file *file_priv) { return 0; }
void drm_master_internal_release(struct drm_device *dev) { }
bool drm_master_internal_acquire(struct drm_device *dev) { return true; }
void drm_show_memory_stats(struct drm_printer *p, struct drm_file *file) { }
void drm_print_memory_stats(struct drm_printer *p, const struct drm_memory_stats *stats, enum drm_gem_object_status supported_status, const char *region) { }

/* --- buffer sharing does not exist ------------------------------------- */

struct dma_buf *drm_gem_prime_export(struct drm_gem_object *obj, int flags) { return ERR_PTR(-ENOSYS); }
struct drm_gem_object *drm_gem_prime_import(struct drm_device *dev, struct dma_buf *dma_buf) { return ERR_PTR(-ENOSYS); }
struct drm_gem_object *drm_gem_prime_import_dev(struct drm_device *dev, struct dma_buf *dma_buf, struct device *attach_dev) { return ERR_PTR(-ENOSYS); }
int drm_gem_prime_fd_to_handle(struct drm_device *dev, struct drm_file *file_priv, int prime_fd, uint32_t *handle) { return -ENOSYS; }
int drm_gem_prime_handle_to_fd(struct drm_device *dev, struct drm_file *file_priv, uint32_t handle, uint32_t flags, int *prime_fd) { return -ENOSYS; }
int drm_gem_prime_mmap(struct drm_gem_object *obj, struct vm_area_struct *vma) { return -ENOSYS; }
struct sg_table *drm_prime_pages_to_sg(struct drm_device *dev, struct page **pages, unsigned int nr_pages) { return ERR_PTR(-ENOSYS); }
int drm_prime_sg_to_page_array(struct sg_table *sgt, struct page **pages, int max_pages) { return -ENOSYS; }
int drm_prime_sg_to_dma_addr_array(struct sg_table *sgt, dma_addr_t *addrs, int max_pages) { return -ENOSYS; }
void drm_prime_gem_destroy(struct drm_gem_object *obj, struct sg_table *sg) { }
void drm_prime_init_file_private(struct drm_prime_file_private *prime_fpriv) { }
void drm_prime_destroy_file_private(struct drm_prime_file_private *prime_fpriv) { }
void drm_prime_remove_buf_handle(struct drm_prime_file_private *prime_fpriv, uint32_t handle) { }
unsigned long drm_prime_get_contiguous_size(struct sg_table *sgt) { return 0; }
int drm_gem_map_attach(struct dma_buf *dma_buf, struct dma_buf_attachment *attach) { return -ENOSYS; }
void drm_gem_map_detach(struct dma_buf *dma_buf, struct dma_buf_attachment *attach) { }
struct sg_table *drm_gem_map_dma_buf(struct dma_buf_attachment *attach, enum dma_data_direction dir) { return ERR_PTR(-ENOSYS); }
void drm_gem_unmap_dma_buf(struct dma_buf_attachment *attach, struct sg_table *sgt, enum dma_data_direction dir) { }
int drm_gem_dmabuf_vmap(struct dma_buf *dma_buf, struct iosys_map *map) { return -ENOSYS; }
void drm_gem_dmabuf_vunmap(struct dma_buf *dma_buf, struct iosys_map *map) { }
int drm_gem_dmabuf_mmap(struct dma_buf *dma_buf, struct vm_area_struct *vma) { return -ENOSYS; }
void drm_gem_dmabuf_release(struct dma_buf *dma_buf) { }

/* --- ttm-backed gem objects: vmap through ttm, no userspace mmap ---------- */

int drm_gem_ttm_vmap(struct drm_gem_object *gem, struct iosys_map *map)
{
    struct ttm_buffer_object *bo = drm_gem_ttm_of_gem(gem);
    return ttm_bo_vmap(bo, map);
}

void drm_gem_ttm_vunmap(struct drm_gem_object *gem, struct iosys_map *map)
{
    struct ttm_buffer_object *bo = drm_gem_ttm_of_gem(gem);
    ttm_bo_vunmap(bo, map);
}

int drm_gem_ttm_mmap(struct drm_gem_object *gem, struct vm_area_struct *vma)
{
    return -ENOSYS;
}

int drm_gem_ttm_dumb_map_offset(struct drm_file *file, struct drm_device *dev, uint32_t handle, uint64_t *offset)
{
    struct drm_gem_object *gem = drm_gem_object_lookup(file, handle);
    if (!gem)
        return -ENOENT;
    *offset = drm_vma_node_offset_addr(&gem->vma_node);
    drm_gem_object_put(gem);
    return 0;
}

void drm_gem_ttm_print_info(struct drm_printer *p, unsigned int indent, const struct drm_gem_object *gem)
{
}

/* --- ttm range manager (only the AGP aperture would need it) ------------- */

int ttm_range_man_init_nocheck(struct ttm_device *bdev, unsigned type, bool use_tt, unsigned long p_size)
{
    return -ENOSYS;
}

int ttm_range_man_fini_nocheck(struct ttm_device *bdev, unsigned type)
{
    return 0;
}
