/*
    Copyright 2019-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>

#include <hidd/gallium.h>
#include <gallium/gallium.h>

#include "vmwaresvga_intern.h"

void *
VMWareSVGA_WSSurf_SurfaceMap(struct svga_winsys_context *swc,
                            struct svga_winsys_surface *srf,
                            unsigned flags, boolean *retry)
{
    struct HIDDGalliumVMWareSVGASurf *surface = VMWareSVGA_WSSurf_HiddSurfFromWinSysSurf(srf);
    void *data = (void *)((struct VMWareSVGAPBBuf *)(surface->surfbuf))->map;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p, 0x%p)\n", __func__, swc, surface));
    D(bug("[VMWareSVGA:Gallium] %s: returning 0x%p\n", __func__, data));

    return data;
}


void
VMWareSVGA_WSSurf_SurfaceUnMap(struct svga_winsys_context *swc,
                              struct svga_winsys_surface *srf,
                              boolean *rebind)
{
   struct HIDDGalliumVMWareSVGASurf *surface = VMWareSVGA_WSSurf_HiddSurfFromWinSysSurf(srf);

    D(bug("[VMWareSVGA:Gallium] %s(0x%p, 0x%p)\n", __func__, swc, surface));
}

void
VMWareSVGA_WSSurf_SurfaceReference(struct HIDDGalliumVMWareSVGASurf **pdst,
                                  struct HIDDGalliumVMWareSVGASurf *src)
{
    struct pipe_reference *src_ref;
    struct pipe_reference *dst_ref;
    struct HIDDGalliumVMWareSVGASurf *dst;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p, 0x%p)\n", __func__, pdst, src));

    if(pdst == NULL || *pdst == src)
        return;

    dst = *pdst;

    src_ref = src ? &src->refcnt : NULL;
    dst_ref = dst ? &dst->refcnt : NULL;

    if (pipe_reference(dst_ref, src_ref)) {
#if (0)
        if (dst->buf)
            VMWareSVGA_WSSurf_buffer_destroy(&dst->screen->base, dst->buf);
#endif
        FREE(dst);
    }

    *pdst = src;
}
