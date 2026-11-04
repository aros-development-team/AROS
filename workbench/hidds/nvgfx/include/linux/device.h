/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DEVICE_H_
#define _LINUX_DEVICE_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/gfp.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/overflow.h>
#include <linux/lockdep.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/kobject.h>
#include <linux/idr.h>
#include <linux/wait.h>
#include <linux/completion.h>

struct device_node;
struct dev_pm_ops;
#include <linux/property.h>
struct kobject { const char *name; };
struct device_driver { const char *name; struct module *owner; const struct dev_pm_ops *pm; const void *of_match_table; };
struct bus_type;
struct class;
struct attribute_group;
struct device_attribute;

/*
 * The AROS side keeps a bare handful of fields; a device is either the
 * one PCI card the driver runs, or a placeholder.
 */
struct device {
    struct kobject kobj;
    struct device *parent;
    struct device_driver *driver;
    const char *init_name;
    void *driver_data;
    void *platform_data;
    struct device_node *of_node;
    struct fwnode_handle *fwnode;
    u64 *dma_mask;
    u64 coherent_dma_mask;
    u64 dma_mask_storage;
    bool is_pci;
    char name[32];
};

const char *dev_name(const struct device *dev);
static inline void *dev_get_drvdata(const struct device *dev)   { return dev->driver_data; }
static inline void dev_set_drvdata(struct device *dev, void *data) { dev->driver_data = data; }
static inline int dev_set_name(struct device *dev, const char *fmt, ...) { return 0; }
static inline bool dev_is_pci(const struct device *dev)          { return dev->is_pci; }
static inline struct device *get_device(struct device *dev)      { return dev; }
static inline void put_device(struct device *dev)                { }
static inline int device_add(struct device *dev)                 { return 0; }
static inline void device_del(struct device *dev)                { }
static inline int device_register(struct device *dev)            { return 0; }
static inline void device_unregister(struct device *dev)         { }
static inline void device_initialize(struct device *dev)         { }
static inline int device_create_file(struct device *dev, const struct device_attribute *attr) { return 0; }
static inline void device_remove_file(struct device *dev, const struct device_attribute *attr) { }
static inline int device_add_groups(struct device *dev, const struct attribute_group **groups) { return 0; }
static inline void device_remove_groups(struct device *dev, const struct attribute_group **groups) { }
static inline int device_link_add(struct device *c, struct device *s, u32 flags) { return 0; }
static inline void device_link_del(void *l) { }
static inline void device_link_remove(void *c, struct device *s) { }
static inline int dev_to_node(struct device *dev)                { return -1; }
static inline bool device_property_read_bool(struct device *dev, const char *name) { return false; }
#define dev_of_node(d)          NULL
#define dev_fwnode(d)           NULL
#define device_property_present(d, n) false
#define DL_FLAG_STATELESS       0
#define DL_FLAG_PM_RUNTIME      0
#define DL_FLAG_RPM_ACTIVE      0
#define DL_FLAG_AUTOREMOVE_CONSUMER 0
#define DL_FLAG_AUTOREMOVE_SUPPLIER 0
#define DL_FLAG_AUTOPROBE_CONSUMER 0
#define DL_FLAG_SYNC_STATE_ONLY 0

/* device managed allocations are plain allocations; nothing unbinds here */
void *devm_kmalloc(struct device *dev, size_t size, gfp_t gfp);
static inline void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp) { return devm_kmalloc(dev, size, gfp | __GFP_ZERO); }
static inline void *devm_kcalloc(struct device *dev, size_t n, size_t size, gfp_t gfp) { return devm_kmalloc(dev, n * size, gfp | __GFP_ZERO); }
static inline void *devm_kmalloc_array(struct device *dev, size_t n, size_t size, gfp_t gfp) { return devm_kmalloc(dev, n * size, gfp); }
void  devm_kfree(struct device *dev, const void *p);
char *devm_kasprintf(struct device *dev, gfp_t gfp, const char *fmt, ...);
char *devm_kstrdup(struct device *dev, const char *s, gfp_t gfp);
#define devm_kmemdup(d, s, l, g)    kmemdup(s, l, g)
typedef void (*dr_release_t)(struct device *dev, void *res);
int  devm_add_action(struct device *dev, void (*action)(void *), void *data);
int  devm_add_action_or_reset(struct device *dev, void (*action)(void *), void *data);
void devm_remove_action(struct device *dev, void (*action)(void *), void *data);
void devm_release_action(struct device *dev, void (*action)(void *), void *data);
void *devres_alloc(dr_release_t release, size_t size, gfp_t gfp);
void devres_add(struct device *dev, void *res);
void devres_free(void *res);
int devres_release(struct device *dev, dr_release_t release, void *match, void *match_data);
#define devres_release_group(d, g)  do { } while (0)
#define devres_open_group(d, g, f)  ((void *)1)
#define devres_close_group(d, g)    do { } while (0)
#define devres_remove_group(d, g)   do { } while (0)

#define dev_printk(level, dev, fmt, ...) printk("%s%s: " fmt, level, dev_name(dev), ##__VA_ARGS__)
#define dev_emerg(dev, fmt, ...)    dev_printk(KERN_EMERG, dev, fmt, ##__VA_ARGS__)
#define dev_alert(dev, fmt, ...)    dev_printk(KERN_ALERT, dev, fmt, ##__VA_ARGS__)
#define dev_crit(dev, fmt, ...)     dev_printk(KERN_CRIT, dev, fmt, ##__VA_ARGS__)
#define dev_err(dev, fmt, ...)      dev_printk(KERN_ERR, dev, fmt, ##__VA_ARGS__)
#define dev_warn(dev, fmt, ...)     dev_printk(KERN_WARNING, dev, fmt, ##__VA_ARGS__)
#define dev_notice(dev, fmt, ...)   dev_printk(KERN_NOTICE, dev, fmt, ##__VA_ARGS__)
#define dev_info(dev, fmt, ...)     dev_printk(KERN_INFO, dev, fmt, ##__VA_ARGS__)
#define dev_dbg(dev, fmt, ...)      no_printk(fmt, ##__VA_ARGS__)
#define dev_vdbg(dev, fmt, ...)     no_printk(fmt, ##__VA_ARGS__)
#define dev_WARN(dev, fmt, ...)     WARN(1, fmt, ##__VA_ARGS__)
#define dev_WARN_ONCE(dev, c, fmt, ...) WARN_ONCE(c, fmt, ##__VA_ARGS__)
#define dev_err_once(dev, fmt, ...) dev_err(dev, fmt, ##__VA_ARGS__)
#define dev_warn_once(dev, fmt, ...) dev_warn(dev, fmt, ##__VA_ARGS__)
#define dev_info_once(dev, fmt, ...) dev_info(dev, fmt, ##__VA_ARGS__)
#define dev_notice_once(dev, fmt, ...) dev_notice(dev, fmt, ##__VA_ARGS__)
#define dev_dbg_once(dev, fmt, ...) dev_dbg(dev, fmt, ##__VA_ARGS__)
#define dev_err_ratelimited(dev, fmt, ...)  dev_err(dev, fmt, ##__VA_ARGS__)
#define dev_warn_ratelimited(dev, fmt, ...) dev_warn(dev, fmt, ##__VA_ARGS__)
#define dev_info_ratelimited(dev, fmt, ...) dev_info(dev, fmt, ##__VA_ARGS__)
#define dev_dbg_ratelimited(dev, fmt, ...)  dev_dbg(dev, fmt, ##__VA_ARGS__)
#define dev_notice_ratelimited(dev, fmt, ...) dev_notice(dev, fmt, ##__VA_ARGS__)
#define dev_err_probe(dev, err, fmt, ...)  ({ dev_err(dev, fmt, ##__VA_ARGS__); (err); })
#define dev_level_once(f, dev, fmt, ...) f(dev, fmt, ##__VA_ARGS__)
#define dev_driver_string(dev)  "nouveau"
#define dev_bus_name(dev)       "pci"

struct attribute { const char *name; umode_t mode; };
struct attribute_group { const char *name; struct attribute **attrs; };
struct device_attribute { struct attribute attr; void *show; void *store; };
#define DEVICE_ATTR(n, m, s, st)    struct device_attribute dev_attr_##n = { .attr = { #n, m }, .show = s, .store = st }
#define DEVICE_ATTR_RO(n)           struct device_attribute dev_attr_##n = { .attr = { #n, 0444 } }
#define DEVICE_ATTR_RW(n)           struct device_attribute dev_attr_##n = { .attr = { #n, 0644 } }
#define ATTRIBUTE_GROUPS(n)         static const struct attribute_group *n##_groups[] = { NULL }
#define __ATTR(n, m, s, st)         { .name = #n, .mode = m }
#define __ATTR_RO(n)                { .name = #n, .mode = 0444 }
#define __ATTR_RW(n)                { .name = #n, .mode = 0644 }
#define __ATTR_NULL                 { .name = NULL }
#define sysfs_create_group(k, g)    (0)
#define sysfs_remove_group(k, g)    do { } while (0)
#define sysfs_create_link(k, t, n)  (0)
#define sysfs_remove_link(k, n)     do { } while (0)
#define sysfs_emit(buf, fmt, ...)   snprintf(buf, 4096, fmt, ##__VA_ARGS__)
#define sysfs_emit_at(buf, at, fmt, ...) snprintf((buf) + (at), 4096 - (at), fmt, ##__VA_ARGS__)
#define kobject_name(k)             ((k)->name)
#define kobject_get(k)              (k)
#define kobject_put(k)              do { } while (0)
#define kobject_uevent_env(k, a, e) (0)
#define kobject_uevent(k, a)        (0)
#define KOBJ_CHANGE                 0
#define KOBJ_ADD                    1
#define KOBJ_REMOVE                 2

/* runtime power management: the device is always awake */
#define pm_runtime_get_sync(dev)            (0)
#define pm_runtime_get(dev)                 (0)
#define pm_runtime_get_noresume(dev)        do { } while (0)
#define pm_runtime_get_if_in_use(dev)       (1)
#define pm_runtime_get_if_active(dev)       (1)
#define pm_runtime_resume_and_get(dev)      (0)
#define pm_runtime_put(dev)                 (0)
#define pm_runtime_put_sync(dev)            (0)
#define pm_runtime_put_autosuspend(dev)     (0)
#define pm_runtime_autosuspend(dev)         (0)
#define __pm_runtime_put_autosuspend(dev)   (0)
#define pm_runtime_put_sync_suspend(dev)    (0)
#define pm_runtime_put_noidle(dev)          do { } while (0)
#define pm_runtime_mark_last_busy(dev)      do { } while (0)
#define pm_runtime_use_autosuspend(dev)     do { } while (0)
#define pm_runtime_dont_use_autosuspend(dev) do { } while (0)
#define pm_runtime_set_autosuspend_delay(dev, d) do { } while (0)
#define pm_runtime_set_active(dev)          (0)
#define pm_runtime_set_suspended(dev)       (0)
#define pm_runtime_enable(dev)              do { } while (0)
#define pm_runtime_disable(dev)             do { } while (0)
#define pm_runtime_allow(dev)               do { } while (0)
#define pm_runtime_forbid(dev)              do { } while (0)
#define pm_runtime_suspended(dev)           (0)
#define pm_runtime_active(dev)              (1)
#define pm_runtime_status_suspended(dev)    (0)
#define pm_runtime_barrier(dev)             (0)
#define pm_runtime_idle(dev)                (0)
#define pm_runtime_force_suspend(dev)       (0)
#define pm_runtime_force_resume(dev)        (0)
#define pm_runtime_set_driver_flags(dev, f) do { } while (0)
#define pm_runtime_no_callbacks(dev)        do { } while (0)
#define pm_runtime_irq_safe(dev)            do { } while (0)
#define pm_runtime_disable_action(dev)      do { } while (0)
#define devm_pm_runtime_enable(dev)         (0)
#define pm_runtime_autosuspend_expiration(dev) (0)
#define pm_runtime_is_irq_safe(dev)         (0)
#define pm_generic_runtime_suspend(dev)     (0)
#define pm_generic_runtime_resume(dev)      (0)
#define pm_suspend_via_firmware()           (0)
#define pm_resume_via_firmware()            (0)
#define device_set_wakeup_capable(d, c)     do { } while (0)
#define device_wakeup_enable(d)             (0)
struct dev_pm_ops {
    int (*prepare)(struct device *dev);
    void (*complete)(struct device *dev);
    int (*suspend)(struct device *dev);
    int (*resume)(struct device *dev);
    int (*freeze)(struct device *dev);
    int (*thaw)(struct device *dev);
    int (*poweroff)(struct device *dev);
    int (*restore)(struct device *dev);
    int (*suspend_late)(struct device *dev);
    int (*resume_early)(struct device *dev);
    int (*freeze_late)(struct device *dev);
    int (*thaw_early)(struct device *dev);
    int (*poweroff_late)(struct device *dev);
    int (*restore_early)(struct device *dev);
    int (*suspend_noirq)(struct device *dev);
    int (*resume_noirq)(struct device *dev);
    int (*freeze_noirq)(struct device *dev);
    int (*thaw_noirq)(struct device *dev);
    int (*poweroff_noirq)(struct device *dev);
    int (*restore_noirq)(struct device *dev);
    int (*runtime_suspend)(struct device *dev);
    int (*runtime_resume)(struct device *dev);
    int (*runtime_idle)(struct device *dev);
};
struct dev_pm_domain { int dummy; };
#define SET_SYSTEM_SLEEP_PM_OPS(s, r)
#define SET_RUNTIME_PM_OPS(s, r, i)
#define SIMPLE_DEV_PM_OPS(name, s, r)       struct dev_pm_ops name = { 0 }
#define DEFINE_SIMPLE_DEV_PM_OPS(name, s, r) struct dev_pm_ops name = { 0 }
#define pm_ptr(p)                           (p)
#define pm_sleep_ptr(p)                     (p)

#endif /* _LINUX_DEVICE_H_ */
