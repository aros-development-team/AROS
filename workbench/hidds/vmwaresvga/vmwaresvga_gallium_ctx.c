/*
    Copyright 2019, The AROS Development Team. All rights reserved.
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


static struct HIDDGalliumVMWareSVGACtx *VMWareSVGA_WSCtx_HiddDataFromWinSys(struct svga_winsys_context *swc)
{
    return (struct HIDDGalliumVMWareSVGACtx *)swc;
}

static void
VMWareSVGA_WSCtx_Destroy(struct svga_winsys_context *swc)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));
}

static void *
VMWareSVGA_WSCtx_Reserve(struct svga_winsys_context *swc,
                uint32_t nr_bytes, uint32_t nr_relocs )
{
    void *retval;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p, %d, %d)\n", __func__, swc, nr_bytes, nr_relocs));

    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(hiddwsctx->wscsws);

    if(nr_bytes > hiddwsctx->command->size)
        return NULL;

    if(hiddwsctx->command->used + nr_bytes > hiddwsctx->command->size ||
      hiddwsctx->surface.used + nr_relocs > hiddwsctx->surface.size ||
      hiddwsctx->shader.used + nr_relocs > hiddwsctx->shader.size ||
      hiddwsctx->region.used + nr_relocs > hiddwsctx->region.size) {
      D(bug("[VMWareSVGA:Gallium] %s: not enough free space\n", __func__));
        return NULL;
    }

    hiddwsctx->surface.reserved = nr_relocs;
    hiddwsctx->surface.staged = 0;
    hiddwsctx->shader.reserved = nr_relocs;
    hiddwsctx->shader.staged = 0;
    hiddwsctx->region.reserved = nr_relocs;
    hiddwsctx->region.staged = 0;

    retval = reserveVMWareSVGAFIFO(data->hwdata, nr_bytes);
    D(bug("[VMWareSVGA:Gallium] %s: returning 0x%p\n", __func__, retval));

    return retval;
}

static unsigned
VMWareSVGA_WSCtx_GetCmdBuffSize(struct svga_winsys_context *swc)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    D(bug("[VMWareSVGA:Gallium] %s: returning %d\n", __func__, hiddwsctx->command->used));
    return hiddwsctx->command->used;
}

static void
VMWareSVGA_WSCtx_SurfaceReloc(struct svga_winsys_context *swc,
                           uint32 *where,
                           uint32 *mobid,
                           struct svga_winsys_surface *surface,
                           unsigned flags)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    if (!surface) {
        *where = SVGA3D_INVALID_ID;
        if (mobid)
            *mobid = SVGA3D_INVALID_ID;
        return;
    }
}

static void
VMWareSVGA_WSCtx_RegionReloc(struct svga_winsys_context *swc,
                          struct SVGAGuestPtr *where,
                          struct svga_winsys_buffer *buffer,
                          uint32 offset,
                          unsigned flags)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));
    ++hiddwsctx->region.staged;
}

static void
VMWareSVGA_WSCtx_MObReloc(struct svga_winsys_context *swc,
		       SVGAMobId *id,
		       uint32 *offset_into_mob,
		       struct svga_winsys_buffer *buffer,
		       uint32 offset,
		       unsigned flags)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    if (id) {
        ++hiddwsctx->region.staged;
    }
}

static void
VMWareSVGA_WSCtx_QueryReloc(struct svga_winsys_context *swc,
                         SVGAMobId *id,
                         struct svga_winsys_gb_query *query)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));
#if (0)
   /* Queries are backed by one big MOB */
   VMWareSVGA_WSCtx_MObReloc(swc, id, NULL, query->buf, 0,
                          SVGA_RELOC_READ | SVGA_RELOC_WRITE);
#endif
}

static void
VMWareSVGA_WSCtx_ContextReloc(struct svga_winsys_context *swc,
			   uint32 *cid)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));
}

static void
VMWareSVGA_WSCtx_ShaderReloc(struct svga_winsys_context *swc,
			  uint32 *shid,
			  uint32 *mobid,
			  uint32 *offset,
			  struct svga_winsys_gb_shader *shader,
                          unsigned flags)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));
    ++hiddwsctx->shader.staged;
}

static void
VMWareSVGA_WSCtx_Commit(struct svga_winsys_context *swc)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(hiddwsctx->wscsws);

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    hiddwsctx->surface.used += hiddwsctx->surface.staged;
    hiddwsctx->surface.staged = 0;
    hiddwsctx->surface.reserved = 0;

    hiddwsctx->shader.used += hiddwsctx->shader.staged;
    hiddwsctx->shader.staged = 0;
    hiddwsctx->shader.reserved = 0;

    hiddwsctx->region.used += hiddwsctx->region.staged;
    hiddwsctx->region.staged = 0;
    hiddwsctx->region.reserved = 0;
    
    commitVMWareSVGAFIFO(data->hwdata, hiddwsctx->command->reserved);
}

static enum pipe_error
VMWareSVGA_WSCtx_Flush(struct svga_winsys_context *swc,
              struct pipe_fence_handle **pfence)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(hiddwsctx->wscsws);
    struct pipe_fence_handle *fence = NULL;
    enum pipe_error ret;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));


    if (hiddwsctx->command->used || pfence != NULL)
        flushVMWareSVGAFIFO(data->hwdata, (ULONG *)(IPTR)&fence);

    syncfenceVMWareSVGAFIFO(data->hwdata, (ULONG)(IPTR)fence);

    hiddwsctx->command->used = 0;

    hiddwsctx->surface.used = 0;
    hiddwsctx->surface.reserved = 0;

    hiddwsctx->shader.used = 0;
    hiddwsctx->shader.reserved = 0;

    hiddwsctx->region.used = 0;
    hiddwsctx->region.reserved = 0;

    return PIPE_OK;
}

static struct svga_winsys_gb_shader *
VMWareSVGA_WSCtx_ShaderCreate(struct svga_winsys_context *swc,
                                     uint32 shaderId,
                                     SVGA3dShaderType shaderType,
                                     const uint32 *bytecode,
                                     uint32 bytecodeLen)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    return hiddwsctx->wscsws->shader_create(hiddwsctx->wscsws, shaderType, bytecode,
                                    bytecodeLen);
}

static void
VMWareSVGA_WSCtx_ShaderDestroy(struct svga_winsys_context *swc,
                                      struct svga_winsys_gb_shader *shader)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    hiddwsctx->wscsws->shader_destroy(hiddwsctx->wscsws, shader);
}

static enum pipe_error
VMWareSVGA_WSCtx_RebindResource(struct svga_winsys_context *swc,
                                struct svga_winsys_surface *surface,
                                struct svga_winsys_gb_shader *shader,
                                unsigned flags)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

   if (!VMWareSVGA_WSCtx_Reserve(swc, 0, 1))
      return PIPE_ERROR_OUT_OF_MEMORY;

    if (surface)
        VMWareSVGA_WSCtx_SurfaceReloc(swc, NULL, NULL, surface, flags);
    else if (shader)
        VMWareSVGA_WSCtx_ShaderReloc(swc, NULL, NULL, NULL, shader, flags);

    VMWareSVGA_WSCtx_Commit(swc);

    return PIPE_OK;
}

ULONG VMWareSVGA_DefineContext(struct HIDDGalliumVMWareSVGAData *data, struct svga_winsys_context *wsctx)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    wsctx->cid = ++data->ctxcnt;
    if ((SVGA3D_DefineContext(wsctx)) != PIPE_OK)
        wsctx->cid = -1;

    D(bug("[VMWareSVGA:Gallium] %s: returning %d\n", __func__, wsctx->cid));

    return wsctx->cid;
}

ULONG VMWareSVGA_DefineExtContext(struct HIDDGalliumVMWareSVGAData *data, struct svga_winsys_context *wsctx, BOOL hasvgpu10)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    wsctx->cid = ++data->ctxcnt;
    if ((SVGA3D_DefineContext(wsctx)) != PIPE_OK)
        wsctx->cid = -1;

    D(bug("[VMWareSVGA:Gallium] %s: returning %d\n", __func__, wsctx->cid));
    return wsctx->cid;
}

void VMWareSVGA_WSCtx_WinSysInit(struct HIDDGalliumVMWareSVGAData *data, struct HIDDGalliumVMWareSVGACtx *hiddwsctx)
{
    struct svga_winsys_context          *wsctx;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, hiddwsctx));

    wsctx = &hiddwsctx->wscbase;

    D(bug("[VMWareSVGA:Gallium] %s: svga_winsys_context @ 0x%p\n", __func__, wsctx));

    wsctx->destroy = VMWareSVGA_WSCtx_Destroy;
    wsctx->reserve = VMWareSVGA_WSCtx_Reserve;
    wsctx->get_command_buffer_size = VMWareSVGA_WSCtx_GetCmdBuffSize;
    wsctx->surface_relocation = VMWareSVGA_WSCtx_SurfaceReloc;
    wsctx->region_relocation = VMWareSVGA_WSCtx_RegionReloc;
    wsctx->mob_relocation = VMWareSVGA_WSCtx_MObReloc;
    wsctx->query_relocation = VMWareSVGA_WSCtx_QueryReloc;

    wsctx->context_relocation = VMWareSVGA_WSCtx_ContextReloc;
    wsctx->shader_relocation = VMWareSVGA_WSCtx_ShaderReloc;
    wsctx->commit = VMWareSVGA_WSCtx_Commit;
    wsctx->flush = VMWareSVGA_WSCtx_Flush;

    wsctx->surface_map = VMWareSVGA_WSSurf_SurfaceMap;
    wsctx->surface_unmap = VMWareSVGA_WSSurf_SurfaceUnMap;
    wsctx->surface_invalidate = VMWareSVGA_WSSurf_SurfaceInvalidate;

    wsctx->shader_create = VMWareSVGA_WSCtx_ShaderCreate;
    wsctx->shader_destroy = VMWareSVGA_WSCtx_ShaderDestroy;

    wsctx->resource_rebind = VMWareSVGA_WSCtx_RebindResource;

    if (data->wssbase.have_vgpu10)
        wsctx->cid = VMWareSVGA_DefineExtContext(data, wsctx, data->wssbase.have_vgpu10);
   else
        wsctx->cid = VMWareSVGA_DefineContext(data, wsctx);

    wsctx->imported_fence_fd = -1;

    wsctx->have_gb_objects = data->wssbase.have_gb_objects;

    hiddwsctx->surface.size = VMW_SURFACE_RELOCS;
    hiddwsctx->shader.size = VMW_SHADER_RELOCS;
    hiddwsctx->region.size = VMW_REGION_RELOCS;
}
