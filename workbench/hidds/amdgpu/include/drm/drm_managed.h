/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_MANAGED_H_
#define _DRM_MANAGED_H_

#include <linux/gfp.h>
#include <linux/overflow.h>
#include <linux/types.h>
#include <linux/mutex.h>

/*
 * Device-managed resources: an action list on the drm_device that is run
 * in reverse order when the last reference to the device goes.
 */
struct drm_device;
struct mutex;
struct workqueue_struct;

typedef void (*drmres_release_t)(struct drm_device *dev, void *res);

int __drmm_add_action(struct drm_device *dev, drmres_release_t action, void *data, const char *name);
int __drmm_add_action_or_reset(struct drm_device *dev, drmres_release_t action, void *data, const char *name);
#define drmm_add_action(dev, action, data)          __drmm_add_action(dev, action, data, #action)
#define drmm_add_action_or_reset(dev, action, data) __drmm_add_action_or_reset(dev, action, data, #action)
void drmm_release_action(struct drm_device *dev, drmres_release_t action, void *data);
void drm_managed_release(struct drm_device *dev);
void drmm_add_final_kfree(struct drm_device *dev, void *container);

void *drmm_kmalloc(struct drm_device *dev, size_t size, gfp_t gfp);
char *drmm_kstrdup(struct drm_device *dev, const char *s, gfp_t gfp);
void  drmm_kfree(struct drm_device *dev, void *data);
static inline void *drmm_kzalloc(struct drm_device *dev, size_t size, gfp_t gfp)
{
    return drmm_kmalloc(dev, size, gfp | __GFP_ZERO);
}
static inline void *drmm_kmalloc_array(struct drm_device *dev, size_t n, size_t size, gfp_t flags)
{
    size_t bytes;
    if (unlikely(check_mul_overflow(n, size, &bytes)))
        return NULL;
    return drmm_kmalloc(dev, bytes, flags);
}
static inline void *drmm_kcalloc(struct drm_device *dev, size_t n, size_t size, gfp_t flags)
{
    return drmm_kmalloc_array(dev, n, size, flags | __GFP_ZERO);
}

void __drmm_mutex_release(struct drm_device *dev, void *res);
#define drmm_mutex_init(dev, lock) ({                                   \
    mutex_init(lock);                                                   \
    drmm_add_action_or_reset(dev, __drmm_mutex_release, lock); })
void __drmm_workqueue_release(struct drm_device *device, void *wq);
#define drmm_alloc_ordered_workqueue(dev, fmt, flags, args...) ({       \
    struct workqueue_struct *wq = alloc_ordered_workqueue(fmt, flags, ##args); \
    wq ? ((drmm_add_action_or_reset(dev, __drmm_workqueue_release, wq)) ? NULL : wq) : NULL; })

#endif /* _DRM_MANAGED_H_ */
