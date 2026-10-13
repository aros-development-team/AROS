/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_KOBJECT_H_
#define _LINUX_KOBJECT_H_
#include <linux/idr.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/kref.h>
#include <linux/wait.h>
#include <linux/sysfs.h>

struct kobject;
struct kset;

struct kobj_type {
    void (*release)(struct kobject *kobj);
    const struct sysfs_ops *sysfs_ops;
    const struct attribute_group **default_groups;
};

struct kobject {
    const char *name;
    struct list_head entry;
    struct kobject *parent;
    struct kset *kset;
    const struct kobj_type *ktype;
    struct kref kref;
    void *sd;
};

struct kset {
    struct list_head list;
    spinlock_t list_lock;
    struct kobject kobj;
};

struct kobj_attribute {
    struct attribute attr;
    ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
    ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
};

extern const struct sysfs_ops kobj_sysfs_ops;

int kobject_init_and_add(struct kobject *kobj, const struct kobj_type *ktype, struct kobject *parent, const char *fmt, ...);
void kobject_init(struct kobject *kobj, const struct kobj_type *ktype);
int kobject_add(struct kobject *kobj, struct kobject *parent, const char *fmt, ...);
void kobject_del(struct kobject *kobj);
struct kobject *kobject_get(struct kobject *kobj);
void kobject_put(struct kobject *kobj);
struct kobject *kobject_create_and_add(const char *name, struct kobject *parent);
struct kset *kset_create_and_add(const char *name, const void *uevent_ops, struct kobject *parent_kobj);
void kset_unregister(struct kset *kset);

int kobject_set_name(struct kobject *kobj, const char *fmt, ...);
int kset_register(struct kset *kset);
#define to_kset(k)                  container_of(k, struct kset, kobj)
#define kobject_name(k)             ((k)->name)
#define KOBJ_CHANGE                 0
#define KOBJ_ADD                    1
#define KOBJ_REMOVE                 2
#endif
