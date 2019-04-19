/*
    Copyright 2015-2019, The AROS Development Team. All rights reserved.
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

#include "util/u_memory.h"
#include "pipebuffer/pb_buffer.h"

struct VMWareSVGAPBBuf
{
    struct pb_buffer pbbuf;
    void *map;
};

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_RAW   /* Big Endian Archs. */
#else
#define AROS_PIXFMT RECTFMT_BGRA32   /* Little Endian Archs. */
#endif


// ****************************************************************************
//                                               winsys/support functions
// ****************************************************************************

static SVGA3dHardwareVersion VMWareSVGA_GetHWVersion(struct svga_winsys_screen *sws)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

#if (0)
    if (sws->have_gb_objects)
    {
#endif
        D(bug("[VMWareSVGA:Gallium] %s: returning SVGA3D_HWVERSION_WS8_B1\n", __func__));
        return SVGA3D_HWVERSION_WS8_B1;
#if (0)
    }
    return (SVGA3dHardwareVersion) vws->ioctl.hwversion;
#else
    return 0;
#endif
}

static boolean VMWareSVGA_GetCap(struct svga_winsys_screen *sws,
              SVGA3dDevCapIndex index,
              SVGA3dDevCapResult *result)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

#if (0)
    if (index > vws->ioctl.num_cap_3d ||
        index >= SVGA3D_DEVCAP_MAX ||
        !vws->ioctl.cap_3d[index].has_cap)
#endif
        return FALSE;
#if (0)
   *result = vws->ioctl.cap_3d[index].result;
    return TRUE;
#endif
}
   
static struct svga_winsys_context *VMWareSVGA_ContextCreate(struct svga_winsys_screen *sws)
{
    struct svga_winsys_context *wsctx;

    D(bug("[VMWareSVGA:Gallium] %s(0x%p)\n", __func__, sws));

    wsctx = CALLOC_STRUCT(svga_winsys_context);

    D(bug("[VMWareSVGA:Gallium] %s: svga_winsys_context @ 0x%p\n", __func__, wsctx));

    return wsctx;
}

static struct svga_winsys_surface *VMWareSVGA_SurfaceCreate(
                    struct svga_winsys_screen *sws,
                     SVGA3dSurfaceAllFlags flags,
                     SVGA3dSurfaceFormat format,
                     unsigned usage,
                     SVGA3dSize size,
                     uint32 numLayers,
                     uint32 numMipLevels,
                     unsigned sampleCount)
{
    struct svga_winsys_surface *surf = NULL;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

#if (0)
    surf = CALLOC_STRUCT(svga_winsys_surface);
#endif

    return surf;
}

static struct svga_winsys_surface *VMWareSVGA_SurfaceFromHandle(struct svga_winsys_screen *sws,
                          struct winsys_handle *whandle,
                          SVGA3dSurfaceFormat *format)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return NULL;
}

static boolean VMWareSVGA_SurfaceGetHandle(struct svga_winsys_screen *sws,
                         struct svga_winsys_surface *surface,
                         unsigned stride,
                         struct winsys_handle *whandle)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return FALSE;
}

static boolean VMWareSVGA_SurfaceIsFlushed(struct svga_winsys_screen *sws,
                         struct svga_winsys_surface *surface)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return TRUE;
}

static void VMWareSVGA_SurfaceReference(struct svga_winsys_screen *sws,
			struct svga_winsys_surface **pdst,
			struct svga_winsys_surface *src)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}

static boolean VMWareSVGA_SurfaceCanCreate(struct svga_winsys_screen *sws,
                         SVGA3dSurfaceFormat format,
                         SVGA3dSize size,
                         uint32 numLayers,
                         uint32 numMipLevels,
                         uint32 numSamples)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return TRUE;
}

static struct svga_winsys_buffer *VMWareSVGA_BufferCreate( struct svga_winsys_screen *sws, 
	             unsigned alignment, 
	             unsigned usage,
	             unsigned size )
{
    struct VMWareSVGAPBBuf *buf;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    buf = CALLOC_STRUCT(VMWareSVGAPBBuf);

    return (struct svga_winsys_buffer *)buf;
}

static void *VMWareSVGA_BufferMap( struct svga_winsys_screen *sws, 
	          struct svga_winsys_buffer *buf,
		  unsigned usage )
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
    return ((struct VMWareSVGAPBBuf *)(buf))->map;
}
   
static void VMWareSVGA_BufferUnMap( struct svga_winsys_screen *sws, 
                    struct svga_winsys_buffer *buf )
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}

static void VMWareSVGA_BufferDestroy( struct svga_winsys_screen *sws,
	              struct svga_winsys_buffer *buf )
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
    FREE(buf);
}

static void VMWareSVGA_FenceReference( struct svga_winsys_screen *sws,
                       struct pipe_fence_handle **pdst,
                       struct pipe_fence_handle *src )
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}

static int VMWareSVGA_FenceSignalled( struct svga_winsys_screen *sws,
                           struct pipe_fence_handle *fence,
                           unsigned flag )
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return 0;
}

static int VMWareSVGA_FenceFinish( struct svga_winsys_screen *sws,
                        struct pipe_fence_handle *fence,
                        uint64_t timeout,
                        unsigned flag )
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return 0;
}

static struct svga_winsys_gb_shader *VMWareSVGA_ShaderCreate(struct svga_winsys_screen *sws,
		    SVGA3dShaderType shaderType,
		    const uint32 *bytecode,
		    uint32 bytecodeLen)
{
    struct svga_winsys_gb_shader *shader = NULL;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return shader;
}

static void VMWareSVGA_ShaderDestroy(struct svga_winsys_screen *sws,
		     struct svga_winsys_gb_shader *shader)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}

static struct svga_winsys_gb_query *VMWareSVGA_QueryCreate(struct svga_winsys_screen *sws, uint32 len)
{
    struct svga_winsys_gb_query *query = NULL;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return query;
}

static void VMWareSVGA_QueryDestroy(struct svga_winsys_screen *sws,
		    struct svga_winsys_gb_query *query)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}

static int VMWareSVGA_QueryInit(struct svga_winsys_screen *sws,
                       struct svga_winsys_gb_query *query,
                       unsigned offset,
                       SVGA3dQueryState queryState)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    return 0;
}

static void VMWareSVGA_QueryGetResult(struct svga_winsys_screen *sws,
                       struct svga_winsys_gb_query *query,
                       unsigned offset,
                       SVGA3dQueryState *queryState,
                       void *result, uint32 resultLen)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}



static void VMWareSVGA_StatsInc(enum svga_stats_count index)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}

static void VMWareSVGA_StatsTimePush(enum svga_stats_time index,
                                struct svga_winsys_stats_timeframe *tf)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}

static void VMWareSVGA_StatsTimePop()
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}


// ****************************************************************************
//                                                                Gallium Hidd Methods
// ****************************************************************************

OOP_Object *METHOD(GalliumVMWareSVGA, Root, New)
{
    IPTR interfaceVers;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (0)
    {
        struct HIDDGalliumVMWareSVGAData * data = OOP_INST_DATA(cl, o);

        data->wsi.destroy                       = NULL;
        data->wsi.get_hw_version                = VMWareSVGA_GetHWVersion;
        data->wsi.get_cap                       = VMWareSVGA_GetCap;

        data->wsi.context_create                = VMWareSVGA_ContextCreate;

        data->wsi.surface_create                = VMWareSVGA_SurfaceCreate;
        data->wsi.surface_is_flushed            = VMWareSVGA_SurfaceIsFlushed;
        data->wsi.surface_reference             = VMWareSVGA_SurfaceReference;
        data->wsi.surface_from_handle           = VMWareSVGA_SurfaceFromHandle;
        data->wsi.surface_get_handle            = VMWareSVGA_SurfaceGetHandle;
        data->wsi.surface_can_create            = VMWareSVGA_SurfaceCanCreate;

        data->wsi.buffer_create                 = VMWareSVGA_BufferCreate;
        data->wsi.buffer_map                    = VMWareSVGA_BufferMap;
        data->wsi.buffer_unmap                  = VMWareSVGA_BufferUnMap;
        data->wsi.buffer_destroy                = VMWareSVGA_BufferDestroy;
   
        data->wsi.fence_reference               = VMWareSVGA_FenceReference;
        data->wsi.fence_signalled               = VMWareSVGA_FenceSignalled;
        data->wsi.fence_finish                  = VMWareSVGA_FenceFinish;
#if (0)
        data->wsi.fence_get_fd = vmw_svga_winsys_fence_get_fd;
        data->wsi.fence_create_fd = vmw_svga_winsys_fence_create_fd;
        data->wsi.fence_server_sync = vmw_svga_winsys_fence_server_sync;
#endif

        data->wsi.shader_create                 = VMWareSVGA_ShaderCreate;
        data->wsi.shader_destroy                = VMWareSVGA_ShaderDestroy;

        data->wsi.query_create                  = VMWareSVGA_QueryCreate;
        data->wsi.query_destroy                 = VMWareSVGA_QueryDestroy;
        data->wsi.query_init                    = VMWareSVGA_QueryInit;
        data->wsi.query_get_result              = VMWareSVGA_QueryGetResult;

        data->wsi.stats_inc                     = VMWareSVGA_StatsInc;
        data->wsi.stats_time_push               = VMWareSVGA_StatsTimePush;
        data->wsi.stats_time_pop                = VMWareSVGA_StatsTimePop;
    }

    return o;
}

VOID METHOD(GalliumVMWareSVGA, Root, Dispose)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(GalliumVMWareSVGA, Root, Get)
{
    ULONG idx;

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            /* Overload the property */
            case aoHidd_Gallium_InterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;
        }
    }

    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(GalliumVMWareSVGA, Hidd_Gallium, CreatePipeScreen)
{
    struct HIDDGalliumVMWareSVGAData * data = OOP_INST_DATA(cl, o);
    struct pipe_screen *screen = NULL;

    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));

    screen = svga_screen_create(&data->wsi);

    D(bug("[VMWareSVGA:Gallium] %s: screen @ 0x%p\n", __func__, screen));

    return screen;

}

VOID METHOD(GalliumVMWareSVGA, Hidd_Gallium, DisplayResource)
{
    D(bug("[VMWareSVGA:Gallium] %s()\n", __func__));
}
