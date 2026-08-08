/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    VC4 Gallium 3D - Bridge implementation.

    Populates struct vc4_aros_bridge with thin wrappers forwarding into
    the fd-based vc4_drm_aros.c / vc4_galliumclass.c helpers. The bridge
    takes an opaque ctx; we synthesise a stack-local vc4_aros_fd per call
    to reuse the existing fd-based handlers untouched.
*/

#define DEBUG 0
#include <aros/debug.h>

#include "vc4gallium_intern.h"
#include "vc4_drm_aros.h"
#include "vc4_aros_bridge.h"

/* Stack-local fake fd wrapping the ctx (the hidd's staticdata). Valid
 * only for the helper call — no helper stores it past return. */
#define BRIDGE_FD(ctx)                                              \
    struct vc4_aros_fd _bfd = { VC4_AROS_FD_MAGIC,                  \
                                (struct vc4galliumstaticdata *)(ctx) }; \
    int _fd = (int)(IPTR)&_bfd

static int bridge_ioctl(void *ctx, unsigned long request, void *arg)
{
    BRIDGE_FD(ctx);
    return vc4_aros_ioctl(_fd, request, arg);
}

static void *bridge_mmap(void *ctx, ULONG handle)
{
    BRIDGE_FD(ctx);
    return vc4_aros_mmap(_fd, handle);
}

static int bridge_munmap(void *ctx, void *addr, unsigned long length)
{
    /* Identity mapping (CPU == physical) and BO lifetime is governed by
     * GEM_CLOSE, so munmap is a no-op. */
    (void)ctx;
    (void)addr;
    (void)length;
    return 0;
}

static int bridge_close(void *ctx)
{
    /* Per-fd state is only the stack-local fake fd; nothing to release.
     * Hidd teardown happens via release_all_bos + ExpungeLib. */
    (void)ctx;
    return 0;
}

static void bridge_release_all_bos(void *ctx)
{
    vc4_aros_release_all_bos((struct vc4galliumstaticdata *)ctx);
}

/* Implemented in vc4_galliumclass.c — resolved fields only, no Mesa
 * types cross the API boundary. */
extern void vc4_aros_display_blit(struct vc4galliumstaticdata *sd,
                                  ULONG src_bo_handle,
                                  ULONG src_stride,
                                  ULONG src_offset,
                                  ULONG cpp,
                                  OOP_Object *bm_obj,
                                  LONG dst_x, LONG dst_y,
                                  ULONG width, ULONG height);
extern void vc4_aros_wait_idle(struct vc4galliumstaticdata *sd);
extern int vc4_aros_get_scanout(struct vc4galliumstaticdata *sd,
                                OOP_Object *bm_obj,
                                struct vc4_aros_scanout *out);
extern int vc4_aros_flip_scanout(struct vc4galliumstaticdata *sd,
                                 OOP_Object *bm_obj);
extern int vc4_aros_set_overlay(struct vc4galliumstaticdata *sd,
                                OOP_Object *bm_obj,
                                ULONG src_bo_handle, ULONG src_stride,
                                LONG x, LONG y, ULONG w, ULONG h,
                                ULONG dest_w, ULONG dest_h);
extern void vc4_aros_clear_overlay(struct vc4galliumstaticdata *sd,
                                   OOP_Object *bm_obj);

static void bridge_display_blit(void *ctx,
    ULONG src_bo_handle, ULONG src_stride, ULONG src_offset, ULONG cpp,
    OOP_Object *dest_bitmap,
    LONG dst_x, LONG dst_y,
    ULONG width, ULONG height)
{
    vc4_aros_display_blit((struct vc4galliumstaticdata *)ctx,
                          src_bo_handle, src_stride, src_offset, cpp,
                          dest_bitmap, dst_x, dst_y, width, height);
}

static void bridge_wait_idle(void *ctx)
{
    vc4_aros_wait_idle((struct vc4galliumstaticdata *)ctx);
}

static int bridge_get_scanout(void *ctx, OOP_Object *dest_bitmap,
                              struct vc4_aros_scanout *out)
{
    return vc4_aros_get_scanout((struct vc4galliumstaticdata *)ctx,
                                dest_bitmap, out);
}

static int bridge_flip_scanout(void *ctx, OOP_Object *dest_bitmap)
{
    return vc4_aros_flip_scanout((struct vc4galliumstaticdata *)ctx,
                                 dest_bitmap);
}

static int bridge_set_overlay(void *ctx, OOP_Object *dest_bitmap,
                              ULONG src_bo_handle, ULONG src_stride,
                              LONG x, LONG y, ULONG w, ULONG h,
                              ULONG dest_w, ULONG dest_h)
{
    return vc4_aros_set_overlay((struct vc4galliumstaticdata *)ctx,
                                dest_bitmap, src_bo_handle, src_stride,
                                x, y, w, h, dest_w, dest_h);
}

static void bridge_clear_overlay(void *ctx, OOP_Object *dest_bitmap)
{
    vc4_aros_clear_overlay((struct vc4galliumstaticdata *)ctx, dest_bitmap);
}

static ULONG bridge_get_seqno(void *ctx)
{
    /* Aligned ULONG read — no lock needed for a monotonic snapshot. */
    return ((struct vc4galliumstaticdata *)ctx)->v3d.seqno;
}

/* Populate a bridge struct for the given hidd staticdata. The struct
 * lives in caller storage (typically mesa3dgl's screen->priv slot). */
void vc4_aros_bridge_init(struct vc4_aros_bridge *bridge,
                          struct vc4galliumstaticdata *sd)
{
    bridge->version         = VC4_AROS_BRIDGE_VERSION;
    bridge->driver_id       = AROS_GALLIUM_DRIVER_VC4;
    bridge->ctx             = sd;
    bridge->ioctl           = bridge_ioctl;
    bridge->mmap            = bridge_mmap;
    bridge->munmap          = bridge_munmap;
    bridge->close           = bridge_close;
    bridge->display_blit    = bridge_display_blit;
    bridge->wait_idle       = bridge_wait_idle;
    bridge->release_all_bos = bridge_release_all_bos;
    bridge->get_scanout     = bridge_get_scanout;
    bridge->flip_scanout    = bridge_flip_scanout;
    bridge->set_overlay     = bridge_set_overlay;
    bridge->clear_overlay   = bridge_clear_overlay;
    bridge->get_seqno       = bridge_get_seqno;
}
