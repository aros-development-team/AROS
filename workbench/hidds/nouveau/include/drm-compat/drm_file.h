/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DRM_COMPAT_DRM_FILE_H
#define DRM_COMPAT_DRM_FILE_H

struct drm_file
{
    struct idr object_idr;
    spinlock_t table_lock;
    void *driver_priv;
    struct list_head fbs;
    struct mutex fbs_lock;
};

#endif
