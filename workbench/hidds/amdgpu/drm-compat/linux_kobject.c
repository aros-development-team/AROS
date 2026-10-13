/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    kobjects carry the reference count and release callback the driver
    relies on to free its objects; there is no sysfs behind them.
*/

#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/kernel.h>

static ssize_t kobj_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *kattr = container_of(attr, struct kobj_attribute, attr);

    return kattr->show ? kattr->show(kobj, kattr, buf) : -EIO;
}

static ssize_t kobj_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    struct kobj_attribute *kattr = container_of(attr, struct kobj_attribute, attr);

    return kattr->store ? kattr->store(kobj, kattr, buf, count) : -EIO;
}

const struct sysfs_ops kobj_sysfs_ops = {
    .show = kobj_attr_show,
    .store = kobj_attr_store,
};

void kobject_init(struct kobject *kobj, const struct kobj_type *ktype)
{
    kref_init(&kobj->kref);
    INIT_LIST_HEAD(&kobj->entry);
    kobj->ktype = ktype;
    kobj->parent = NULL;
    kobj->name = NULL;
}

static int kobject_vset_name(struct kobject *kobj, const char *fmt, va_list ap)
{
    char *name = kvasprintf(GFP_KERNEL, fmt, ap);

    if (!name)
        return -ENOMEM;
    kfree(kobj->name);
    kobj->name = name;
    return 0;
}

int kobject_add(struct kobject *kobj, struct kobject *parent, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = kobject_vset_name(kobj, fmt, ap);
    va_end(ap);
    kobj->parent = parent ? kobject_get(parent) : NULL;
    return ret;
}

int kobject_init_and_add(struct kobject *kobj, const struct kobj_type *ktype, struct kobject *parent, const char *fmt, ...)
{
    va_list ap;
    int ret;

    kobject_init(kobj, ktype);
    va_start(ap, fmt);
    ret = kobject_vset_name(kobj, fmt, ap);
    va_end(ap);
    kobj->parent = parent ? kobject_get(parent) : NULL;
    return ret;
}

void kobject_del(struct kobject *kobj)
{
    struct kobject *parent;

    if (!kobj)
        return;
    parent = kobj->parent;
    kobj->parent = NULL;
    kobject_put(parent);
}

struct kobject *kobject_get(struct kobject *kobj)
{
    if (kobj)
        kref_get(&kobj->kref);
    return kobj;
}

static void kobject_release(struct kref *kref)
{
    struct kobject *kobj = container_of(kref, struct kobject, kref);
    const char *name = kobj->name;

    kobject_del(kobj);
    if (kobj->ktype && kobj->ktype->release)
        kobj->ktype->release(kobj);
    kfree(name);
}

void kobject_put(struct kobject *kobj)
{
    if (kobj)
        kref_put(&kobj->kref, kobject_release);
}

static void dynamic_kobj_release(struct kobject *kobj)
{
    kfree(kobj);
}

static const struct kobj_type dynamic_kobj_ktype = {
    .release = dynamic_kobj_release,
    .sysfs_ops = &kobj_sysfs_ops,
};

struct kobject *kobject_create_and_add(const char *name, struct kobject *parent)
{
    struct kobject *kobj = kzalloc(sizeof(*kobj), GFP_KERNEL);

    if (!kobj)
        return NULL;
    if (kobject_init_and_add(kobj, &dynamic_kobj_ktype, parent, "%s", name))
    {
        kobject_put(kobj);
        return NULL;
    }
    return kobj;
}

static void kset_release(struct kobject *kobj)
{
    kfree(container_of(kobj, struct kset, kobj));
}

static const struct kobj_type kset_ktype = {
    .release = kset_release,
    .sysfs_ops = &kobj_sysfs_ops,
};

struct kset *kset_create_and_add(const char *name, const void *uevent_ops, struct kobject *parent_kobj)
{
    struct kset *kset = kzalloc(sizeof(*kset), GFP_KERNEL);

    if (!kset)
        return NULL;
    INIT_LIST_HEAD(&kset->list);
    if (kobject_init_and_add(&kset->kobj, &kset_ktype, parent_kobj, "%s", name))
    {
        kobject_put(&kset->kobj);
        return NULL;
    }
    return kset;
}

void kset_unregister(struct kset *kset)
{
    if (kset)
        kobject_put(&kset->kobj);
}

int kobject_set_name(struct kobject *kobj, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = kobject_vset_name(kobj, fmt, ap);
    va_end(ap);
    return ret;
}

int kset_register(struct kset *kset)
{
    INIT_LIST_HEAD(&kset->list);
    spin_lock_init(&kset->list_lock);
    if (!kset->kobj.ktype)
        kobject_init(&kset->kobj, &kset_ktype);
    return 0;
}
