/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DRM_COMPAT_DRM_PRINT_H
#define DRM_COMPAT_DRM_PRINT_H

#define DRM_ERROR(fmt, ...) bug("[" DRM_NAME "(ERROR):%s] " fmt, __func__ , ##__VA_ARGS__)
#define DRM_DEBUG(fmt, ...) D(bug("[" DRM_NAME "(DEBUG):%s] " fmt, __func__ , ##__VA_ARGS__))
#define DRM_DEBUG_KMS(fmt, ...) D(bug("[" DRM_NAME "(DEBUG):%s] " fmt, __func__ , ##__VA_ARGS__))
#define DRM_INFO(fmt, ...)  bug("[" DRM_NAME "(INFO)] " fmt, ##__VA_ARGS__)
#define DRM_NOTE(fmt, ...)  bug("[" DRM_NAME "(NOTE)] " fmt, ##__VA_ARGS__)

#endif
