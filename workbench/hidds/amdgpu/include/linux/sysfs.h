/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_SYSFS_H_
#define _LINUX_SYSFS_H_

#include <linux/types.h>
#include <linux/stringify.h>

struct attribute {
    const char *name;
    umode_t mode;
};

struct bin_attribute;

struct attribute_group {
    const char *name;
    umode_t (*is_visible)(struct kobject *, struct attribute *, int);
    umode_t (*is_bin_visible)(struct kobject *, const struct bin_attribute *, int);
    struct attribute **attrs;
    const struct bin_attribute *const *bin_attrs;
};

struct sysfs_ops {
    ssize_t (*show)(struct kobject *, struct attribute *, char *);
    ssize_t (*store)(struct kobject *, struct attribute *, const char *, size_t);
};

#define __ATTR(_name, _mode, _show, _store) { \
    .attr = { .name = __stringify(_name), .mode = (_mode) }, .show = _show, .store = _store }
#define __ATTR_RO(_name)        { .attr = { .name = __stringify(_name), .mode = 0444 }, .show = _name##_show }
#define __ATTR_RO_MODE(_name, _mode) { .attr = { .name = __stringify(_name), .mode = (_mode) }, .show = _name##_show }
#define __ATTR_WO(_name)        { .attr = { .name = __stringify(_name), .mode = 0200 }, .store = _name##_store }
#define __ATTR_RW(_name)        __ATTR(_name, 0644, _name##_show, _name##_store)
#define __ATTR_RW_MODE(_name, _mode) __ATTR(_name, _mode, _name##_show, _name##_store)
#define __ATTR_NULL             { .attr = { .name = NULL } }
#define ATTRIBUTE_GROUPS(_name) \
    static const struct attribute_group _name##_group = { .attrs = _name##_attrs }; \
    static const struct attribute_group *_name##_groups[] = { &_name##_group, NULL }
#define BIN_ATTR(_name, _mode, _read, _write, _size) \
    struct bin_attribute bin_attr_##_name = { .attr = { .name = __stringify(_name), .mode = (_mode) }, \
    .read = _read, .write = _write, .size = _size }

struct kobject;
struct file;

struct bin_attribute {
    struct attribute attr;
    size_t size;
    void *private;
    ssize_t (*read)(struct file *, struct kobject *, const struct bin_attribute *, char *, loff_t, size_t);
    ssize_t (*write)(struct file *, struct kobject *, const struct bin_attribute *, char *, loff_t, size_t);
};

#define sysfs_attr_init(attr)   do { } while (0)
#define sysfs_bin_attr_init(a)  do { } while (0)

static inline int sysfs_create_groups(struct kobject *kobj, const struct attribute_group **grps) { return 0; }
static inline void sysfs_remove_groups(struct kobject *kobj, const struct attribute_group **grps) { }
#define sysfs_create_group(k, g)    (0)
#define sysfs_remove_group(k, g)    do { } while (0)
static inline int sysfs_update_group(struct kobject *kobj, const struct attribute_group *grp) { return 0; }
static inline int sysfs_create_files(struct kobject *kobj, const struct attribute * const *attr) { return 0; }
static inline void sysfs_remove_files(struct kobject *kobj, const struct attribute * const *attr) { }
static inline int sysfs_create_file(struct kobject *kobj, const struct attribute *attr) { return 0; }
static inline void sysfs_remove_file(struct kobject *kobj, const struct attribute *attr) { }
static inline int sysfs_create_bin_file(struct kobject *kobj, const struct bin_attribute *attr) { return 0; }
static inline void sysfs_remove_bin_file(struct kobject *kobj, const struct bin_attribute *attr) { }
static inline int sysfs_merge_group(struct kobject *kobj, const struct attribute_group *grp) { return 0; }
static inline void sysfs_unmerge_group(struct kobject *kobj, const struct attribute_group *grp) { }
static inline int sysfs_add_file_to_group(struct kobject *kobj, const struct attribute *attr, const char *group) { return 0; }
static inline void sysfs_remove_file_from_group(struct kobject *kobj, const struct attribute *attr, const char *group) { }
static inline void sysfs_notify(struct kobject *kobj, const char *dir, const char *attr) { }

#endif
