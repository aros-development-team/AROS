#include <stdint.h>
#include <string.h>

#include "native.h"
#include "util/u_inlines.h"
#include "state_tracker/drm_driver.h"

#ifdef HAVE_WAYLAND_BACKEND

#include <wayland-server.h>
#include <wayland-drm-server-protocol.h>

#include "native_wayland_drm_bufmgr_helper.h"

void *
egl_g3d_wl_drm_helper_reference_buffer(void *user_data, uint32_t name,
                                       int32_t width, int32_t height,
                                       uint32_t stride,
                                       struct wl_visual *visual)
{
   struct native_display *ndpy = user_data;
   struct pipe_resource templ;
   struct winsys_handle wsh;
   enum pipe_format format = PIPE_FORMAT_B8G8R8A8_UNORM;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = format;
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;

   memset(&wsh, 0, sizeof(wsh));
   wsh.handle = name;
   wsh.stride = stride;

   return ndpy->screen->resource_from_handle(ndpy->screen, &templ, &wsh);
}

void
egl_g3d_wl_drm_helper_unreference_buffer(void *user_data, void *buffer)
{
   struct pipe_resource *resource = buffer;

   pipe_resource_reference(&resource, NULL);
}

struct pipe_resource *
egl_g3d_wl_drm_common_wl_buffer_get_resource(struct native_display *ndpy,
                                             struct wl_buffer *buffer)
{
   return wayland_drm_buffer_get_buffer(buffer);
}

#endif
