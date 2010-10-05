/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008 VMware, Inc.  All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "tgsi/tgsi_exec.h"
#include "sp_clear.h"
#include "sp_context.h"
#include "sp_flush.h"
#include "sp_prim_vbuf.h"
#include "sp_state.h"
#include "sp_surface.h"
#include "sp_tile_cache.h"
#include "sp_tex_tile_cache.h"
#include "sp_texture.h"
#include "sp_query.h"



/**
 * Map any drawing surfaces which aren't already mapped
 */
void
softpipe_map_transfers(struct softpipe_context *sp)
{
   unsigned i;

   for (i = 0; i < sp->framebuffer.nr_cbufs; i++) {
      sp_tile_cache_map_transfers(sp->cbuf_cache[i]);
   }

   sp_tile_cache_map_transfers(sp->zsbuf_cache);
}


/**
 * Unmap any mapped drawing surfaces
 */
void
softpipe_unmap_transfers(struct softpipe_context *sp)
{
   uint i;

   for (i = 0; i < sp->framebuffer.nr_cbufs; i++) {
      sp_tile_cache_unmap_transfers(sp->cbuf_cache[i]);
   }

   sp_tile_cache_unmap_transfers(sp->zsbuf_cache);
}


static void
softpipe_destroy( struct pipe_context *pipe )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   uint i;

   if (softpipe->draw)
      draw_destroy( softpipe->draw );

   softpipe->quad.shade->destroy( softpipe->quad.shade );
   softpipe->quad.depth_test->destroy( softpipe->quad.depth_test );
   softpipe->quad.blend->destroy( softpipe->quad.blend );
   softpipe->quad.pstipple->destroy( softpipe->quad.pstipple );

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      sp_destroy_tile_cache(softpipe->cbuf_cache[i]);
      pipe_surface_reference(&softpipe->framebuffer.cbufs[i], NULL);
   }

   sp_destroy_tile_cache(softpipe->zsbuf_cache);
   pipe_surface_reference(&softpipe->framebuffer.zsbuf, NULL);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      sp_destroy_tex_tile_cache(softpipe->tex_cache[i]);
      pipe_sampler_view_reference(&softpipe->sampler_views[i], NULL);
   }

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      sp_destroy_tex_tile_cache(softpipe->vertex_tex_cache[i]);
      pipe_sampler_view_reference(&softpipe->vertex_sampler_views[i], NULL);
   }

   for (i = 0; i < PIPE_MAX_GEOMETRY_SAMPLERS; i++) {
      sp_destroy_tex_tile_cache(softpipe->geometry_tex_cache[i]);
      pipe_sampler_view_reference(&softpipe->geometry_sampler_views[i], NULL);
   }

   for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      uint j;

      for (j = 0; j < PIPE_MAX_CONSTANT_BUFFERS; j++) {
         if (softpipe->constants[i][j]) {
            pipe_resource_reference(&softpipe->constants[i][j], NULL);
         }
      }
   }

   tgsi_exec_machine_destroy(softpipe->fs_machine);

   FREE( softpipe );
}


/**
 * if (the texture is being used as a framebuffer surface)
 *    return PIPE_REFERENCED_FOR_WRITE
 * else if (the texture is a bound texture source)
 *    return PIPE_REFERENCED_FOR_READ
 * else
 *    return PIPE_UNREFERENCED
 */
static unsigned int
softpipe_is_resource_referenced( struct pipe_context *pipe,
				struct pipe_resource *texture,
				unsigned face, unsigned level)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   unsigned i;

   if (texture->target == PIPE_BUFFER)
      return PIPE_UNREFERENCED;
   
   /* check if any of the bound drawing surfaces are this texture */
   if (softpipe->dirty_render_cache) {
      for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++) {
         if (softpipe->framebuffer.cbufs[i] && 
             softpipe->framebuffer.cbufs[i]->texture == texture) {
            return PIPE_REFERENCED_FOR_WRITE;
         }
      }
      if (softpipe->framebuffer.zsbuf && 
          softpipe->framebuffer.zsbuf->texture == texture) {
         return PIPE_REFERENCED_FOR_WRITE;
      }
   }
   
   /* check if any of the tex_cache textures are this texture */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      if (softpipe->tex_cache[i] &&
          softpipe->tex_cache[i]->texture == texture)
         return PIPE_REFERENCED_FOR_READ;
   }
   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      if (softpipe->vertex_tex_cache[i] &&
          softpipe->vertex_tex_cache[i]->texture == texture)
         return PIPE_REFERENCED_FOR_READ;
   }
   for (i = 0; i < PIPE_MAX_GEOMETRY_SAMPLERS; i++) {
      if (softpipe->geometry_tex_cache[i] &&
          softpipe->geometry_tex_cache[i]->texture == texture)
         return PIPE_REFERENCED_FOR_READ;
   }

   return PIPE_UNREFERENCED;
}




static void
softpipe_render_condition( struct pipe_context *pipe,
                           struct pipe_query *query,
                           uint mode )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   softpipe->render_cond_query = query;
   softpipe->render_cond_mode = mode;
}



struct pipe_context *
softpipe_create_context( struct pipe_screen *screen,
			 void *priv )
{
   struct softpipe_context *softpipe = CALLOC_STRUCT(softpipe_context);
   uint i;

   util_init_math();

#ifdef PIPE_ARCH_X86
   softpipe->use_sse = !debug_get_bool_option( "GALLIUM_NOSSE", FALSE );
#else
   softpipe->use_sse = FALSE;
#endif

   softpipe->dump_fs = debug_get_bool_option( "GALLIUM_DUMP_FS", FALSE );
   softpipe->dump_gs = debug_get_bool_option( "SOFTPIPE_DUMP_GS", FALSE );

   softpipe->pipe.winsys = NULL;
   softpipe->pipe.screen = screen;
   softpipe->pipe.destroy = softpipe_destroy;
   softpipe->pipe.priv = priv;

   /* state setters */
   softpipe->pipe.create_blend_state = softpipe_create_blend_state;
   softpipe->pipe.bind_blend_state   = softpipe_bind_blend_state;
   softpipe->pipe.delete_blend_state = softpipe_delete_blend_state;

   softpipe->pipe.create_sampler_state = softpipe_create_sampler_state;
   softpipe->pipe.bind_fragment_sampler_states  = softpipe_bind_sampler_states;
   softpipe->pipe.bind_vertex_sampler_states = softpipe_bind_vertex_sampler_states;
   softpipe->pipe.bind_geometry_sampler_states = softpipe_bind_geometry_sampler_states;
   softpipe->pipe.delete_sampler_state = softpipe_delete_sampler_state;

   softpipe->pipe.create_depth_stencil_alpha_state = softpipe_create_depth_stencil_state;
   softpipe->pipe.bind_depth_stencil_alpha_state   = softpipe_bind_depth_stencil_state;
   softpipe->pipe.delete_depth_stencil_alpha_state = softpipe_delete_depth_stencil_state;

   softpipe->pipe.create_rasterizer_state = softpipe_create_rasterizer_state;
   softpipe->pipe.bind_rasterizer_state   = softpipe_bind_rasterizer_state;
   softpipe->pipe.delete_rasterizer_state = softpipe_delete_rasterizer_state;

   softpipe->pipe.create_fs_state = softpipe_create_fs_state;
   softpipe->pipe.bind_fs_state   = softpipe_bind_fs_state;
   softpipe->pipe.delete_fs_state = softpipe_delete_fs_state;

   softpipe->pipe.create_vs_state = softpipe_create_vs_state;
   softpipe->pipe.bind_vs_state   = softpipe_bind_vs_state;
   softpipe->pipe.delete_vs_state = softpipe_delete_vs_state;

   softpipe->pipe.create_gs_state = softpipe_create_gs_state;
   softpipe->pipe.bind_gs_state   = softpipe_bind_gs_state;
   softpipe->pipe.delete_gs_state = softpipe_delete_gs_state;

   softpipe->pipe.create_vertex_elements_state = softpipe_create_vertex_elements_state;
   softpipe->pipe.bind_vertex_elements_state = softpipe_bind_vertex_elements_state;
   softpipe->pipe.delete_vertex_elements_state = softpipe_delete_vertex_elements_state;

   softpipe->pipe.create_stream_output_state = softpipe_create_stream_output_state;
   softpipe->pipe.bind_stream_output_state = softpipe_bind_stream_output_state;
   softpipe->pipe.delete_stream_output_state = softpipe_delete_stream_output_state;

   softpipe->pipe.set_blend_color = softpipe_set_blend_color;
   softpipe->pipe.set_stencil_ref = softpipe_set_stencil_ref;
   softpipe->pipe.set_clip_state = softpipe_set_clip_state;
   softpipe->pipe.set_sample_mask = softpipe_set_sample_mask;
   softpipe->pipe.set_constant_buffer = softpipe_set_constant_buffer;
   softpipe->pipe.set_framebuffer_state = softpipe_set_framebuffer_state;
   softpipe->pipe.set_polygon_stipple = softpipe_set_polygon_stipple;
   softpipe->pipe.set_scissor_state = softpipe_set_scissor_state;
   softpipe->pipe.set_fragment_sampler_views = softpipe_set_sampler_views;
   softpipe->pipe.set_vertex_sampler_views = softpipe_set_vertex_sampler_views;
   softpipe->pipe.set_geometry_sampler_views = softpipe_set_geometry_sampler_views;
   softpipe->pipe.create_sampler_view = softpipe_create_sampler_view;
   softpipe->pipe.sampler_view_destroy = softpipe_sampler_view_destroy;
   softpipe->pipe.set_viewport_state = softpipe_set_viewport_state;
   softpipe->pipe.set_stream_output_buffers = softpipe_set_stream_output_buffers;
   softpipe->pipe.set_vertex_buffers = softpipe_set_vertex_buffers;
   softpipe->pipe.set_index_buffer = softpipe_set_index_buffer;

   softpipe->pipe.draw_vbo = softpipe_draw_vbo;
   softpipe->pipe.draw_stream_output = softpipe_draw_stream_output;

   softpipe->pipe.clear = softpipe_clear;
   softpipe->pipe.flush = softpipe_flush;

   softpipe->pipe.is_resource_referenced = softpipe_is_resource_referenced;

   softpipe_init_query_funcs( softpipe );
   softpipe_init_texture_funcs( &softpipe->pipe );

   softpipe->pipe.render_condition = softpipe_render_condition;

   /*
    * Alloc caches for accessing drawing surfaces and textures.
    * Must be before quad stage setup!
    */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
      softpipe->cbuf_cache[i] = sp_create_tile_cache( &softpipe->pipe );
   softpipe->zsbuf_cache = sp_create_tile_cache( &softpipe->pipe );

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++)
      softpipe->tex_cache[i] = sp_create_tex_tile_cache( &softpipe->pipe );
   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      softpipe->vertex_tex_cache[i] = sp_create_tex_tile_cache( &softpipe->pipe );
   }
   for (i = 0; i < PIPE_MAX_GEOMETRY_SAMPLERS; i++) {
      softpipe->geometry_tex_cache[i] = sp_create_tex_tile_cache( &softpipe->pipe );
   }

   softpipe->fs_machine = tgsi_exec_machine_create();

   /* setup quad rendering stages */
   softpipe->quad.shade = sp_quad_shade_stage(softpipe);
   softpipe->quad.depth_test = sp_quad_depth_test_stage(softpipe);
   softpipe->quad.blend = sp_quad_blend_stage(softpipe);
   softpipe->quad.pstipple = sp_quad_polygon_stipple_stage(softpipe);


   /*
    * Create drawing context and plug our rendering stage into it.
    */
   softpipe->draw = draw_create(&softpipe->pipe);
   if (!softpipe->draw) 
      goto fail;

   draw_texture_samplers(softpipe->draw,
                         PIPE_SHADER_VERTEX,
                         PIPE_MAX_VERTEX_SAMPLERS,
                         (struct tgsi_sampler **)
                            softpipe->tgsi.vert_samplers_list);

   draw_texture_samplers(softpipe->draw,
                         PIPE_SHADER_GEOMETRY,
                         PIPE_MAX_GEOMETRY_SAMPLERS,
                         (struct tgsi_sampler **)
                            softpipe->tgsi.geom_samplers_list);

   if (debug_get_bool_option( "SP_NO_RAST", FALSE ))
      softpipe->no_rast = TRUE;

   softpipe->vbuf_backend = sp_create_vbuf_backend(softpipe);
   if (!softpipe->vbuf_backend)
      goto fail;

   softpipe->vbuf = draw_vbuf_stage(softpipe->draw, softpipe->vbuf_backend);
   if (!softpipe->vbuf)
      goto fail;

   draw_set_rasterize_stage(softpipe->draw, softpipe->vbuf);
   draw_set_render(softpipe->draw, softpipe->vbuf_backend);


   /* plug in AA line/point stages */
   draw_install_aaline_stage(softpipe->draw, &softpipe->pipe);
   draw_install_aapoint_stage(softpipe->draw, &softpipe->pipe);

   /* Do polygon stipple w/ texture map + frag prog? */
#if DO_PSTIPPLE_IN_DRAW_MODULE
   draw_install_pstipple_stage(softpipe->draw, &softpipe->pipe);
#endif

   draw_wide_point_sprites(softpipe->draw, TRUE);

   sp_init_surface_functions(softpipe);

   return &softpipe->pipe;

 fail:
   softpipe_destroy(&softpipe->pipe);
   return NULL;
}
