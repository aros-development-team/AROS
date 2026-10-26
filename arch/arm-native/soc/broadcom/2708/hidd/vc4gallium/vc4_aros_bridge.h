/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    VC4 Gallium 3D - Bridge ABI between mesa3dgl.library and vc4gallium.hidd.

    mesa3dgl statically links the Mesa VC4 driver; vc4gallium.hidd owns the
    hardware (V3D registers, mailbox, BO table, DMA channel). This header is
    the only ABI surface between them. It carries no Mesa types — only AROS
    primitives and DRM UAPI handle ids — so it stays stable across Mesa
    upgrades.

    Lifecycle: vc4gallium.hidd::CreatePipeScreen builds and populates a
    vc4_aros_bridge, passes it to mesa3dgl's vc4_screen_create(), which
    stores it in pipe_screen->priv. Mesa-side drmIoctl(fd, …) calls become
    bridge->ioctl(bridge->ctx, …).
*/

#ifndef _VC4_AROS_BRIDGE_H
#define _VC4_AROS_BRIDGE_H

#include <exec/types.h>
#include <oop/oop.h>

/* ABI version. Bump on any field add/remove/reorder, and on any change to
 * a function pointer's signature: that breaks callers just the same.
 * Screen-create refuses a bridge of another version. */
#define VC4_AROS_BRIDGE_VERSION   7

/* Which gallium driver this bridge fronts. mesa3dgl dispatches
 * screen-create on driver_id, so one mesa3dgl can carry several drivers
 * (vc4 for Pi 0-3, v3d for Pi 4) and pick by the instantiated hidd. */
#define AROS_GALLIUM_DRIVER_VC4   0
//#define AROS_GALLIUM_DRIVER_V3D   1

struct vc4_aros_bridge
{
    ULONG   version;        /* Must equal VC4_AROS_BRIDGE_VERSION */
    ULONG   driver_id;      /* AROS_GALLIUM_DRIVER_* — picks *_screen_create */

    /* Opaque per-hidd context. mesa3dgl never dereferences it, just
     * passes it back; the hidd resolves it to its staticdata. */
    void   *ctx;

    /* DRM ioctl entry. request is a DRM_VC4_* command (DRM_COMMAND_BASE +
     * cmd) or plain DRM_GEM_CLOSE etc; arg is the matching drm_vc4_*
     * struct. fd is the screen's, which names its session. Returns 0 /
     * negative errno. */
    int    (*ioctl)(void *ctx, int fd, unsigned long request, void *arg);

    /* Map a BO (handle from DRM_VC4_CREATE_BO) and return a CPU pointer,
     * NULL on failure. Mapping is identity (CPU == physical) on AROS, so
     * this is just a handle→pointer lookup. */
    void  *(*mmap)(void *ctx, ULONG handle);

    /* Reverse of mmap. Safe to call with NULL/zero. */
    int    (*munmap)(void *ctx, void *addr, unsigned long length);

    /* Close the fake DRM fd (per-fd state only). Bridge ctx stays alive
     * until the hidd expunges. */
    int    (*close)(void *ctx);

    /* DMA a rendered BO into an AROS bitmap. src_bo_handle is a CREATE_BO
     * handle (hidd looks up the vaddr); dest_bitmap is a Hidd_BitMap whose
     * framebuffer address the hidd resolves via aHidd_ChunkyBM_Buffer.
     * No pipe_resource crosses the boundary — mesa3dgl resolves it to
     * {handle, stride, offset, cpp} first. */
    void   (*display_blit)(void *ctx,
                           ULONG src_bo_handle,
                           ULONG src_stride,
                           ULONG src_offset,
                           ULONG cpp,
                           OOP_Object *dest_bitmap,
                           LONG dst_x, LONG dst_y,
                           ULONG width, ULONG height);

    /* Wait for outstanding GPU work; mesa3dgl calls this before reading a
     * BO's CPU mapping. Optional — no-op if the bridge syncs per submit. */
    void   (*wait_idle)(void *ctx);

    /* Release every outstanding BO back to the firmware. Called by
     * mesa3dgl from screen destroy. */
    void   (*release_all_bos)(void *ctx);
};

/* Populate `bridge` with function pointers forwarding to `sd`'s state.
 * Implemented in the hidd; mesa3dgl only ever sees the bridge struct. */
struct vc4galliumstaticdata;
void vc4_aros_bridge_init(struct vc4_aros_bridge *bridge,
                          struct vc4galliumstaticdata *sd);

#endif /* _VC4_AROS_BRIDGE_H */
