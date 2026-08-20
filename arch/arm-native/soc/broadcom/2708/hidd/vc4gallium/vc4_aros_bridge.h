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

/* ABI version. Bump on any field add/remove/reorder. mesa3dgl checks it
 * at screen-create and refuses an incompatible bridge. */
#define VC4_AROS_BRIDGE_VERSION   5

/* Which gallium driver this bridge fronts. mesa3dgl dispatches
 * screen-create on driver_id, so one mesa3dgl can carry several drivers
 * (vc4 for Pi 0-3, v3d for Pi 4) and pick by the instantiated hidd. */
#define AROS_GALLIUM_DRIVER_VC4   0
//#define AROS_GALLIUM_DRIVER_V3D   1

/* Scanout page description for zero-copy fullscreen GL (v2). The vcgfx
 * framebuffer has two flip pages; Mesa renders into the back page and
 * presents via flip_scanout instead of copying. name[] are GEM_OPEN names
 * (page physical addresses), stable across flips; `back` indexes the page
 * not currently shown. */
struct vc4_aros_scanout
{
    ULONG   name[2];
    ULONG   pitch;
    ULONG   width;
    ULONG   height;
    ULONG   back;
};

struct vc4_aros_bridge
{
    ULONG   version;        /* Must equal VC4_AROS_BRIDGE_VERSION */
    ULONG   driver_id;      /* AROS_GALLIUM_DRIVER_* — picks *_screen_create */

    /* Opaque per-hidd context. mesa3dgl never dereferences it, just
     * passes it back; the hidd resolves it to its staticdata. */
    void   *ctx;

    /* DRM ioctl entry. request is a DRM_VC4_* command (DRM_COMMAND_BASE +
     * cmd) or plain DRM_GEM_CLOSE etc; arg is the matching drm_vc4_*
     * struct. Returns 0 / negative errno. */
    int    (*ioctl)(void *ctx, unsigned long request, void *arg);

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

    /* v2: query the framebuffer's flip pages (publishing them as GEM_OPEN
     * names). Returns 0 for a flippable vcgfx framebuffer, -1 otherwise. */
    int    (*get_scanout)(void *ctx, OOP_Object *dest_bitmap,
                          struct vc4_aros_scanout *out);

    /* v2: make the back page visible (waits for GPU + DMA first). Returns
     * 0 on success; get_scanout then reports the new back index. */
    int    (*flip_scanout)(void *ctx, OOP_Object *dest_bitmap);

    /* v3: zero-copy windowed present. Show src_bo_handle as an opaque
     * HVS overlay plane at (x,y) (fb coordinates), source size w*h,
     * shown at dest_w*dest_h (v4; 0 = unscaled, larger = HVS PPF
     * upscale) on the screen's framebuffer bitmap. Waits for GPU idle
     * first, pins the BO while shown, and returns 0 on success (-1 =
     * unavailable: firmware owns the display / scaled desktop /
     * downscale — caller blits instead). Repeated calls retarget/move
     * the plane; the update latches at vblank with flip pacing. */
    int    (*set_overlay)(void *ctx, OOP_Object *dest_bitmap,
                          ULONG src_bo_handle, ULONG src_stride,
                          LONG x, LONG y, ULONG w, ULONG h,
                          ULONG dest_w, ULONG dest_h);

    /* v3: remove the overlay plane and unpin its BO. Safe to call when
     * none is shown; dest_bitmap may be NULL (uses the bitmap of the
     * last set_overlay). */
    void   (*clear_overlay)(void *ctx, OOP_Object *dest_bitmap);

    /* v5: latest SUBMITTED job seqno (same domain as DRM_VC4_WAIT_SEQNO
     * and drm_vc4_submit_cl.seqno). Snapshotted at present time so the
     * pipelined overlay path can wait for exactly the frame it is about
     * to display instead of full GPU idle. */
    ULONG  (*get_seqno)(void *ctx);
};

/* Populate `bridge` with function pointers forwarding to `sd`'s state.
 * Implemented in the hidd; mesa3dgl only ever sees the bridge struct. */
struct vc4galliumstaticdata;
void vc4_aros_bridge_init(struct vc4_aros_bridge *bridge,
                          struct vc4galliumstaticdata *sd);

#endif /* _VC4_AROS_BRIDGE_H */
