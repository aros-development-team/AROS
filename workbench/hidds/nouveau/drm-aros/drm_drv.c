/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/err.h>
#include <linux/dma-fence.h>
#include <linux/fs.h>

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>
#include <drm/drm_managed.h>
#include <drm/drm_print.h>
#include <drm/drm_mode_config.h>

#include <drm-compat/drm_compat_funcs.h>
#include <drm-aros/drm_aros_pci.h>

#include "drm_internal.h"
#include "drm_crtc_internal.h"

/*
 * The parts of the DRM core that a driver needs from the OS side - device
 * lifetime, managed resources, files and events - without minors, sysfs
 * or a VFS. There is one device and it never goes away while the hidd is
 * loaded, so drm_dev_enter() always succeeds.
 */

/* --- managed resources ---------------------------------------------- */

struct drmres {
    struct list_head node;
    drmres_release_t release;
    const char *name;
    void *data;
    u8 payload[] __aligned(16);
};

static struct drmres *alloc_dr(drmres_release_t release, size_t size, gfp_t gfp)
{
    struct drmres *dr = kzalloc(sizeof(*dr) + size, gfp);
    if (!dr)
        return NULL;
    INIT_LIST_HEAD(&dr->node);
    dr->release = release;
    return dr;
}

static void add_dr(struct drm_device *dev, struct drmres *dr)
{
    unsigned long flags;

    spin_lock_irqsave(&dev->managed.lock, flags);
    list_add(&dr->node, &dev->managed.resources);
    spin_unlock_irqrestore(&dev->managed.lock, flags);
}

int __drmm_add_action(struct drm_device *dev, drmres_release_t action, void *data, const char *name)
{
    struct drmres *dr = alloc_dr(action, 0, GFP_KERNEL);

    if (!dr) {
        drm_dbg_core(dev, "failed to add action %s for %p\n", name, data);
        return -ENOMEM;
    }
    dr->name = name;
    dr->data = data;
    add_dr(dev, dr);
    return 0;
}

int __drmm_add_action_or_reset(struct drm_device *dev, drmres_release_t action, void *data, const char *name)
{
    int ret = __drmm_add_action(dev, action, data, name);
    if (ret)
        action(dev, data);
    return ret;
}

void drmm_release_action(struct drm_device *dev, drmres_release_t action, void *data)
{
    struct drmres *dr, *tmp;
    unsigned long flags;

    spin_lock_irqsave(&dev->managed.lock, flags);
    list_for_each_entry_safe(dr, tmp, &dev->managed.resources, node) {
        if (dr->release == action && dr->data == data) {
            list_del(&dr->node);
            spin_unlock_irqrestore(&dev->managed.lock, flags);
            action(dev, data);
            kfree(dr);
            return;
        }
    }
    spin_unlock_irqrestore(&dev->managed.lock, flags);
}

void drm_managed_release(struct drm_device *dev)
{
    struct drmres *dr, *tmp;

    list_for_each_entry_safe(dr, tmp, &dev->managed.resources, node) {
        if (dr->release)
            dr->release(dev, dr->data);
        list_del(&dr->node);
        kfree(dr);
    }
}

void drmm_add_final_kfree(struct drm_device *dev, void *container)
{
    dev->managed.final_kfree = container;
}

static void drmm_kmalloc_release(struct drm_device *dev, void *data)
{
}

void *drmm_kmalloc(struct drm_device *dev, size_t size, gfp_t gfp)
{
    struct drmres *dr = alloc_dr(drmm_kmalloc_release, size, gfp);

    if (!dr)
        return NULL;
    dr->name = "kmalloc";
    dr->data = dr->payload;
    add_dr(dev, dr);
    return dr->payload;
}

char *drmm_kstrdup(struct drm_device *dev, const char *s, gfp_t gfp)
{
    size_t size;
    char *buf;

    if (!s)
        return NULL;
    size = strlen(s) + 1;
    buf = drmm_kmalloc(dev, size, gfp);
    if (buf)
        memcpy(buf, s, size);
    return buf;
}

void drmm_kfree(struct drm_device *dev, void *data)
{
    struct drmres *dr, *tmp;
    unsigned long flags;

    if (!data)
        return;
    spin_lock_irqsave(&dev->managed.lock, flags);
    list_for_each_entry_safe(dr, tmp, &dev->managed.resources, node) {
        if (dr->data == data) {
            list_del(&dr->node);
            spin_unlock_irqrestore(&dev->managed.lock, flags);
            kfree(dr);
            return;
        }
    }
    spin_unlock_irqrestore(&dev->managed.lock, flags);
}

void __drmm_mutex_release(struct drm_device *dev, void *res)
{
    mutex_destroy(res);
}

void __drmm_workqueue_release(struct drm_device *device, void *res)
{
    destroy_workqueue(res);
}

/* --- device -------------------------------------------------------- */

static struct drm_minor *drm_minor_alloc(struct drm_device *dev, int type)
{
    struct drm_minor *minor = drmm_kzalloc(dev, sizeof(*minor), GFP_KERNEL);
    if (!minor)
        return NULL;
    minor->type = type;
    minor->dev = dev;
    minor->index = type == DRM_MINOR_RENDER ? 128 : 0;
    return minor;
}

static int drm_dev_init(struct drm_device *dev, const struct drm_driver *driver, struct device *parent)
{
    int ret;

    kref_init(&dev->ref);
    dev->dev = parent;
    dev->driver = driver;

    INIT_LIST_HEAD(&dev->managed.resources);
    spin_lock_init(&dev->managed.lock);

    dev->driver_features = ~0u;

    dev->anon_inode = drmm_kzalloc(dev, sizeof(*dev->anon_inode), GFP_KERNEL);
    if (!dev->anon_inode)
        return -ENOMEM;
    dev->anon_inode->i_mapping = &dev->anon_inode->i_data;

    INIT_LIST_HEAD(&dev->filelist);
    INIT_LIST_HEAD(&dev->filelist_internal);
    INIT_LIST_HEAD(&dev->clientlist);
    INIT_LIST_HEAD(&dev->vblank_event_list);

    spin_lock_init(&dev->event_lock);
    mutex_init(&dev->filelist_mutex);
    mutex_init(&dev->clientlist_mutex);
    mutex_init(&dev->master_mutex);
    raw_spin_lock_init(&dev->mode_config.panic_lock);

    if (drm_core_check_feature(dev, DRIVER_RENDER)) {
        dev->render = drm_minor_alloc(dev, DRM_MINOR_RENDER);
        if (!dev->render)
            return -ENOMEM;
    }
    dev->primary = drm_minor_alloc(dev, DRM_MINOR_PRIMARY);
    if (!dev->primary)
        return -ENOMEM;

    if (drm_core_check_feature(dev, DRIVER_GEM)) {
        ret = drm_gem_init(dev);
        if (ret) {
            DRM_ERROR("Cannot initialize graphics execution manager (GEM)\n");
            return ret;
        }
    }

    dev->unique = drmm_kstrdup(dev, dev_name(parent), GFP_KERNEL);
    if (!dev->unique)
        return -ENOMEM;

    return 0;
}

struct drm_device *drm_dev_alloc(const struct drm_driver *driver, struct device *parent)
{
    struct drm_device *dev;
    int ret;

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return ERR_PTR(-ENOMEM);

    ret = drm_dev_init(dev, driver, parent);
    if (ret) {
        drm_managed_release(dev);
        kfree(dev);
        return ERR_PTR(ret);
    }

    drmm_add_final_kfree(dev, dev);
    return dev;
}

void *__devm_drm_dev_alloc(struct device *parent, const struct drm_driver *driver, size_t size, size_t offset)
{
    void *container;
    struct drm_device *drm;
    int ret;

    container = kzalloc(size, GFP_KERNEL);
    if (!container)
        return ERR_PTR(-ENOMEM);

    drm = container + offset;
    ret = drm_dev_init(drm, driver, parent);
    if (ret) {
        kfree(container);
        return ERR_PTR(ret);
    }
    drmm_add_final_kfree(drm, container);
    return container;
}

static void drm_dev_release(struct kref *ref)
{
    struct drm_device *dev = container_of(ref, struct drm_device, ref);

    if (dev->driver->release)
        dev->driver->release(dev);

    drm_managed_release(dev);

    if (dev->managed.final_kfree)
        kfree(dev->managed.final_kfree);
}

void drm_dev_get(struct drm_device *dev)
{
    if (dev)
        kref_get(&dev->ref);
}

void drm_dev_put(struct drm_device *dev)
{
    if (dev)
        kref_put(&dev->ref, drm_dev_release);
}

int drm_dev_register(struct drm_device *dev, unsigned long flags)
{
    const struct drm_driver *driver = dev->driver;

    dev->registered = true;

    if (drm_core_check_feature(dev, DRIVER_MODESET))
        drm_modeset_register_all(dev);

    DRM_INFO("Initialized %s %d.%d.%d for %s\n",
             driver->name, driver->major, driver->minor, driver->patchlevel,
             dev->unique);
    return 0;
}

void drm_dev_unregister(struct drm_device *dev)
{
    dev->registered = false;
    if (drm_core_check_feature(dev, DRIVER_MODESET))
        drm_modeset_unregister_all(dev);
}

void drm_dev_unplug(struct drm_device *dev)
{
    dev->unplugged = true;
    drm_dev_unregister(dev);
}

bool drm_dev_enter(struct drm_device *dev, int *idx)
{
    *idx = 0;
    return !dev->unplugged;
}

void drm_dev_exit(int idx)
{
}

void drm_put_dev(struct drm_device *dev)
{
    if (!dev)
        return;
    drm_dev_unregister(dev);
    drm_dev_put(dev);
}

/* --- files ---------------------------------------------------------- */

struct drm_file *drm_file_alloc(struct drm_minor *minor)
{
    struct drm_device *dev = minor->dev;
    struct drm_file *file;
    int ret;

    file = kzalloc(sizeof(*file), GFP_KERNEL);
    if (!file)
        return ERR_PTR(-ENOMEM);

    file->minor = minor;
    file->authenticated = true;
    file->is_master = true;
    file->universal_planes = true;
    file->atomic = true;
    file->aspect_ratio_allowed = true;

    INIT_LIST_HEAD(&file->lhead);
    INIT_LIST_HEAD(&file->fbs);
    mutex_init(&file->fbs_lock);
    INIT_LIST_HEAD(&file->blobs);
    INIT_LIST_HEAD(&file->pending_event_list);
    INIT_LIST_HEAD(&file->event_list);
    init_waitqueue_head(&file->event_wait);
    file->event_space = 4096;
    mutex_init(&file->event_read_lock);
    spin_lock_init(&file->master_lookup_lock);
    mutex_init(&file->client_name_lock);

    if (drm_core_check_feature(dev, DRIVER_GEM))
        drm_gem_open(dev, file);

    if (dev->driver->open) {
        ret = dev->driver->open(dev, file);
        if (ret < 0) {
            drm_gem_release(dev, file);
            kfree(file);
            return ERR_PTR(ret);
        }
    }

    mutex_lock(&dev->filelist_mutex);
    list_add(&file->lhead, &dev->filelist);
    mutex_unlock(&dev->filelist_mutex);
    return file;
}

void drm_file_free(struct drm_file *file)
{
    struct drm_device *dev;
    struct drm_pending_event *e, *et;

    if (!file)
        return;
    dev = file->minor->dev;

    mutex_lock(&dev->filelist_mutex);
    list_del(&file->lhead);
    mutex_unlock(&dev->filelist_mutex);

    if (dev->driver->postclose)
        dev->driver->postclose(dev, file);

    if (drm_core_check_feature(dev, DRIVER_MODESET))
        drm_fb_release(file);

    if (drm_core_check_feature(dev, DRIVER_GEM))
        drm_gem_release(dev, file);

    list_for_each_entry_safe(e, et, &file->pending_event_list, pending_link) {
        list_del(&e->pending_link);
        e->file_priv = NULL;
    }
    list_for_each_entry_safe(e, et, &file->event_list, link) {
        list_del(&e->link);
        kfree(e);
    }

    kfree(file);
}

/* events: nobody reads them, so a sent event is a completed event */

int drm_event_reserve_init_locked(struct drm_device *dev, struct drm_file *file_priv,
    struct drm_pending_event *p, struct drm_event *e)
{
    if (file_priv->event_space < e->length)
        return -ENOMEM;
    file_priv->event_space -= e->length;
    p->event = e;
    list_add(&p->pending_link, &file_priv->pending_event_list);
    p->file_priv = file_priv;
    return 0;
}

int drm_event_reserve_init(struct drm_device *dev, struct drm_file *file_priv,
    struct drm_pending_event *p, struct drm_event *e)
{
    unsigned long flags;
    int ret;

    spin_lock_irqsave(&dev->event_lock, flags);
    ret = drm_event_reserve_init_locked(dev, file_priv, p, e);
    spin_unlock_irqrestore(&dev->event_lock, flags);
    return ret;
}

void drm_event_cancel_free(struct drm_device *dev, struct drm_pending_event *p)
{
    unsigned long flags;

    spin_lock_irqsave(&dev->event_lock, flags);
    if (p->file_priv) {
        p->file_priv->event_space += p->event->length;
        list_del(&p->pending_link);
    }
    spin_unlock_irqrestore(&dev->event_lock, flags);

    if (p->fence)
        dma_fence_put(p->fence);
    kfree(p);
}

void drm_send_event_locked(struct drm_device *dev, struct drm_pending_event *e)
{
    if (e->completion) {
        complete_all(e->completion);
        e->completion_release(e->completion);
        e->completion = NULL;
    }
    if (e->fence) {
        dma_fence_signal(e->fence);
        dma_fence_put(e->fence);
    }
    if (!e->file_priv) {
        kfree(e);
        return;
    }
    e->file_priv->event_space += e->event->length;
    list_del(&e->pending_link);
    kfree(e);
}

void drm_send_event(struct drm_device *dev, struct drm_pending_event *e)
{
    unsigned long flags;

    spin_lock_irqsave(&dev->event_lock, flags);
    drm_send_event_locked(dev, e);
    spin_unlock_irqrestore(&dev->event_lock, flags);
}

void drm_send_event_timestamp_locked(struct drm_device *dev, struct drm_pending_event *e, ktime_t timestamp)
{
    drm_send_event_locked(dev, e);
}

/* --- driver bring-up ---------------------------------------------------- */

int nouveau_aros_probe(struct pci_dev *pdev, struct drm_device **pdrm_dev);
struct drm_device *current_drm_device;
BOOL workqueue_init(void);

/*
 * Bring-up is split in two so that the caller gets a look at the card -
 * and in particular at its BARs - after it has been found but before it
 * has been touched. Whoever the firmware left driving this hardware has
 * to be shut down in that gap: nouveau_init_probe() reprograms the card,
 * and any boot-mode driver still writing through the old apertures
 * afterwards writes into the new owner's state.
 */
struct pci_dev *nouveau_init_findcard(void)
{
    if (drm_aros_pci_init())
        return NULL;

    return drm_aros_pci_find_supported_video_card();
}

/*
 * The probe runs on its own process: whoever installs the driver has a
 * default-sized stack, and nvkm's bring-up (GSP in particular) is deep.
 */
struct probe_start {
    struct pci_dev *pdev;
    struct Task *parent;
    ULONG sigbit;
    int ret;
};

static void probe_main(void)
{
    struct probe_start *ps = FindTask(NULL)->tc_UserData;

    ps->ret = nouveau_aros_probe(ps->pdev, &current_drm_device);
    Signal(ps->parent, 1UL << ps->sigbit);
}

int nouveau_init_probe(struct pci_dev *pdev)
{
    struct probe_start ps;

    if (!pdev)
        return -1;

    if (!workqueue_init())
        return -1;

    bug("\003\n"); /* Tell vga text mode debug output to die */

    ps.pdev = pdev;
    ps.parent = FindTask(NULL);
    ps.ret = -1;
    ps.sigbit = AllocSignal(-1);
    if (ps.sigbit == (ULONG)-1)
        return -1;

    if (!CreateNewProcTags(
            NP_Name, (IPTR)"nouveau probe",
            NP_Entry, (IPTR)probe_main,
            NP_StackSize, 1024 * 1024,
            NP_UserData, (IPTR)&ps,
            TAG_DONE)) {
        FreeSignal(ps.sigbit);
        return -1;
    }
    Wait(1UL << ps.sigbit);
    FreeSignal(ps.sigbit);

    return ps.ret ? -1 : 0;
}

int nouveau_init(void)
{
    return nouveau_init_probe(nouveau_init_findcard());
}
