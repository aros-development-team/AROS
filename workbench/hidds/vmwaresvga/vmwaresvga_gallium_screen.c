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

#include "svga3d_surfacedefs.h"

// ****************************************************************************
//                      winsys screen support functions
// ****************************************************************************

static struct HIDDGalliumVMWareSVGAData *VMWareSVGA_WSScr_HiddDataFromWinSys(struct svga_winsys_screen *sws)
{
    return (struct HIDDGalliumVMWareSVGAData *)sws;
}

static SVGA3dHardwareVersion VMWareSVGA_WSScr_GetHWVersion(struct svga_winsys_screen *sws)
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);
    SVGA3dHardwareVersion retval;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    if (sws->have_gb_objects)
    {
        retval = SVGA3D_HWVERSION_WS8_B1;
    }
    else
    {
        volatile ULONG *fifo = data->hwdata->mmiobase;
        if (data->hwdata->fifocapabilities & SVGA_FIFO_CAP_3D_HWVERSION_REVISED)
            retval = (SVGA3dHardwareVersion)fifo[SVGA_FIFO_3D_HWVERSION_REVISED];
        else
            retval = (SVGA3dHardwareVersion)fifo[SVGA_FIFO_3D_HWVERSION];
    }

    D(bug("[VMWareSVGA:Gallium] %s: returning #%08x\n", __func__, retval);)

    return retval;
}

static void VMWareSVGA_WSScr_CopyDev3DCaps(struct HIDDGalliumVMWareSVGAData *data, ULONG capcnt)
{
    int i;

    D(bug("[VMWareSVGA:Gallium] %s(%d)\n", __func__, capcnt);)

    if (capcnt > SVGA3D_DEVCAP_MAX)
        capcnt = SVGA3D_DEVCAP_MAX;

    D(bug("[VMWareSVGA:Gallium] %s: copying %d capabilities ...\n", __func__, capcnt);)

    for (i = 0; i < capcnt; ++i) {
        if (i == SVGA3D_DEVCAP_DEAD5) // SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES is deprecated
        {
            data->cap_3d[i].has_cap = FALSE;
            data->cap_3d[i].result = (SVGA3dDevCapResult)0;
            D(bug("[VMWareSVGA:Gallium] %s:     ** deprecated\n", __func__);)
        }
        else
        {
            data->cap_3d[i].has_cap = TRUE;
            vmwareWriteReg(data->hwdata, SVGA_REG_DEV_CAP, i);
            data->cap_3d[i].result = (SVGA3dDevCapResult)vmwareReadReg(data->hwdata, SVGA_REG_DEV_CAP);
            D(bug("[VMWareSVGA:Gallium] %s:     %08x\n", __func__, data->cap_3d[i].result);)
        }
    }
}

static void VMWareSVGA_WSScr_InitHW3DCaps(struct HIDDGalliumVMWareSVGAData *data)
{
    ULONG cap_count;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)

    if ((data->use_gbobjects) && (data->hwdata->capabilities & SVGA_CAP_GBOBJECTS))
    {
        cap_count = SVGA3D_DEVCAP_MAX;
        data->size_3dcaps = cap_count * sizeof(ULONG);
    }
    else
    {
        cap_count = (SVGA_FIFO_3D_CAPS_LAST - SVGA_FIFO_3D_CAPS + 1);
        data->size_3dcaps = cap_count * sizeof(ULONG);
    }

    D(bug("[VMWareSVGA:Gallium] %s: cap size = %d\n", __func__, data->size_3dcaps);)

    data->cap_3d = AllocMem((cap_count * sizeof(struct VMWareSVGA3DCap)), MEMF_CLEAR|MEMF_ANY);

    D(bug("[VMWareSVGA:Gallium] %s: cap_3d[%d] allocated @ 0x%p\n", __func__, cap_count, data->cap_3d);)
    if ((data->use_gbobjects) && (data->hwdata->capabilities & SVGA_CAP_GBOBJECTS)) {
        VMWareSVGA_WSScr_CopyDev3DCaps(data, cap_count);
    } else {
        volatile ULONG *fifo = data->hwdata->mmiobase;
        int i;

        for (i = 0; i < cap_count; i++)
        {
            data->cap_3d[i].has_cap = TRUE;
            data->cap_3d[i].result = (SVGA3dDevCapResult)fifo[SVGA_FIFO_3D_CAPS + i];
        }
    }

    if (data->hwdata->capabilities & SVGA_CAP_GBOBJECTS) {
        data->hwdata->txrmax = vmwareReadReg(data->hwdata, SVGA_REG_MOB_MAX_SIZE);
    }
    else
        data->hwdata->txrmax = VMW_MAX_DEFAULT_TEXTURE_SIZE;

    D(bug("[VMWareSVGA:Gallium] %s: max texture size = %d\n", __func__, data->hwdata->txrmax);)
}

static boolean VMWareSVGA_WSScr_GetCap(struct svga_winsys_screen *sws,
              SVGA3dDevCapIndex index,
              SVGA3dDevCapResult *result)
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);

    D(bug("[VMWareSVGA:Gallium] %s(%d)\n", __func__, index);)

    if ((index > (data->size_3dcaps / sizeof(ULONG))) ||
        (index >= SVGA3D_DEVCAP_MAX) ||
        (!data->cap_3d[index].has_cap))
    {
        D(bug("[VMWareSVGA:Gallium] %s: unsupported\n", __func__);)
        return FALSE;
    }

   *result = data->cap_3d[index].result;
    D(bug("[VMWareSVGA:Gallium] %s: returning %08x\n", __func__, *result);)

    return TRUE;
}

/******************************/

static struct svga_winsys_buffer *VMWareSVGA_WSScr_BufferCreate( struct svga_winsys_screen *sws, 
	             unsigned alignment, 
	             unsigned usage,
	             unsigned size )
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);
    struct VMWareSVGAPBBuf *buf;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    buf = CALLOC_STRUCT(VMWareSVGAPBBuf);

    buf->allocated_size = size + alignment;
    if( !(buf->allocated_map = VMWareSVGA_MemAlloc(data->hwdata, buf->allocated_size) ) )
    {
        FREE(buf);
        return NULL;
    }
    if (alignment)
        buf->map = (APTR)(((IPTR)buf->allocated_map + (alignment - 1)) & ~(alignment - 1));
    else
        buf->map = buf->allocated_map;
    buf->size = size;
    
    return (struct svga_winsys_buffer *)buf;
}

static void *VMWareSVGA_WSScr_BufferMap( struct svga_winsys_screen *sws, 
	          struct svga_winsys_buffer *buf,
		  unsigned usage )
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));
    return ((struct VMWareSVGAPBBuf *)(buf))->map;
}
   
static void VMWareSVGA_WSScr_BufferUnMap( struct svga_winsys_screen *sws, 
                    struct svga_winsys_buffer *buf )
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));
}

static void VMWareSVGA_WSScr_BufferDestroy( struct svga_winsys_screen *sws,
	              struct svga_winsys_buffer *buf )
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);
    struct VMWareSVGAPBBuf *pbuf = (struct VMWareSVGAPBBuf *)buf;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    if (pbuf->allocated_map)
        VMWareSVGA_MemFree(data->hwdata, pbuf->allocated_map, pbuf->allocated_size);
    FREE(pbuf);
}

/******************************/

static struct svga_winsys_context *VMWareSVGA_WSScr_ContextCreate(struct svga_winsys_screen *sws)
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);
    struct HIDDGalliumVMWareSVGACtx     *hiddwsctx;
    struct svga_winsys_context          *wsctx;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    hiddwsctx = CALLOC_STRUCT(HIDDGalliumVMWareSVGACtx);
    wsctx = &hiddwsctx->wscbase;
    hiddwsctx->wscsws = sws;

    D(bug("[VMWareSVGA:Gallium] %s: svga_winsys_context @ 0x%p\n", __func__, wsctx));

    VMWareSVGA_WSCtx_WinSysInit(data, hiddwsctx);
    
    return wsctx;
}

static struct svga_winsys_surface *VMWareSVGA_WSScr_SurfaceCreate(
                    struct svga_winsys_screen *sws,
                     SVGA3dSurfaceAllFlags flags,
                     SVGA3dSurfaceFormat format,
                     unsigned usage,
                     SVGA3dSize size,
                     uint32 numLayers,
                     uint32 numMipLevels,
                     unsigned sampleCount)
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);
    struct HIDDGalliumVMWareSVGASurf *surface = NULL;
    struct pb_buffer *pb_buf;

    uint32_t buffer_size;
    uint32_t num_samples = 1;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    surface = CALLOC_STRUCT(HIDDGalliumVMWareSVGASurf);
    D(bug("[VMWareSVGA:Gallium] %s: HIDDGalliumVMWareSVGASurf @ 0x%p\n", __func__, surface));

    pipe_reference_init(&surface->refcnt, 1);

    /*
     * Used for the backing buffer GB surfaces, and to approximate
     * when to flush on non-GB hosts.
     */
    buffer_size = svga3dsurface_get_serialized_size_extended(format, size,
                                                            numMipLevels,
                                                            numLayers,
                                                            num_samples);
    if (flags & SVGA3D_SURFACE_BIND_STREAM_OUTPUT)
        buffer_size += sizeof(SVGA3dDXSOState);

    if (buffer_size <= data->hwdata->txrmax) {
        D(bug("[VMWareSVGA:Gallium] %s: buffsize = %d\n", __func__, buffer_size));

        // allocate page aligned gfx memory
        surface->surfbuf = VMWareSVGA_WSScr_BufferCreate(sws, 4096,
                                                   0,
                                                   buffer_size);

        D(bug("[VMWareSVGA:Gallium] %s: surface buffer @ 0x%p (allocated @ 0x%p, %d bytes)\n", __func__, ((struct VMWareSVGAPBBuf *)(surface->surfbuf))->map, ((struct VMWareSVGAPBBuf *)(surface->surfbuf))->allocated_map, ((struct VMWareSVGAPBBuf *)(surface->surfbuf))->allocated_size));
    }
    else
    {
        FREE(surface);
        return NULL;
    }
    return VMWareSVGA_WSSurf_WinSysSurfFromHiddSurf(surface);
}

static struct svga_winsys_surface *VMWareSVGA_WSScr_SurfaceFromHandle(struct svga_winsys_screen *sws,
                          struct winsys_handle *whandle,
                          SVGA3dSurfaceFormat *format)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    return NULL;
}

static boolean VMWareSVGA_WSScr_SurfaceGetHandle(struct svga_winsys_screen *sws,
                         struct svga_winsys_surface *surface,
                         unsigned stride,
                         struct winsys_handle *whandle)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    return FALSE;
}

static boolean VMWareSVGA_WSScr_SurfaceIsFlushed(struct svga_winsys_screen *sws,
                         struct svga_winsys_surface *surface)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    return TRUE;
}

static void VMWareSVGA_WSScr_SurfaceReference(struct svga_winsys_screen *sws,
			struct svga_winsys_surface **pdst,
			struct svga_winsys_surface *src)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    *pdst = src;
}

static boolean VMWareSVGA_WSScr_SurfaceCanCreate(struct svga_winsys_screen *sws,
                         SVGA3dSurfaceFormat format,
                         SVGA3dSize size,
                         uint32 numLayers,
                         uint32 numMipLevels,
                         uint32 numSamples)
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);
    uint32_t buffer_size;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    buffer_size = svga3dsurface_get_serialized_size(format, size, 
                                                   numMipLevels, 
                                                   numLayers);
    if (numSamples > 1)
      buffer_size *= numSamples;

    if (buffer_size > data->hwdata->txrmax) {
	return FALSE;
    }

    return TRUE;
}

static int
VMWareSVGA_WSScr_FenceGet(struct svga_winsys_screen *sws,
                             struct pipe_fence_handle *fence,
                             boolean duplicate)
{
    return (int)(IPTR)fence;
}

static void
VMWareSVGA_WSScr_FenceCreate(struct svga_winsys_screen *sws,
                                struct pipe_fence_handle **fence,
                                int32_t fd)
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);
    *fence = (struct pipe_fence_handle *)(IPTR)fenceVMWareSVGAFIFO(data->hwdata);
}

static void VMWareSVGA_WSScr_FenceReference( struct svga_winsys_screen *sws,
                       struct pipe_fence_handle **pdst,
                       struct pipe_fence_handle *src )
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    *pdst = src;
}

static int VMWareSVGA_WSScr_FenceSignalled( struct svga_winsys_screen *sws,
                           struct pipe_fence_handle *fence,
                           unsigned flag )
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    return 0;
}

static int VMWareSVGA_WSScr_FenceFinish( struct svga_winsys_screen *sws,
                        struct pipe_fence_handle *fence,
                        uint64_t timeout,
                        unsigned flag )
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    return 0;
}

static struct svga_winsys_gb_shader *VMWareSVGA_WSScr_ShaderCreate(struct svga_winsys_screen *sws,
		    SVGA3dShaderType shaderType,
		    const uint32 *bytecode,
		    uint32 bytecodeLen)
{
    struct HIDDGalliumVMWareSVGAData *data = VMWareSVGA_WSScr_HiddDataFromWinSys(sws);
    struct HIDDGalliumVMWareSVGAShader *shader = NULL;
    void *code;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    shader = CALLOC_STRUCT(HIDDGalliumVMWareSVGAShader);
    if(!shader)
        return NULL;

    pipe_reference_init(&shader->refcnt, 1);
    shader->shaderbuf = VMWareSVGA_WSScr_BufferCreate(sws, 64,
					       SVGA_BUFFER_USAGE_SHADER,
					       bytecodeLen);

    code = VMWareSVGA_WSScr_BufferMap(sws, shader->shaderbuf, PIPE_TRANSFER_WRITE);
    memcpy(code, bytecode, bytecodeLen);
    VMWareSVGA_WSScr_BufferUnMap(sws, shader->shaderbuf);

    if (!sws->have_vgpu10) {

    }

    return VMWareSVGA_WSSurf_WinsysShaderHiddShader(shader);
}

static void VMWareSVGA_WSScr_ShaderDestroy(struct svga_winsys_screen *sws,
		     struct svga_winsys_gb_shader *shader)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));
}

static struct svga_winsys_gb_query *VMWareSVGA_WSScr_QueryCreate(struct svga_winsys_screen *sws, uint32 len)
{
    struct svga_winsys_gb_query *query = NULL;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    return query;
}

static void VMWareSVGA_WSScr_QueryDestroy(struct svga_winsys_screen *sws,
		    struct svga_winsys_gb_query *query)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));
}

static int VMWareSVGA_WSScr_QueryInit(struct svga_winsys_screen *sws,
                       struct svga_winsys_gb_query *query,
                       unsigned offset,
                       SVGA3dQueryState queryState)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    return 0;
}

static void VMWareSVGA_WSScr_QueryGetResult(struct svga_winsys_screen *sws,
                       struct svga_winsys_gb_query *query,
                       unsigned offset,
                       SVGA3dQueryState *queryState,
                       void *result, uint32 resultLen)
{
    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));
}

static void VMWareSVGA_WSScr_StatsInc(enum svga_stats_count index)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)
}

static void VMWareSVGA_WSScr_StatsTimePush(enum svga_stats_time index,
                                struct svga_winsys_stats_timeframe *tf)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)
}

static void VMWareSVGA_WSScr_StatsTimePop()
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__);)
}

void VMWareSVGA_WSScr_WinSysInit(struct HIDDGalliumVMWareSVGAData * data)
{
    data->wssbase.destroy                       = NULL;
    data->wssbase.get_hw_version                = VMWareSVGA_WSScr_GetHWVersion;
    data->wssbase.get_cap                       = VMWareSVGA_WSScr_GetCap;

    data->wssbase.context_create                = VMWareSVGA_WSScr_ContextCreate;

    data->wssbase.surface_create                = VMWareSVGA_WSScr_SurfaceCreate;
    data->wssbase.surface_is_flushed            = VMWareSVGA_WSScr_SurfaceIsFlushed;
    data->wssbase.surface_reference             = VMWareSVGA_WSScr_SurfaceReference;
    data->wssbase.surface_from_handle           = VMWareSVGA_WSScr_SurfaceFromHandle;
    data->wssbase.surface_get_handle            = VMWareSVGA_WSScr_SurfaceGetHandle;
    data->wssbase.surface_can_create            = VMWareSVGA_WSScr_SurfaceCanCreate;

    data->wssbase.buffer_create                 = VMWareSVGA_WSScr_BufferCreate;
    data->wssbase.buffer_map                    = VMWareSVGA_WSScr_BufferMap;
    data->wssbase.buffer_unmap                  = VMWareSVGA_WSScr_BufferUnMap;
    data->wssbase.buffer_destroy                = VMWareSVGA_WSScr_BufferDestroy;

    data->wssbase.fence_get_fd                  = VMWareSVGA_WSScr_FenceGet;
    data->wssbase.fence_create_fd               = VMWareSVGA_WSScr_FenceCreate;
#if (0)
    data->wssbase.fence_server_sync              = vmw_svga_winsys_fence_server_sync;
#endif
    data->wssbase.fence_reference               = VMWareSVGA_WSScr_FenceReference;
    data->wssbase.fence_signalled               = VMWareSVGA_WSScr_FenceSignalled;
    data->wssbase.fence_finish                  = VMWareSVGA_WSScr_FenceFinish;

    data->wssbase.shader_create                 = VMWareSVGA_WSScr_ShaderCreate;
    data->wssbase.shader_destroy                = VMWareSVGA_WSScr_ShaderDestroy;

    data->wssbase.query_create                  = VMWareSVGA_WSScr_QueryCreate;
    data->wssbase.query_destroy                 = VMWareSVGA_WSScr_QueryDestroy;
    data->wssbase.query_init                    = VMWareSVGA_WSScr_QueryInit;
    data->wssbase.query_get_result              = VMWareSVGA_WSScr_QueryGetResult;

    data->wssbase.stats_inc                     = VMWareSVGA_WSScr_StatsInc;
    data->wssbase.stats_time_push               = VMWareSVGA_WSScr_StatsTimePush;
    data->wssbase.stats_time_pop                = VMWareSVGA_WSScr_StatsTimePop;

    data->use_gbobjects = TRUE;

    data->wssbase.have_gb_objects = FALSE;
    data->wssbase.have_gb_dma = FALSE;
    data->wssbase.need_to_rebind_resources = FALSE;

    if (data->use_gbobjects)
    {
        if (data->hwdata->capabilities & SVGA_CAP_GBOBJECTS)
            data->wssbase.have_gb_objects = TRUE;
    }

    VMWareSVGA_WSScr_InitHW3DCaps(data);
}
