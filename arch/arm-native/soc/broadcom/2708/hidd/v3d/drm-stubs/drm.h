/*
 * DRM core stub for AROS: the GEM handle calls and the ioctl numbering they
 * travel on. These are the nr only - real DRM also packs direction and struct
 * size into the value, which nothing on this side needs.
 */
#ifndef DRM_H
#define DRM_H

#include <stdint.h>

#ifndef __u32
typedef unsigned char __u8;
typedef unsigned short __u16;
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

/* Real DRM values. Nothing here exports dma-bufs or waits on a syncobj, but
 * the driver names them. */
#define DRM_CLOEXEC                         0x80000
#define DRM_RDWR                            0x00002
#define DRM_SYNCOBJ_WAIT_FLAGS_WAIT_ALL     (1 << 0)

#endif
