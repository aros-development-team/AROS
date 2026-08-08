/*
    Copyright 2025, The AROS Development Team. All rights reserved.

    VC4 Gallium 3D HIDD - DRM compatibility layer for AROS

    This replaces the DRM ioctl interface with direct hardware
    access. Mesa's VC4 Gallium driver calls drmIoctl() — we intercept
    these and implement them using mailbox + V3D registers.
*/

#ifndef _VC4_DRM_AROS_H
#define _VC4_DRM_AROS_H

#include <exec/types.h>

struct vc4galliumstaticdata;

/*
 * Fake file descriptor: Mesa stores screen->fd and passes it to all
 * ioctls, so we stash our static-data pointer in place of a real fd.
 */
struct vc4_aros_fd
{
    ULONG                       magic;      /* VC4_AROS_FD_MAGIC */
    struct vc4galliumstaticdata  *sd;
};

#define VC4_AROS_FD_MAGIC   0x56433444  /* "VC4D" */

/* DRM ioctl encoding */
#define DRM_COMMAND_BASE            0x40

/* VC4-specific DRM ioctl numbers (at DRM_COMMAND_BASE offset) */
#define DRM_VC4_SUBMIT_CL           (DRM_COMMAND_BASE + 0x00)
#define DRM_VC4_WAIT_SEQNO          (DRM_COMMAND_BASE + 0x01)
#define DRM_VC4_WAIT_BO             (DRM_COMMAND_BASE + 0x02)
#define DRM_VC4_CREATE_BO           (DRM_COMMAND_BASE + 0x03)
#define DRM_VC4_MMAP_BO             (DRM_COMMAND_BASE + 0x04)
#define DRM_VC4_CREATE_SHADER_BO    (DRM_COMMAND_BASE + 0x05)
#define DRM_VC4_GET_HANG_STATE      (DRM_COMMAND_BASE + 0x06)
#define DRM_VC4_GET_PARAM           (DRM_COMMAND_BASE + 0x07)
#define DRM_VC4_SET_TILING          (DRM_COMMAND_BASE + 0x08)
#define DRM_VC4_GET_TILING          (DRM_COMMAND_BASE + 0x09)
#define DRM_VC4_LABEL_BO            (DRM_COMMAND_BASE + 0x0a)
#define DRM_VC4_GEM_MADVISE         (DRM_COMMAND_BASE + 0x0b)
#define DRM_VC4_PERFMON_CREATE      (DRM_COMMAND_BASE + 0x0c)
#define DRM_VC4_PERFMON_DESTROY     (DRM_COMMAND_BASE + 0x0d)
#define DRM_VC4_PERFMON_GET_VALUES  (DRM_COMMAND_BASE + 0x0e)

/* Standard DRM ioctls (below DRM_COMMAND_BASE) */
#define DRM_GEM_CLOSE               0x09
#define DRM_GEM_OPEN                0x0b

/*
 * DRM ioctl structures — mirroring the DRM UAPI exactly so Mesa's
 * VC4 driver can use them unchanged.
 */

struct drm_vc4_submit_rcl_surface {
    ULONG hindex;
    ULONG offset;
    UWORD bits;
    UWORD flags;
};

struct drm_vc4_submit_cl {
    UQUAD bin_cl;
    UQUAD shader_rec;
    UQUAD uniforms;
    UQUAD bo_handles;

    ULONG bin_cl_size;
    ULONG shader_rec_size;
    ULONG shader_rec_count;
    ULONG uniforms_size;
    ULONG bo_handle_count;

    UWORD width;
    UWORD height;
    UBYTE min_x_tile;
    UBYTE min_y_tile;
    UBYTE max_x_tile;
    UBYTE max_y_tile;

    struct drm_vc4_submit_rcl_surface color_read;
    struct drm_vc4_submit_rcl_surface color_write;
    struct drm_vc4_submit_rcl_surface zs_read;
    struct drm_vc4_submit_rcl_surface zs_write;
    struct drm_vc4_submit_rcl_surface msaa_color_write;
    struct drm_vc4_submit_rcl_surface msaa_zs_write;

    ULONG clear_color[2];
    ULONG clear_z;
    UBYTE clear_s;

    /*
     * UAPI layout:
     *   __u8  clear_s;
     *   __u32 pad:24;
     *   __u32 flags;
     * clear_s + pad:24 share one 4-byte storage unit (clear_s in the low
     * byte, 24 bits of padding above it). flags follows in the next
     * 4-byte unit. Total from clear_s to end of flags = 8 bytes.
     */
    UBYTE _pad_cs[3];
    ULONG flags;

    UQUAD seqno;

    ULONG perfmonid;
    ULONG in_sync;
    ULONG out_sync;
    ULONG pad2;
};

struct drm_vc4_wait_seqno {
    UQUAD seqno;
    UQUAD timeout_ns;
};

struct drm_vc4_wait_bo {
    ULONG handle;
    ULONG pad;
    UQUAD timeout_ns;
};

struct drm_vc4_create_bo {
    ULONG size;
    ULONG flags;
    ULONG handle;
    ULONG pad;
};

struct drm_vc4_mmap_bo {
    ULONG handle;
    ULONG flags;
    UQUAD offset;
};

struct drm_vc4_create_shader_bo {
    ULONG size;
    ULONG flags;
    UQUAD data;
    ULONG handle;
    ULONG pad;
};

struct drm_vc4_get_param {
    ULONG param;
    ULONG pad;
    UQUAD value;
};

struct drm_vc4_set_tiling {
    ULONG handle;
    ULONG flags;
    UQUAD modifier;
};

struct drm_vc4_get_tiling {
    ULONG handle;
    ULONG flags;
    UQUAD modifier;
};

struct drm_vc4_label_bo {
    ULONG handle;
    ULONG len;
    UQUAD name;
};

struct drm_vc4_gem_madvise {
    ULONG handle;
    ULONG madv;
    ULONG retained;
    ULONG pad;
};

struct drm_gem_close {
    ULONG handle;
    ULONG pad;
};

/* Layout matches struct drm_gem_open (u32 name/handle, u64 size).
 * Used by Mesa's vc4_bo_open_name() to wrap the scanout pages announced
 * by the bridge's get_scanout entry: `name` is the page's physical
 * address. */
struct drm_gem_open {
    ULONG name;
    ULONG handle;
    UQUAD size;
};

/* Parameter IDs for GET_PARAM */
#define DRM_VC4_PARAM_V3D_IDENT0                0
#define DRM_VC4_PARAM_V3D_IDENT1                1
#define DRM_VC4_PARAM_V3D_IDENT2                2
#define DRM_VC4_PARAM_SUPPORTS_BRANCHES         3
#define DRM_VC4_PARAM_SUPPORTS_ETC1             4
#define DRM_VC4_PARAM_SUPPORTS_THREADED_FS      5
#define DRM_VC4_PARAM_SUPPORTS_FIXED_RCL_ORDER  6
#define DRM_VC4_PARAM_SUPPORTS_MADVISE          7
#define DRM_VC4_PARAM_SUPPORTS_PERFMON          8

/* Madvise values */
#define VC4_MADV_WILLNEED       0
#define VC4_MADV_DONTNEED       1

/* Submit flags */
#define VC4_SUBMIT_CL_USE_CLEAR_COLOR           (1 << 0)
#define VC4_SUBMIT_CL_FIXED_RCL_ORDER           (1 << 1)
#define VC4_SUBMIT_CL_RCL_ORDER_INCREASING_X    (1 << 2)
#define VC4_SUBMIT_CL_RCL_ORDER_INCREASING_Y    (1 << 3)

/* Replaces drmIoctl(). Called by the bridge wrappers (vc4_aros_bridge.c)
 * which synthesise a stack-local fake fd whose sd matches the bridge ctx. */
int vc4_aros_ioctl(int fd, unsigned long request, void *arg);

/* mmap replacement — returns CPU pointer for a BO. */
void *vc4_aros_mmap(int fd, ULONG handle);

/*
 * Drop one refcount on a BO handle. If this is the last reference,
 * free the firmware allocation and reset the table slot — mirrors
 * the GEM_CLOSE drop-to-zero path so a pin held by display_blit /
 * async DMA can run the same cleanup once released. Caller must
 * hold sd->bo_lock.
 */
void vc4_aros_bo_unref_locked(struct vc4galliumstaticdata *sd, ULONG handle);

/*
 * Release every outstanding BO and pool-set BO back to the firmware.
 * Called from Expunge.
 */
void vc4_aros_release_all_bos(struct vc4galliumstaticdata *sd);

#endif /* _VC4_DRM_AROS_H */
