/*
 * DRM core stub for AROS. Only what Mesa's v3d driver actually reaches
 * for: the GEM handle calls and the ioctl numbering they travel on.
 *
 * The numbering convention here is "the nr, nothing else": core calls
 * occupy 0x00-0x3f and a driver's own start at DRM_COMMAND_BASE. Real DRM
 * packs direction and struct size into the value as well, which nothing on
 * this side needs, and leaving them out keeps the numbers readable in the
 * shim that dispatches them.
 */
#ifndef DRM_H
#define DRM_H

#include <stdint.h>

#ifndef __u32
typedef unsigned int __u32;
typedef unsigned long long __u64;
typedef int __s32;
typedef long long __s64;
#endif

#ifndef DRM_COMMAND_BASE
#define DRM_COMMAND_BASE 0x40
#endif

#ifndef DRM_IOWR
#define DRM_IOWR(nr, type) (nr)
#endif
#ifndef DRM_IOW
#define DRM_IOW(nr, type) (nr)
#endif
#ifndef DRM_IOR
#define DRM_IOR(nr, type) (nr)
#endif

struct drm_gem_close {
    __u32 handle;
    __u32 pad;
};

struct drm_gem_flink {
    __u32 handle;
    __u32 name;
};

struct drm_gem_open {
    __u32 name;
    __u32 handle;
    __u64 size;
};

/* Core numbers, as in DRM itself - note FLINK precedes OPEN. */
#define DRM_IOCTL_GEM_CLOSE     DRM_IOW(0x09, struct drm_gem_close)
#define DRM_IOCTL_GEM_FLINK     DRM_IOWR(0x0a, struct drm_gem_flink)
#define DRM_IOCTL_GEM_OPEN      DRM_IOWR(0x0b, struct drm_gem_open)

#endif
