/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: arosmesa_intel.c 32712 2010-02-21 12:04:21Z deadwood $
*/

#include "arosmesa_winsys.h"
#include "intel_drm_winsys.h"
#include "util/u_memory.h"

#define DEBUG 0
#include <aros/debug.h>

#include <proto/graphics.h>


static void
arosmesa_intel_display_surface(AROSMesaContext amesa,
                              struct pipe_surface *surf)
{
    bug("IMPLEMENT: arosmesa_intel_display_surface\n");
}

static void 
arosmesa_intel_flush_frontbuffer( struct pipe_screen *screen,
                            struct pipe_surface *surf,
                            void *context_private )
{
    /* No Op */
}

/* FIXME: should this be here? Maybe add open intel device function to libdrm? */
#include "arosdrm.h"

static void
intel_drm_winsys_destroy(struct intel_winsys *iws)
{
   struct intel_drm_winsys *idws = intel_drm_winsys(iws);

   drm_intel_bufmgr_destroy(idws->pools.gem);

   drmClose(idws->fd);
   
   FREE(idws);
}

static struct pipe_screen *
arosmesa_intel_create_screen( void )
{
   struct intel_drm_winsys *idws;
   unsigned int deviceID = 0x2772;  /* FIXME: hardcoded value */

   idws = CALLOC_STRUCT(intel_drm_winsys);
   if (!idws)
      return NULL;

   intel_drm_winsys_init_batchbuffer_functions(idws);
   intel_drm_winsys_init_buffer_functions(idws);
   intel_drm_winsys_init_fence_functions(idws);

   idws->fd = drmOpen("", "");
   idws->id = deviceID;
   idws->max_batch_size = 16 * 4096;

   idws->base.destroy = intel_drm_winsys_destroy;

   idws->pools.gem = drm_intel_bufmgr_gem_init(idws->fd, idws->max_batch_size);
   drm_intel_bufmgr_gem_enable_reuse(idws->pools.gem);

   idws->dump_cmd = debug_get_bool_option("INTEL_DUMP_CMD", FALSE);

   return i915_create_screen(&idws->base, deviceID);
}

static struct pipe_context *
arosmesa_intel_create_context( struct pipe_screen *pscreen )
{
    if (!pscreen)
        return NULL;

    return pscreen->context_create(pscreen, NULL);
}

static void
arosmesa_intel_cleanup( struct pipe_screen * screen )
{
    if (screen)
    {
        /* This also destroys the winsys */
        screen->destroy(screen);
    }
}

static struct pipe_surface *
arosmesa_intel_get_screen_surface(struct pipe_screen * screen, int width, int height, int bpp)
{
    bug("IMPLEMENT: arosmesa_intel_get_screen_surface\n");
    return NULL;
}

static void
arosmesa_intel_query_depth_stencil(int color, int * depth, int * stencil)
{
    /* FIXME: Are those values correct ? */
    (*depth)    = 24;
    (*stencil)  = 8;
}

struct arosmesa_driver arosmesa_intel_driver = 
{
    .create_pipe_screen = arosmesa_intel_create_screen,
    .create_pipe_context = arosmesa_intel_create_context,
    .display_surface = arosmesa_intel_display_surface,
    .get_screen_surface = arosmesa_intel_get_screen_surface,
    .cleanup = arosmesa_intel_cleanup,
    .query_depth_stencil = arosmesa_intel_query_depth_stencil,
};

