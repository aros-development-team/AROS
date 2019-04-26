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
    D(bug("[VMWareSVGA:Gallium] %s(0x%p, %d, %d)\n", __func__, swc, nr_bytes, nr_relocs));

    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);

    if(hiddwsctx->command.used + nr_bytes > hiddwsctx->command.size ||
      hiddwsctx->surface.used + nr_relocs > hiddwsctx->surface.size ||
      hiddwsctx->shader.used + nr_relocs > hiddwsctx->shader.size ||
      hiddwsctx->region.used + nr_relocs > hiddwsctx->region.size) {
      D(bug("[VMWareSVGA:Gallium] %s: not enough free space\n", __func__));
        return NULL;
    }

    hiddwsctx->command.reserved = nr_bytes;
    hiddwsctx->surface.reserved = nr_relocs;
    hiddwsctx->surface.staged = 0;
    hiddwsctx->shader.reserved = nr_relocs;
    hiddwsctx->shader.staged = 0;
    hiddwsctx->region.reserved = nr_relocs;
    hiddwsctx->region.staged = 0;

    D(bug("[VMWareSVGA:Gallium] %s: returning 0x%p\n", __func__, (hiddwsctx->command.buffer + hiddwsctx->command.used)));

    return hiddwsctx->command.buffer + hiddwsctx->command.used;
}

static unsigned
VMWareSVGA_WSCtx_GetCmdBuffSize(struct svga_winsys_context *swc)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    D(bug("[VMWareSVGA:Gallium] %s: returning %d\n", __func__, hiddwsctx->command.used));
    return hiddwsctx->command.used;
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
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));
}

static void
VMWareSVGA_WSCtx_QueryReloc(struct svga_winsys_context *swc,
                         SVGAMobId *id,
                         struct svga_winsys_gb_query *query)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));
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

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    hiddwsctx->command.used += hiddwsctx->command.reserved;
    hiddwsctx->command.reserved = 0;

    hiddwsctx->surface.used += hiddwsctx->surface.staged;
    hiddwsctx->surface.staged = 0;
    hiddwsctx->surface.reserved = 0;

    hiddwsctx->shader.used += hiddwsctx->shader.staged;
    hiddwsctx->shader.staged = 0;
    hiddwsctx->shader.reserved = 0;

    hiddwsctx->region.used += hiddwsctx->region.staged;
    hiddwsctx->region.staged = 0;
    hiddwsctx->region.reserved = 0;
}

static enum pipe_error
VMWareSVGA_WSCtx_Flush(struct svga_winsys_context *swc,
              struct pipe_fence_handle **pfence)
{
    struct HIDDGalliumVMWareSVGACtx *hiddwsctx = VMWareSVGA_WSCtx_HiddDataFromWinSys(swc);

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, swc));

    hiddwsctx->command.used = 0;
    hiddwsctx->command.reserved = 0;

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

    return PIPE_OK;
}

void VMWareSVGA_WSCtx_WinSysInit(struct HIDDGalliumVMWareSVGAData * data, struct HIDDGalliumVMWareSVGACtx *hiddwsctx)
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

   wsctx->imported_fence_fd = -1;

   wsctx->have_gb_objects = data->wssbase.have_gb_objects;

   hiddwsctx->command.size = VMW_COMMAND_SIZE;
   hiddwsctx->surface.size = VMW_SURFACE_RELOCS;
   hiddwsctx->shader.size = VMW_SHADER_RELOCS;
   hiddwsctx->region.size = VMW_REGION_RELOCS;
}
