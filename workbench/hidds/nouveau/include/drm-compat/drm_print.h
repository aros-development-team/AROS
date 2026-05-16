/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DRM_COMPAT_DRM_PRINT_H
#define DRM_COMPAT_DRM_PRINT_H

#define DRM_ERROR(fmt, ...) bug("[" DRM_NAME "(ERROR):%s] " fmt, __func__ , ##__VA_ARGS__)

#endif
