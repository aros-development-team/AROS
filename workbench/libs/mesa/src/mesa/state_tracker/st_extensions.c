/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2008 VMware, Inc.
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

#include "main/imports.h"
#include "main/context.h"
#include "main/macros.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"

#include "st_context.h"
#include "st_extensions.h"


static int _min(int a, int b)
{
   return (a < b) ? a : b;
}

static float _maxf(float a, float b)
{
   return (a > b) ? a : b;
}

static int _clamp(int a, int min, int max)
{
   if (a < min)
      return min;
   else if (a > max)
      return max;
   else
      return a;
}


/**
 * Query driver to get implementation limits.
 * Note that we have to limit/clamp against Mesa's internal limits too.
 */
void st_init_limits(struct st_context *st)
{
   struct pipe_screen *screen = st->pipe->screen;
   struct gl_constants *c = &st->ctx->Const;

   c->MaxTextureLevels
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS),
            MAX_TEXTURE_LEVELS);

   c->Max3DTextureLevels
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_3D_LEVELS),
            MAX_3D_TEXTURE_LEVELS);

   c->MaxCubeTextureLevels
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS),
            MAX_CUBE_TEXTURE_LEVELS);

   c->MaxTextureRectSize
      = _min(1 << (c->MaxTextureLevels - 1), MAX_TEXTURE_RECT_SIZE);

   c->MaxTextureImageUnits
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS),
            MAX_TEXTURE_IMAGE_UNITS);

   c->MaxVertexTextureImageUnits
      = _min(screen->get_param(screen, PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS),
             MAX_VERTEX_TEXTURE_IMAGE_UNITS);

   c->MaxCombinedTextureImageUnits
      = _min(screen->get_param(screen, PIPE_CAP_MAX_COMBINED_SAMPLERS),
             MAX_COMBINED_TEXTURE_IMAGE_UNITS);

   c->MaxTextureCoordUnits
      = _min(c->MaxTextureImageUnits, MAX_TEXTURE_COORD_UNITS);

   c->MaxTextureUnits = _min(c->MaxTextureImageUnits, c->MaxTextureCoordUnits);

   c->MaxDrawBuffers
      = _clamp(screen->get_param(screen, PIPE_CAP_MAX_RENDER_TARGETS),
              1, MAX_DRAW_BUFFERS);

   c->MaxLineWidth
      = _maxf(1.0f, screen->get_paramf(screen, PIPE_CAP_MAX_LINE_WIDTH));
   c->MaxLineWidthAA
      = _maxf(1.0f, screen->get_paramf(screen, PIPE_CAP_MAX_LINE_WIDTH_AA));

   c->MaxPointSize
      = _maxf(1.0f, screen->get_paramf(screen, PIPE_CAP_MAX_POINT_WIDTH));
   c->MaxPointSizeAA
      = _maxf(1.0f, screen->get_paramf(screen, PIPE_CAP_MAX_POINT_WIDTH_AA));
   /* called after _mesa_create_context/_mesa_init_point, fix default user
    * settable max point size up
    */
   st->ctx->Point.MaxSize = MAX2(c->MaxPointSize, c->MaxPointSizeAA);
   /* these are not queryable. Note that GL basically mandates a 1.0 minimum
    * for non-aa sizes, but we can go down to 0.0 for aa points.
    */
   c->MinPointSize = 1.0f;
   c->MinPointSizeAA = 0.0f;

   c->MaxTextureMaxAnisotropy
      = _maxf(2.0f, screen->get_paramf(screen, PIPE_CAP_MAX_TEXTURE_ANISOTROPY));

   c->MaxTextureLodBias
      = screen->get_paramf(screen, PIPE_CAP_MAX_TEXTURE_LOD_BIAS);

   c->MaxDrawBuffers
      = CLAMP(screen->get_param(screen, PIPE_CAP_MAX_RENDER_TARGETS),
              1, MAX_DRAW_BUFFERS);

   /* Is TGSI_OPCODE_CONT supported? */
   /* XXX separate query for early function return? */
   st->ctx->Shader.EmitContReturn =
      screen->get_param(screen, PIPE_CAP_TGSI_CONT_SUPPORTED);
}


/**
 * Use pipe_screen::get_param() to query PIPE_CAP_ values to determine
 * which GL extensions are supported.
 * Quite a few extensions are always supported because they are standard
 * features or can be built on top of other gallium features.
 * Some fine tuning may still be needed.
 */
void st_init_extensions(struct st_context *st)
{
   struct pipe_screen *screen = st->pipe->screen;
   GLcontext *ctx = st->ctx;

   /*
    * Extensions that are supported by all Gallium drivers:
    */
   ctx->Extensions.ARB_copy_buffer = GL_TRUE;
   ctx->Extensions.ARB_fragment_coord_conventions = GL_TRUE;
   ctx->Extensions.ARB_fragment_program = GL_TRUE;
   ctx->Extensions.ARB_map_buffer_range = GL_TRUE;
   ctx->Extensions.ARB_multisample = GL_TRUE;
   ctx->Extensions.ARB_texture_border_clamp = GL_TRUE; /* XXX temp */
   ctx->Extensions.ARB_texture_compression = GL_TRUE;
   ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
   ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
   ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
   ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.ARB_vertex_array_object = GL_TRUE;
   ctx->Extensions.ARB_vertex_buffer_object = GL_TRUE;
   ctx->Extensions.ARB_vertex_program = GL_TRUE;

   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_logic_op = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_blend_subtract = GL_TRUE;
   ctx->Extensions.EXT_framebuffer_blit = GL_TRUE;
   ctx->Extensions.EXT_framebuffer_object = GL_TRUE;
   ctx->Extensions.EXT_framebuffer_multisample = GL_TRUE;
   ctx->Extensions.EXT_fog_coord = GL_TRUE;
   ctx->Extensions.EXT_multi_draw_arrays = GL_TRUE;
   ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
   ctx->Extensions.EXT_point_parameters = GL_TRUE;
   ctx->Extensions.EXT_provoking_vertex = GL_TRUE;
   ctx->Extensions.EXT_secondary_color = GL_TRUE;
   ctx->Extensions.EXT_stencil_wrap = GL_TRUE;
   ctx->Extensions.EXT_texture_env_add = GL_TRUE;
   ctx->Extensions.EXT_texture_env_combine = GL_TRUE;
   ctx->Extensions.EXT_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.EXT_texture_lod_bias = GL_TRUE;
   ctx->Extensions.EXT_vertex_array_bgra = GL_TRUE;

   ctx->Extensions.APPLE_vertex_array_object = GL_TRUE;

   ctx->Extensions.NV_blend_square = GL_TRUE;
   ctx->Extensions.NV_texgen_reflection = GL_TRUE;
   ctx->Extensions.NV_texture_env_combine4 = GL_TRUE;

#if FEATURE_OES_draw_texture
   ctx->Extensions.OES_draw_texture = GL_TRUE;
#endif

   ctx->Extensions.SGI_color_matrix = GL_TRUE;
   ctx->Extensions.SGIS_generate_mipmap = GL_TRUE;

   /*
    * Extensions that depend on the driver/hardware:
    */
   if (screen->get_param(screen, PIPE_CAP_MAX_RENDER_TARGETS) > 0) {
      ctx->Extensions.ARB_draw_buffers = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_GLSL)) {
      ctx->Extensions.ARB_fragment_shader = GL_TRUE;
      ctx->Extensions.ARB_vertex_shader = GL_TRUE;
      ctx->Extensions.ARB_shader_objects = GL_TRUE;
      ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
      ctx->Extensions.ARB_shading_language_120 = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_MIRROR_REPEAT) > 0) {
      ctx->Extensions.ARB_texture_mirrored_repeat = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_BLEND_EQUATION_SEPARATE)) {
      ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_MIRROR_CLAMP) > 0) {
      ctx->Extensions.EXT_texture_mirror_clamp = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES)) {
      ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
      ctx->Extensions.NV_texture_rectangle = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS) > 1) {
      ctx->Extensions.ARB_multitexture = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TWO_SIDED_STENCIL)) {
      ctx->Extensions.ATI_separate_stencil = GL_TRUE;
      ctx->Extensions.EXT_stencil_two_side = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_ANISOTROPIC_FILTER)) {
      ctx->Extensions.EXT_texture_filter_anisotropic = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_POINT_SPRITE)) {
      ctx->Extensions.ARB_point_sprite = GL_TRUE;
      /* GL_NV_point_sprite is not supported by gallium because we don't
       * support the GL_POINT_SPRITE_R_MODE_NV option.
       */
   }

   if (screen->get_param(screen, PIPE_CAP_OCCLUSION_QUERY)) {
      ctx->Extensions.ARB_occlusion_query = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_SHADOW_MAP)) {
      ctx->Extensions.ARB_depth_texture = GL_TRUE;
      ctx->Extensions.ARB_shadow = GL_TRUE;
      ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
      /*ctx->Extensions.ARB_shadow_ambient = GL_TRUE;*/
   }

   /* GL_EXT_packed_depth_stencil requires both the ability to render to
    * a depth/stencil buffer and texture from depth/stencil source.
    */
   if (screen->is_format_supported(screen, PIPE_FORMAT_S8Z24_UNORM,
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0) &&
       screen->is_format_supported(screen, PIPE_FORMAT_S8Z24_UNORM,
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
   }
   else if (screen->is_format_supported(screen, PIPE_FORMAT_Z24S8_UNORM,
                                        PIPE_TEXTURE_2D, 
                                        PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0) &&
            screen->is_format_supported(screen, PIPE_FORMAT_Z24S8_UNORM,
                                        PIPE_TEXTURE_2D, 
                                        PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
   }

   /* sRGB support */
   if (screen->is_format_supported(screen, PIPE_FORMAT_A8B8G8R8_SRGB,
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0) ||
      screen->is_format_supported(screen, PIPE_FORMAT_B8G8R8A8_SRGB,
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
   }

   /* s3tc support */
   if (screen->is_format_supported(screen, PIPE_FORMAT_DXT5_RGBA,
                                   PIPE_TEXTURE_2D,
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0) &&
       (ctx->Mesa_DXTn ||
        screen->is_format_supported(screen, PIPE_FORMAT_DXT5_RGBA,
                                    PIPE_TEXTURE_2D,
                                    PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))) {
      ctx->Extensions.EXT_texture_compression_s3tc = GL_TRUE;
      ctx->Extensions.S3_s3tc = GL_TRUE;
   }

   /* ycbcr support */
   if (screen->is_format_supported(screen, PIPE_FORMAT_UYVY, 
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0) ||
       screen->is_format_supported(screen, PIPE_FORMAT_YUYV, 
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.MESA_ycbcr_texture = GL_TRUE;
   }

   /* GL_ARB_framebuffer_object */
   if (ctx->Extensions.EXT_packed_depth_stencil) {
      /* we support always support GL_EXT_framebuffer_blit */
      ctx->Extensions.ARB_framebuffer_object = GL_TRUE;
   }

   if (st->pipe->render_condition) {
      ctx->Extensions.NV_conditional_render = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_INDEP_BLEND_ENABLE)) {
      ctx->Extensions.EXT_draw_buffers2 = GL_TRUE;
   }

#if 0 /* not yet */
   if (screen->get_param(screen, PIPE_CAP_INDEP_BLEND_FUNC)) {
      ctx->Extensions.ARB_draw_buffers_blend = GL_TRUE;
   }
#endif
}
