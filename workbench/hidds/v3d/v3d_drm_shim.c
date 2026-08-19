/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    V3D DRM ioctl shim for AROS
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include <proto/exec.h>

#include "v3d_intern.h"

/* DRM ioctl numbers used by Mesa V3D driver */
#define DRM_IOCTL_V3D_SUBMIT_CL    0x00
#define DRM_IOCTL_V3D_WAIT_BO      0x01
#define DRM_IOCTL_V3D_CREATE_BO    0x02
#define DRM_IOCTL_V3D_MMAP_BO      0x03
#define DRM_IOCTL_V3D_GET_PARAM    0x04
#define DRM_IOCTL_V3D_GET_BO_OFFSET 0x05
#define DRM_IOCTL_V3D_SUBMIT_TFU   0x06
#define DRM_IOCTL_GEM_CLOSE        0x09
#define DRM_IOCTL_GEM_OPEN         0x0A
#define DRM_IOCTL_GEM_FLINK        0x0B

/* Structures matching Linux DRM V3D ioctls */
struct drm_v3d_submit_cl {
    ULONG bcl_start;
    ULONG bcl_end;
    ULONG rcl_start;
    ULONG rcl_end;
    ULONG qma;          /* Query memory address */
    ULONG qms;          /* Query memory size */
    ULONG qts;          /* Query timestamp */
    ULONG flags;
    /* BO handles array follows */
};

struct drm_v3d_create_bo {
    ULONG size;
    ULONG flags;
    ULONG handle;       /* Output */
    ULONG offset;       /* Output: GPU address */
};

struct drm_v3d_mmap_bo {
    ULONG handle;
    ULONG flags;
    UQUAD offset;     /* Output: mmap offset (we return vaddr) */
};

struct drm_v3d_wait_bo {
    ULONG handle;
    ULONG pad;
    UQUAD timeout_ns;
};

struct drm_v3d_get_param {
    ULONG param;
    ULONG pad;
    UQUAD value;      /* Output */
};

struct drm_v3d_get_bo_offset {
    ULONG handle;
    ULONG offset;       /* Output */
};

struct drm_gem_close {
    ULONG handle;
    ULONG pad;
};

/* V3D_GET_PARAM parameter IDs */
#define DRM_V3D_PARAM_V3D_UIFCFG    0
#define DRM_V3D_PARAM_V3D_HUB_IDENT1 1
#define DRM_V3D_PARAM_V3D_HUB_IDENT2 2
#define DRM_V3D_PARAM_V3D_HUB_IDENT3 3
#define DRM_V3D_PARAM_V3D_CORE0_IDENT0 4
#define DRM_V3D_PARAM_V3D_CORE0_IDENT1 5
#define DRM_V3D_PARAM_V3D_CORE0_IDENT2 6
#define DRM_V3D_PARAM_SUPPORTS_TFU  7
#define DRM_V3D_PARAM_SUPPORTS_CSD  8

/*
 * Simple handle table — maps integer handles to V3DBO pointers.
 * Max 256 concurrent BOs should be plenty for AROS usage.
 */
#define MAX_BO_HANDLES 256
static struct V3DBO *bo_table[MAX_BO_HANDLES];
static ULONG next_handle = 1;

static ULONG alloc_handle(struct V3DBO *bo)
{
    ULONG h = next_handle++;
    if (h >= MAX_BO_HANDLES)
        return 0;
    bo_table[h] = bo;
    return h;
}

static struct V3DBO *lookup_handle(ULONG handle)
{
    if (handle >= MAX_BO_HANDLES)
        return NULL;
    return bo_table[handle];
}

static void free_handle(ULONG handle)
{
    if (handle < MAX_BO_HANDLES)
        bo_table[handle] = NULL;
}

/*
 * v3d_ioctl_aros — replacement for drmIoctl/v3d_ioctl.
 *
 * This function is called by the Mesa V3D driver instead of the
 * Linux DRM ioctl interface.
 */
int v3d_ioctl_aros(struct V3DData *sd, unsigned long request, void *arg)
{
    switch (request) {
    case DRM_IOCTL_V3D_CREATE_BO:
    {
        struct drm_v3d_create_bo *create = arg;
        struct V3DBO *bo = v3d_aros_bo_alloc(sd, create->size);
        if (!bo)
            return -1;
        create->handle = alloc_handle(bo);
        create->offset = bo->paddr;
        return 0;
    }

    case DRM_IOCTL_GEM_CLOSE:
    {
        struct drm_gem_close *close = arg;
        struct V3DBO *bo = lookup_handle(close->handle);
        if (bo) {
            free_handle(close->handle);
            v3d_aros_bo_free(sd, bo);
        }
        return 0;
    }

    case DRM_IOCTL_V3D_MMAP_BO:
    {
        struct drm_v3d_mmap_bo *map = arg;
        struct V3DBO *bo = lookup_handle(map->handle);
        if (!bo)
            return -1;
        /* On AROS, vaddr IS the usable pointer (no mmap needed) */
        map->offset = (IPTR)bo->vaddr;
        return 0;
    }

    case DRM_IOCTL_V3D_GET_BO_OFFSET:
    {
        struct drm_v3d_get_bo_offset *get = arg;
        struct V3DBO *bo = lookup_handle(get->handle);
        if (!bo)
            return -1;
        get->offset = bo->paddr;
        return 0;
    }

    case DRM_IOCTL_V3D_WAIT_BO:
    {
        /* All our submissions are synchronous for now */
        v3d_wait_idle(sd);
        return 0;
    }

    case DRM_IOCTL_V3D_SUBMIT_CL:
    {
        struct drm_v3d_submit_cl *submit = arg;

        /* Submit binning CL */
        if (submit->bcl_start != submit->bcl_end) {
            v3d_submit_cl(sd, submit->bcl_start, submit->bcl_end, FALSE);
            v3d_wait_idle(sd);
        }

        /* Submit rendering CL */
        if (submit->rcl_start != submit->rcl_end) {
            v3d_submit_cl(sd, submit->rcl_start, submit->rcl_end, TRUE);
            v3d_wait_idle(sd);
        }

        return 0;
    }

    case DRM_IOCTL_V3D_GET_PARAM:
    {
        struct drm_v3d_get_param *p = arg;
        ULONG val;

        switch (p->param) {
        case DRM_V3D_PARAM_V3D_HUB_IDENT1:
            val = AROS_LE2LONG(*(volatile ULONG *)(sd->hub_base + V3D_HUB_IDENT1));
            break;
        case DRM_V3D_PARAM_V3D_HUB_IDENT2:
            val = AROS_LE2LONG(*(volatile ULONG *)(sd->hub_base + V3D_HUB_IDENT2));
            break;
        case DRM_V3D_PARAM_V3D_HUB_IDENT3:
            val = AROS_LE2LONG(*(volatile ULONG *)(sd->hub_base + V3D_HUB_IDENT3));
            break;
        case DRM_V3D_PARAM_V3D_CORE0_IDENT0:
            val = AROS_LE2LONG(*(volatile ULONG *)(sd->core0_base + V3D_CTL_IDENT0));
            break;
        case DRM_V3D_PARAM_V3D_CORE0_IDENT1:
            val = AROS_LE2LONG(*(volatile ULONG *)(sd->core0_base + V3D_CTL_IDENT1));
            break;
        case DRM_V3D_PARAM_V3D_CORE0_IDENT2:
            val = AROS_LE2LONG(*(volatile ULONG *)(sd->core0_base + V3D_CTL_IDENT2));
            break;
        case DRM_V3D_PARAM_SUPPORTS_TFU:
            val = 1; /* BCM2711 V3D 4.2 has TFU */
            break;
        case DRM_V3D_PARAM_SUPPORTS_CSD:
            val = 1; /* BCM2711 V3D 4.2 has CSD */
            break;
        default:
            val = 0;
            break;
        }
        p->value = val;
        return 0;
    }

    case DRM_IOCTL_V3D_SUBMIT_TFU:
        /* TFU (Texture Formatting Unit) — not yet implemented */
        D(bug("[V3D] TFU submit not yet implemented\n"));
        return -1;

    default:
        D(bug("[V3D] Unknown ioctl 0x%lx\n", request));
        return -1;
    }
}
