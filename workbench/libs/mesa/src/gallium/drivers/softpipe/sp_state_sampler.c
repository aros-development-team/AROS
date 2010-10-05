/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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

/* Authors:
 *  Brian Paul
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "draw/draw_context.h"
#include "draw/draw_context.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"
#include "sp_tex_sample.h"
#include "sp_tex_tile_cache.h"


struct sp_sampler {
   struct pipe_sampler_state base;
   struct sp_sampler_varient *varients;
   struct sp_sampler_varient *current;
};

static struct sp_sampler *sp_sampler( struct pipe_sampler_state *sampler )
{
   return (struct sp_sampler *)sampler;
}


void *
softpipe_create_sampler_state(struct pipe_context *pipe,
                              const struct pipe_sampler_state *sampler)
{
   struct sp_sampler *sp_sampler = CALLOC_STRUCT(sp_sampler);

   sp_sampler->base = *sampler;
   sp_sampler->varients = NULL;

   return (void *)sp_sampler;
}


void
softpipe_bind_sampler_states(struct pipe_context *pipe,
                             unsigned num, void **sampler)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_samplers &&
       !memcmp(softpipe->sampler, sampler, num * sizeof(void *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < num; ++i)
      softpipe->sampler[i] = sampler[i];
   for (i = num; i < PIPE_MAX_SAMPLERS; ++i)
      softpipe->sampler[i] = NULL;

   softpipe->num_samplers = num;

   softpipe->dirty |= SP_NEW_SAMPLER;
}


void
softpipe_bind_vertex_sampler_states(struct pipe_context *pipe,
                                    unsigned num_samplers,
                                    void **samplers)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(num_samplers <= PIPE_MAX_VERTEX_SAMPLERS);

   /* Check for no-op */
   if (num_samplers == softpipe->num_vertex_samplers &&
       !memcmp(softpipe->vertex_samplers, samplers, num_samplers * sizeof(void *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < num_samplers; ++i)
      softpipe->vertex_samplers[i] = samplers[i];
   for (i = num_samplers; i < PIPE_MAX_VERTEX_SAMPLERS; ++i)
      softpipe->vertex_samplers[i] = NULL;

   softpipe->num_vertex_samplers = num_samplers;

   softpipe->dirty |= SP_NEW_SAMPLER;
}

void
softpipe_bind_geometry_sampler_states(struct pipe_context *pipe,
                                      unsigned num_samplers,
                                      void **samplers)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   assert(num_samplers <= PIPE_MAX_GEOMETRY_SAMPLERS);

   /* Check for no-op */
   if (num_samplers == softpipe->num_geometry_samplers &&
       !memcmp(softpipe->geometry_samplers, samplers, num_samplers * sizeof(void *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < num_samplers; ++i)
      softpipe->geometry_samplers[i] = samplers[i];
   for (i = num_samplers; i < PIPE_MAX_GEOMETRY_SAMPLERS; ++i)
      softpipe->geometry_samplers[i] = NULL;

   softpipe->num_geometry_samplers = num_samplers;

   softpipe->dirty |= SP_NEW_SAMPLER;
}


struct pipe_sampler_view *
softpipe_create_sampler_view(struct pipe_context *pipe,
                             struct pipe_resource *resource,
                             const struct pipe_sampler_view *templ)
{
   struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);

   if (view) {
      *view = *templ;
      view->reference.count = 1;
      view->texture = NULL;
      pipe_resource_reference(&view->texture, resource);
      view->context = pipe;
   }

   return view;
}


void
softpipe_sampler_view_destroy(struct pipe_context *pipe,
                              struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);
   FREE(view);
}


void
softpipe_set_sampler_views(struct pipe_context *pipe,
                           unsigned num,
                           struct pipe_sampler_view **views)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_sampler_views &&
       !memcmp(softpipe->sampler_views, views, num * sizeof(struct pipe_sampler_view *)))
      return;

   draw_flush(softpipe->draw);

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      pipe_sampler_view_reference(&softpipe->sampler_views[i], view);
      sp_tex_tile_cache_set_sampler_view(softpipe->tex_cache[i], view);
   }

   softpipe->num_sampler_views = num;

   softpipe->dirty |= SP_NEW_TEXTURE;
}


void
softpipe_set_vertex_sampler_views(struct pipe_context *pipe,
                                  unsigned num,
                                  struct pipe_sampler_view **views)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_VERTEX_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_vertex_sampler_views &&
       !memcmp(softpipe->vertex_sampler_views, views, num * sizeof(struct pipe_sampler_view *))) {
      return;
   }

   draw_flush(softpipe->draw);

   for (i = 0; i < PIPE_MAX_VERTEX_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      pipe_sampler_view_reference(&softpipe->vertex_sampler_views[i], view);
      sp_tex_tile_cache_set_sampler_view(softpipe->vertex_tex_cache[i], view);
   }

   softpipe->num_vertex_sampler_views = num;

   softpipe->dirty |= SP_NEW_TEXTURE;
}

void
softpipe_set_geometry_sampler_views(struct pipe_context *pipe,
                                    unsigned num,
                                    struct pipe_sampler_view **views)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   uint i;

   assert(num <= PIPE_MAX_GEOMETRY_SAMPLERS);

   /* Check for no-op */
   if (num == softpipe->num_geometry_sampler_views &&
       !memcmp(softpipe->geometry_sampler_views, views, num * sizeof(struct pipe_sampler_view *))) {
      return;
   }

   draw_flush(softpipe->draw);

   for (i = 0; i < PIPE_MAX_GEOMETRY_SAMPLERS; i++) {
      struct pipe_sampler_view *view = i < num ? views[i] : NULL;

      pipe_sampler_view_reference(&softpipe->geometry_sampler_views[i], view);
      sp_tex_tile_cache_set_sampler_view(softpipe->geometry_tex_cache[i], view);
   }

   softpipe->num_geometry_sampler_views = num;

   softpipe->dirty |= SP_NEW_TEXTURE;
}


/**
 * Find/create an sp_sampler_varient object for sampling the given texture,
 * sampler and tex unit.
 *
 * Note that the tex unit is significant.  We can't re-use a sampler
 * varient for multiple texture units because the sampler varient contains
 * the texture object pointer.  If the texture object pointer were stored
 * somewhere outside the sampler varient, we could re-use samplers for
 * multiple texture units.
 */
static struct sp_sampler_varient *
get_sampler_varient( unsigned unit,
                     struct sp_sampler *sampler,
                     struct pipe_resource *resource,
                     unsigned processor )
{
   struct softpipe_resource *sp_texture = softpipe_resource(resource);
   struct sp_sampler_varient *v = NULL;
   union sp_sampler_key key;

   /* if this fails, widen the key.unit field and update this assertion */
   assert(PIPE_MAX_SAMPLERS <= 16);

   key.bits.target = sp_texture->base.target;
   key.bits.is_pot = sp_texture->pot;
   key.bits.processor = processor;
   key.bits.unit = unit;
   key.bits.pad = 0;

   if (sampler->current && 
       key.value == sampler->current->key.value) {
      v = sampler->current;
   }

   if (v == NULL) {
      for (v = sampler->varients; v; v = v->next)
         if (v->key.value == key.value)
            break;

      if (v == NULL) {
         v = sp_create_sampler_varient( &sampler->base, key );
         v->next = sampler->varients;
         sampler->varients = v;
      }
   }
   
   sampler->current = v;
   return v;
}




void
softpipe_reset_sampler_varients(struct softpipe_context *softpipe)
{
   int i;

   /* It's a bit hard to build these samplers ahead of time -- don't
    * really know which samplers are going to be used for vertex and
    * fragment programs.
    */
   for (i = 0; i <= softpipe->vs->max_sampler; i++) {
      if (softpipe->vertex_samplers[i]) {
         struct pipe_resource *texture = NULL;

         if (softpipe->vertex_sampler_views[i]) {
            texture = softpipe->vertex_sampler_views[i]->texture;
         }

         softpipe->tgsi.vert_samplers_list[i] = 
            get_sampler_varient( i,
                                 sp_sampler(softpipe->vertex_samplers[i]),
                                 texture,
                                 TGSI_PROCESSOR_VERTEX );

         sp_sampler_varient_bind_texture( softpipe->tgsi.vert_samplers_list[i], 
                                          softpipe->vertex_tex_cache[i],
                                          texture );
      }
   }

   if (softpipe->gs) {
      for (i = 0; i <= softpipe->gs->max_sampler; i++) {
         if (softpipe->geometry_samplers[i]) {
            struct pipe_resource *texture = NULL;

            if (softpipe->geometry_sampler_views[i]) {
               texture = softpipe->geometry_sampler_views[i]->texture;
            }

            softpipe->tgsi.geom_samplers_list[i] =
               get_sampler_varient(
                  i,
                  sp_sampler(softpipe->geometry_samplers[i]),
                  texture,
                  TGSI_PROCESSOR_GEOMETRY );

            sp_sampler_varient_bind_texture(
               softpipe->tgsi.geom_samplers_list[i],
               softpipe->geometry_tex_cache[i],
               texture );
         }
      }
   }

   for (i = 0; i <= softpipe->fs->info.file_max[TGSI_FILE_SAMPLER]; i++) {
      if (softpipe->sampler[i]) {
         struct pipe_resource *texture = NULL;

         if (softpipe->sampler_views[i]) {
            texture = softpipe->sampler_views[i]->texture;
         }

         softpipe->tgsi.frag_samplers_list[i] =
            get_sampler_varient( i,
                                 sp_sampler(softpipe->sampler[i]),
                                 texture,
                                 TGSI_PROCESSOR_FRAGMENT );

         sp_sampler_varient_bind_texture( softpipe->tgsi.frag_samplers_list[i], 
                                          softpipe->tex_cache[i],
                                          texture );
      }
   }
}



void
softpipe_delete_sampler_state(struct pipe_context *pipe,
                              void *sampler)
{
   struct sp_sampler *sp_sampler = (struct sp_sampler *)sampler;
   struct sp_sampler_varient *v, *tmp;

   for (v = sp_sampler->varients; v; v = tmp) {
      tmp = v->next;
      sp_sampler_varient_destroy(v);
   }

   FREE( sampler );
}



