/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_transfer.h"

#include "tgsi/tgsi_parse.h"

#include "nvc0_stateobj.h"
#include "nvc0_context.h"

#include "nvc0_3d.xml.h"
#include "nv50/nv50_texture.xml.h"

#include "nouveau/nouveau_gldefs.h"

static INLINE uint32_t
nvc0_colormask(unsigned mask)
{
    uint32_t ret = 0;

    if (mask & PIPE_MASK_R)
        ret |= 0x0001;
    if (mask & PIPE_MASK_G)
        ret |= 0x0010;
    if (mask & PIPE_MASK_B)
        ret |= 0x0100;
    if (mask & PIPE_MASK_A)
        ret |= 0x1000;

    return ret;
}

#define NVC0_BLEND_FACTOR_CASE(a, b) \
   case PIPE_BLENDFACTOR_##a: return NV50_3D_BLEND_FACTOR_##b

static INLINE uint32_t
nvc0_blend_fac(unsigned factor)
{
   switch (factor) {
   NVC0_BLEND_FACTOR_CASE(ONE, ONE);
   NVC0_BLEND_FACTOR_CASE(SRC_COLOR, SRC_COLOR);
   NVC0_BLEND_FACTOR_CASE(SRC_ALPHA, SRC_ALPHA);
   NVC0_BLEND_FACTOR_CASE(DST_ALPHA, DST_ALPHA);
   NVC0_BLEND_FACTOR_CASE(DST_COLOR, DST_COLOR);
   NVC0_BLEND_FACTOR_CASE(SRC_ALPHA_SATURATE, SRC_ALPHA_SATURATE);
   NVC0_BLEND_FACTOR_CASE(CONST_COLOR, CONSTANT_COLOR);
   NVC0_BLEND_FACTOR_CASE(CONST_ALPHA, CONSTANT_ALPHA);
   NVC0_BLEND_FACTOR_CASE(SRC1_COLOR, SRC1_COLOR);
   NVC0_BLEND_FACTOR_CASE(SRC1_ALPHA, SRC1_ALPHA);
   NVC0_BLEND_FACTOR_CASE(ZERO, ZERO);
   NVC0_BLEND_FACTOR_CASE(INV_SRC_COLOR, ONE_MINUS_SRC_COLOR);
   NVC0_BLEND_FACTOR_CASE(INV_SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
   NVC0_BLEND_FACTOR_CASE(INV_DST_ALPHA, ONE_MINUS_DST_ALPHA);
   NVC0_BLEND_FACTOR_CASE(INV_DST_COLOR, ONE_MINUS_DST_COLOR);
   NVC0_BLEND_FACTOR_CASE(INV_CONST_COLOR, ONE_MINUS_CONSTANT_COLOR);
   NVC0_BLEND_FACTOR_CASE(INV_CONST_ALPHA, ONE_MINUS_CONSTANT_ALPHA);
   NVC0_BLEND_FACTOR_CASE(INV_SRC1_COLOR, ONE_MINUS_SRC1_COLOR);
   NVC0_BLEND_FACTOR_CASE(INV_SRC1_ALPHA, ONE_MINUS_SRC1_ALPHA);
   default:
      return NV50_3D_BLEND_FACTOR_ZERO;
   }
}

static void *
nvc0_blend_state_create(struct pipe_context *pipe,
                        const struct pipe_blend_state *cso)
{
    struct nvc0_blend_stateobj *so = CALLOC_STRUCT(nvc0_blend_stateobj);
    int i;

    so->pipe = *cso;

    SB_IMMED_3D(so, BLEND_INDEPENDENT, cso->independent_blend_enable);

    if (!cso->logicop_enable)
       SB_IMMED_3D(so, LOGIC_OP_ENABLE, 0);

    if (cso->logicop_enable) {
       SB_BEGIN_3D(so, LOGIC_OP_ENABLE, 2);
       SB_DATA    (so, 1);
       SB_DATA    (so, nvgl_logicop_func(cso->logicop_func));

       SB_IMMED_3D(so, BLEND_ENABLES, 0);
    } else
    if (!cso->independent_blend_enable) {
        SB_IMMED_3D(so, BLEND_ENABLES, cso->rt[0].blend_enable ? 0xff : 0);

        if (cso->rt[0].blend_enable) {
            SB_BEGIN_3D(so, BLEND_EQUATION_RGB, 5);
            SB_DATA    (so, nvgl_blend_eqn(cso->rt[0].rgb_func));
            SB_DATA    (so, nvc0_blend_fac(cso->rt[0].rgb_src_factor));
            SB_DATA    (so, nvc0_blend_fac(cso->rt[0].rgb_dst_factor));
            SB_DATA    (so, nvgl_blend_eqn(cso->rt[0].alpha_func));
            SB_DATA    (so, nvc0_blend_fac(cso->rt[0].alpha_src_factor));
            SB_BEGIN_3D(so, BLEND_FUNC_DST_ALPHA, 1);
            SB_DATA    (so, nvc0_blend_fac(cso->rt[0].alpha_dst_factor));
        }

        SB_IMMED_3D(so, COLOR_MASK_COMMON, 1);
        SB_BEGIN_3D(so, COLOR_MASK(0), 1);
        SB_DATA    (so, nvc0_colormask(cso->rt[0].colormask));
    } else {
        uint8_t en = 0;

        for (i = 0; i < 8; ++i) {
            if (!cso->rt[i].blend_enable)
                continue;
            en |= 1 << i;

            SB_BEGIN_3D(so, IBLEND_EQUATION_RGB(i), 6);
            SB_DATA    (so, nvgl_blend_eqn(cso->rt[i].rgb_func));
            SB_DATA    (so, nvc0_blend_fac(cso->rt[i].rgb_src_factor));
            SB_DATA    (so, nvc0_blend_fac(cso->rt[i].rgb_dst_factor));
            SB_DATA    (so, nvgl_blend_eqn(cso->rt[i].alpha_func));
            SB_DATA    (so, nvc0_blend_fac(cso->rt[i].alpha_src_factor));
            SB_DATA    (so, nvc0_blend_fac(cso->rt[i].alpha_dst_factor));
        }
        SB_IMMED_3D(so, BLEND_ENABLES, en);

        SB_IMMED_3D(so, COLOR_MASK_COMMON, 0);
        SB_BEGIN_3D(so, COLOR_MASK(0), 8);
        for (i = 0; i < 8; ++i)
            SB_DATA(so, nvc0_colormask(cso->rt[i].colormask));
    }

    assert(so->size <= (sizeof(so->state) / sizeof(so->state[0])));
    return so;
}

static void
nvc0_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->blend = hwcso;
    nvc0->dirty |= NVC0_NEW_BLEND;
}

static void
nvc0_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
    FREE(hwcso);
}

/* NOTE: ignoring line_last_pixel, using FALSE (set on screen init) */
static void *
nvc0_rasterizer_state_create(struct pipe_context *pipe,
                             const struct pipe_rasterizer_state *cso)
{
    struct nvc0_rasterizer_stateobj *so;
    uint32_t reg;

    so = CALLOC_STRUCT(nvc0_rasterizer_stateobj);
    if (!so)
        return NULL;
    so->pipe = *cso;

    /* Scissor enables are handled in scissor state, we will not want to
     * always emit 16 commands, one for each scissor rectangle, here.
     */
    
    SB_BEGIN_3D(so, SHADE_MODEL, 1);
    SB_DATA    (so, cso->flatshade ? NVC0_3D_SHADE_MODEL_FLAT :
                                     NVC0_3D_SHADE_MODEL_SMOOTH);
    SB_IMMED_3D(so, PROVOKING_VERTEX_LAST, !cso->flatshade_first);
    SB_IMMED_3D(so, VERTEX_TWO_SIDE_ENABLE, cso->light_twoside);

    SB_IMMED_3D(so, VERT_COLOR_CLAMP_EN, cso->clamp_vertex_color);
    SB_BEGIN_3D(so, FRAG_COLOR_CLAMP_EN, 1);
    SB_DATA    (so, cso->clamp_fragment_color ? 0x11111111 : 0x00000000);

    SB_IMMED_3D(so, LINE_SMOOTH_ENABLE, cso->line_smooth);
    if (cso->line_smooth)
       SB_BEGIN_3D(so, LINE_WIDTH_SMOOTH, 1);
    else
       SB_BEGIN_3D(so, LINE_WIDTH_ALIASED, 1);
    SB_DATA    (so, fui(cso->line_width));

    SB_IMMED_3D(so, LINE_STIPPLE_ENABLE, cso->line_stipple_enable);
    if (cso->line_stipple_enable) {
        SB_BEGIN_3D(so, LINE_STIPPLE_PATTERN, 1);
        SB_DATA    (so, (cso->line_stipple_pattern << 8) |
                         cso->line_stipple_factor);
                    
    }

    SB_IMMED_3D(so, VP_POINT_SIZE_EN, cso->point_size_per_vertex);
    if (!cso->point_size_per_vertex) {
       SB_BEGIN_3D(so, POINT_SIZE, 1);
       SB_DATA    (so, fui(cso->point_size));
    }

    reg = (cso->sprite_coord_mode == PIPE_SPRITE_COORD_UPPER_LEFT) ?
       NVC0_3D_POINT_COORD_REPLACE_COORD_ORIGIN_UPPER_LEFT :
       NVC0_3D_POINT_COORD_REPLACE_COORD_ORIGIN_LOWER_LEFT;

    SB_BEGIN_3D(so, POINT_COORD_REPLACE, 1);
    SB_DATA    (so, ((cso->sprite_coord_enable & 0xff) << 3) | reg);
    SB_IMMED_3D(so, POINT_SPRITE_ENABLE, cso->point_quad_rasterization);
    SB_IMMED_3D(so, POINT_SMOOTH_ENABLE, cso->point_smooth);

    SB_BEGIN_3D(so, POLYGON_MODE_FRONT, 1);
    SB_DATA    (so, nvgl_polygon_mode(cso->fill_front));
    SB_BEGIN_3D(so, POLYGON_MODE_BACK, 1);
    SB_DATA    (so, nvgl_polygon_mode(cso->fill_back));
    SB_IMMED_3D(so, POLYGON_SMOOTH_ENABLE, cso->poly_smooth);

    SB_BEGIN_3D(so, CULL_FACE_ENABLE, 3);
    SB_DATA    (so, cso->cull_face != PIPE_FACE_NONE);
    SB_DATA    (so, cso->front_ccw ? NVC0_3D_FRONT_FACE_CCW :
                                     NVC0_3D_FRONT_FACE_CW);
    switch (cso->cull_face) {
    case PIPE_FACE_FRONT_AND_BACK:
       SB_DATA(so, NVC0_3D_CULL_FACE_FRONT_AND_BACK);
       break;
    case PIPE_FACE_FRONT:
       SB_DATA(so, NVC0_3D_CULL_FACE_FRONT);
       break;
    case PIPE_FACE_BACK:
    default:
       SB_DATA(so, NVC0_3D_CULL_FACE_BACK);
       break;
    }

    SB_IMMED_3D(so, POLYGON_STIPPLE_ENABLE, cso->poly_stipple_enable);
    SB_BEGIN_3D(so, POLYGON_OFFSET_POINT_ENABLE, 3);
    SB_DATA    (so, cso->offset_point);
    SB_DATA    (so, cso->offset_line);
    SB_DATA    (so, cso->offset_tri);

    if (cso->offset_point || cso->offset_line || cso->offset_tri) {
        SB_BEGIN_3D(so, POLYGON_OFFSET_FACTOR, 1);
        SB_DATA    (so, fui(cso->offset_scale));
        SB_BEGIN_3D(so, POLYGON_OFFSET_UNITS, 1);
        SB_DATA    (so, fui(cso->offset_units * 2.0f));
    }

    assert(so->size <= (sizeof(so->state) / sizeof(so->state[0])));
    return (void *)so;
}

static void
nvc0_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);

   nvc0->rast = hwcso;
   nvc0->dirty |= NVC0_NEW_RASTERIZER;
}

static void
nvc0_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
   FREE(hwcso);
}

static void *
nvc0_zsa_state_create(struct pipe_context *pipe,
                      const struct pipe_depth_stencil_alpha_state *cso)
{
   struct nvc0_zsa_stateobj *so = CALLOC_STRUCT(nvc0_zsa_stateobj);

   so->pipe = *cso;

   SB_IMMED_3D(so, DEPTH_TEST_ENABLE, cso->depth.enabled);
   if (cso->depth.enabled) {
      SB_IMMED_3D(so, DEPTH_WRITE_ENABLE, cso->depth.writemask);
      SB_BEGIN_3D(so, DEPTH_TEST_FUNC, 1);
      SB_DATA    (so, nvgl_comparison_op(cso->depth.func));
   }

   if (cso->stencil[0].enabled) {
      SB_BEGIN_3D(so, STENCIL_ENABLE, 5);
      SB_DATA    (so, 1);
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].fail_op));
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].zfail_op));
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].zpass_op));
      SB_DATA    (so, nvgl_comparison_op(cso->stencil[0].func));
      SB_BEGIN_3D(so, STENCIL_FRONT_FUNC_MASK, 2);
      SB_DATA    (so, cso->stencil[0].valuemask);
      SB_DATA    (so, cso->stencil[0].writemask);
   } else {
      SB_IMMED_3D(so, STENCIL_ENABLE, 0);
   }

   if (cso->stencil[1].enabled) {
      assert(cso->stencil[0].enabled);
      SB_BEGIN_3D(so, STENCIL_TWO_SIDE_ENABLE, 5);
      SB_DATA    (so, 1);
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].fail_op));
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].zfail_op));
      SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].zpass_op));
      SB_DATA    (so, nvgl_comparison_op(cso->stencil[1].func));
      SB_BEGIN_3D(so, STENCIL_BACK_MASK, 2);
      SB_DATA    (so, cso->stencil[1].writemask);
      SB_DATA    (so, cso->stencil[1].valuemask);
   } else
   if (cso->stencil[0].enabled) {
      SB_IMMED_3D(so, STENCIL_TWO_SIDE_ENABLE, 0);
   }

   SB_IMMED_3D(so, ALPHA_TEST_ENABLE, cso->alpha.enabled);
   if (cso->alpha.enabled) {
      SB_BEGIN_3D(so, ALPHA_TEST_REF, 2);
      SB_DATA    (so, fui(cso->alpha.ref_value));
      SB_DATA    (so, nvgl_comparison_op(cso->alpha.func));
   }

   assert(so->size <= (sizeof(so->state) / sizeof(so->state[0])));
   return (void *)so;
}

static void
nvc0_zsa_state_bind(struct pipe_context *pipe, void *hwcso)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);

   nvc0->zsa = hwcso;
   nvc0->dirty |= NVC0_NEW_ZSA;
}

static void
nvc0_zsa_state_delete(struct pipe_context *pipe, void *hwcso)
{
   FREE(hwcso);
}

/* ====================== SAMPLERS AND TEXTURES ================================
 */

#define NV50_TSC_WRAP_CASE(n) \
    case PIPE_TEX_WRAP_##n: return NV50_TSC_WRAP_##n

static INLINE unsigned
nv50_tsc_wrap_mode(unsigned wrap)
{
   switch (wrap) {
   NV50_TSC_WRAP_CASE(REPEAT);
   NV50_TSC_WRAP_CASE(MIRROR_REPEAT);
   NV50_TSC_WRAP_CASE(CLAMP_TO_EDGE);
   NV50_TSC_WRAP_CASE(CLAMP_TO_BORDER);
   NV50_TSC_WRAP_CASE(CLAMP);
   NV50_TSC_WRAP_CASE(MIRROR_CLAMP_TO_EDGE);
   NV50_TSC_WRAP_CASE(MIRROR_CLAMP_TO_BORDER);
   NV50_TSC_WRAP_CASE(MIRROR_CLAMP);
   default:
       NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
       return NV50_TSC_WRAP_REPEAT;
   }
}

static void
nvc0_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
   unsigned s, i;

   for (s = 0; s < 5; ++s)
      for (i = 0; i < nvc0_context(pipe)->num_samplers[s]; ++i)
         if (nvc0_context(pipe)->samplers[s][i] == hwcso)
            nvc0_context(pipe)->samplers[s][i] = NULL;

   nvc0_screen_tsc_free(nvc0_context(pipe)->screen, nv50_tsc_entry(hwcso));

   FREE(hwcso);
}

static INLINE void
nvc0_stage_sampler_states_bind(struct nvc0_context *nvc0, int s,
                               unsigned nr, void **hwcso)
{
   unsigned i;

   for (i = 0; i < nr; ++i) {
      struct nv50_tsc_entry *old = nvc0->samplers[s][i];

      nvc0->samplers[s][i] = nv50_tsc_entry(hwcso[i]);
      if (old)
         nvc0_screen_tsc_unlock(nvc0->screen, old);
   }
   for (; i < nvc0->num_samplers[s]; ++i)
      if (nvc0->samplers[s][i])
         nvc0_screen_tsc_unlock(nvc0->screen, nvc0->samplers[s][i]);

   nvc0->num_samplers[s] = nr;

   nvc0->dirty |= NVC0_NEW_SAMPLERS;
}

static void
nvc0_vp_sampler_states_bind(struct pipe_context *pipe, unsigned nr, void **s)
{
   nvc0_stage_sampler_states_bind(nvc0_context(pipe), 0, nr, s);
}

static void
nvc0_fp_sampler_states_bind(struct pipe_context *pipe, unsigned nr, void **s)
{
   nvc0_stage_sampler_states_bind(nvc0_context(pipe), 4, nr, s);
}

static void
nvc0_gp_sampler_states_bind(struct pipe_context *pipe, unsigned nr, void **s)
{
   nvc0_stage_sampler_states_bind(nvc0_context(pipe), 3, nr, s);
}

/* NOTE: only called when not referenced anywhere, won't be bound */
static void
nvc0_sampler_view_destroy(struct pipe_context *pipe,
                          struct pipe_sampler_view *view)
{
   pipe_resource_reference(&view->texture, NULL);

   nvc0_screen_tic_free(nvc0_context(pipe)->screen, nv50_tic_entry(view));

   FREE(nv50_tic_entry(view));
}

static INLINE void
nvc0_stage_set_sampler_views(struct nvc0_context *nvc0, int s,
                             unsigned nr,
                             struct pipe_sampler_view **views)
{
   unsigned i;

   for (i = 0; i < nr; ++i) {
      struct nv50_tic_entry *old = nv50_tic_entry(nvc0->textures[s][i]);
      if (old)
         nvc0_screen_tic_unlock(nvc0->screen, old);

      pipe_sampler_view_reference(&nvc0->textures[s][i], views[i]);
   }

   for (i = nr; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *old = nv50_tic_entry(nvc0->textures[s][i]);
      if (!old)
         continue;
      nvc0_screen_tic_unlock(nvc0->screen, old);

      pipe_sampler_view_reference(&nvc0->textures[s][i], NULL);
   }

   nvc0->num_textures[s] = nr;

   nvc0_bufctx_reset(nvc0, NVC0_BUFCTX_TEXTURES);

   nvc0->dirty |= NVC0_NEW_TEXTURES;
}

static void
nvc0_vp_set_sampler_views(struct pipe_context *pipe,
                          unsigned nr,
                          struct pipe_sampler_view **views)
{
   nvc0_stage_set_sampler_views(nvc0_context(pipe), 0, nr, views);
}

static void
nvc0_fp_set_sampler_views(struct pipe_context *pipe,
                          unsigned nr,
                          struct pipe_sampler_view **views)
{
   nvc0_stage_set_sampler_views(nvc0_context(pipe), 4, nr, views);
}

static void
nvc0_gp_set_sampler_views(struct pipe_context *pipe,
                          unsigned nr,
                          struct pipe_sampler_view **views)
{
   nvc0_stage_set_sampler_views(nvc0_context(pipe), 3, nr, views);
}

/* ============================= SHADERS =======================================
 */

static void *
nvc0_sp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso, unsigned type)
{
   struct nvc0_program *prog;

   prog = CALLOC_STRUCT(nvc0_program);
   if (!prog)
      return NULL;

   prog->type = type;
   prog->pipe.tokens = tgsi_dup_tokens(cso->tokens);

   return (void *)prog;
}

static void
nvc0_sp_state_delete(struct pipe_context *pipe, void *hwcso)
{
   struct nvc0_program *prog = (struct nvc0_program *)hwcso;

   nvc0_program_destroy(nvc0_context(pipe), prog);

   FREE((void *)prog->pipe.tokens);
   FREE(prog);
}

static void *
nvc0_vp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso)
{
   return nvc0_sp_state_create(pipe, cso, PIPE_SHADER_VERTEX);
}

static void
nvc0_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->vertprog = hwcso;
    nvc0->dirty |= NVC0_NEW_VERTPROG;
}

static void *
nvc0_fp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso)
{
   return nvc0_sp_state_create(pipe, cso, PIPE_SHADER_FRAGMENT);
}

static void
nvc0_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->fragprog = hwcso;
    nvc0->dirty |= NVC0_NEW_FRAGPROG;
}

static void *
nvc0_gp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso)
{
   return nvc0_sp_state_create(pipe, cso, PIPE_SHADER_GEOMETRY);
}

static void
nvc0_gp_state_bind(struct pipe_context *pipe, void *hwcso)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->gmtyprog = hwcso;
    nvc0->dirty |= NVC0_NEW_GMTYPROG;
}

static void
nvc0_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
                         struct pipe_resource *res)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);

   switch (shader) {
   case PIPE_SHADER_VERTEX: shader = 0; break;
      /*
   case PIPE_SHADER_TESSELLATION_CONTROL: shader = 1; break;
   case PIPE_SHADER_TESSELLATION_EVALUATION: shader = 2; break;
      */
   case PIPE_SHADER_GEOMETRY: shader = 3; break;
   case PIPE_SHADER_FRAGMENT: shader = 4; break;
   default:
      assert(0);
      break;
   }

   if (nvc0->constbuf[shader][index])
      nvc0_bufctx_del_resident(nvc0, NVC0_BUFCTX_CONSTANT,
			       nv04_resource(nvc0->constbuf[shader][index]));

   pipe_resource_reference(&nvc0->constbuf[shader][index], res);

   nvc0->constbuf_dirty[shader] |= 1 << index;

   nvc0->dirty |= NVC0_NEW_CONSTBUF;
}

/* =============================================================================
 */

static void
nvc0_set_blend_color(struct pipe_context *pipe,
                     const struct pipe_blend_color *bcol)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->blend_colour = *bcol;
    nvc0->dirty |= NVC0_NEW_BLEND_COLOUR;
}

static void
nvc0_set_stencil_ref(struct pipe_context *pipe,
                     const struct pipe_stencil_ref *sr)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->stencil_ref = *sr;
    nvc0->dirty |= NVC0_NEW_STENCIL_REF;
}

static void
nvc0_set_clip_state(struct pipe_context *pipe,
                    const struct pipe_clip_state *clip)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);
    const unsigned size = clip->nr * sizeof(clip->ucp[0]);

    memcpy(&nvc0->clip.ucp[0][0], &clip->ucp[0][0], size);
    nvc0->clip.nr = clip->nr;

    nvc0->clip.depth_clamp = clip->depth_clamp;

    nvc0->dirty |= NVC0_NEW_CLIP;
}

static void
nvc0_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->sample_mask = sample_mask;
    nvc0->dirty |= NVC0_NEW_SAMPLE_MASK;
}


static void
nvc0_set_framebuffer_state(struct pipe_context *pipe,
                           const struct pipe_framebuffer_state *fb)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->framebuffer = *fb;
    nvc0->dirty |= NVC0_NEW_FRAMEBUFFER;
}

static void
nvc0_set_polygon_stipple(struct pipe_context *pipe,
                         const struct pipe_poly_stipple *stipple)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->stipple = *stipple;
    nvc0->dirty |= NVC0_NEW_STIPPLE;
}

static void
nvc0_set_scissor_state(struct pipe_context *pipe,
                       const struct pipe_scissor_state *scissor)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->scissor = *scissor;
    nvc0->dirty |= NVC0_NEW_SCISSOR;
}

static void
nvc0_set_viewport_state(struct pipe_context *pipe,
                        const struct pipe_viewport_state *vpt)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->viewport = *vpt;
    nvc0->dirty |= NVC0_NEW_VIEWPORT;
}

static void
nvc0_set_vertex_buffers(struct pipe_context *pipe,
                        unsigned count,
                        const struct pipe_vertex_buffer *vb)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);
    unsigned i;

    for (i = 0; i < count; ++i)
       pipe_resource_reference(&nvc0->vtxbuf[i].buffer, vb[i].buffer);
    for (; i < nvc0->num_vtxbufs; ++i)
       pipe_resource_reference(&nvc0->vtxbuf[i].buffer, NULL);

    memcpy(nvc0->vtxbuf, vb, sizeof(*vb) * count);
    nvc0->num_vtxbufs = count;

    nvc0_bufctx_reset(nvc0, NVC0_BUFCTX_VERTEX);

    nvc0->dirty |= NVC0_NEW_ARRAYS;
}

static void
nvc0_set_index_buffer(struct pipe_context *pipe,
                      const struct pipe_index_buffer *ib)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    if (ib) {
       pipe_resource_reference(&nvc0->idxbuf.buffer, ib->buffer);

       memcpy(&nvc0->idxbuf, ib, sizeof(nvc0->idxbuf));
    } else {
       pipe_resource_reference(&nvc0->idxbuf.buffer, NULL);
    }
}

static void
nvc0_vertex_state_bind(struct pipe_context *pipe, void *hwcso)
{
    struct nvc0_context *nvc0 = nvc0_context(pipe);

    nvc0->vertex = hwcso;
    nvc0->dirty |= NVC0_NEW_VERTEX;
}

static void *
nvc0_tfb_state_create(struct pipe_context *pipe,
                      const struct pipe_stream_output_state *pso)
{
   struct nvc0_transform_feedback_state *so;
   int n = 0;
   int i, c, b;

   so = MALLOC(sizeof(*so) + pso->num_outputs * 4 * sizeof(uint8_t));
   if (!so)
      return NULL;

   for (b = 0; b < 4; ++b) {
      for (i = 0; i < pso->num_outputs; ++i) {
         if (pso->output_buffer[i] != b)
            continue;
         for (c = 0; c < 4; ++c) {
            if (!(pso->register_mask[i] & (1 << c)))
               continue;
            so->varying_count[b]++;
            so->varying_index[n++] = (pso->register_index[i] << 2) | c;
         }
      }
      so->stride[b] = so->varying_count[b] * 4;
   }
   if (pso->stride)
      so->stride[0] = pso->stride;

   return so;
}

static void
nvc0_tfb_state_delete(struct pipe_context *pipe, void *hwcso)
{
   FREE(hwcso);
}

static void
nvc0_tfb_state_bind(struct pipe_context *pipe, void *hwcso)
{
   nvc0_context(pipe)->tfb = hwcso;
   nvc0_context(pipe)->dirty |= NVC0_NEW_TFB;
}

static void
nvc0_set_transform_feedback_buffers(struct pipe_context *pipe,
                                    struct pipe_resource **buffers,
                                    int *offsets,
                                    int num_buffers)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   int i;

   assert(num_buffers >= 0 && num_buffers <= 4); /* why signed ? */

   for (i = 0; i < num_buffers; ++i) {
       assert(offsets[i] >= 0);
       nvc0->tfb_offset[i] = offsets[i];
       pipe_resource_reference(&nvc0->tfbbuf[i], buffers[i]);
   }
   for (; i < nvc0->num_tfbbufs; ++i)
      pipe_resource_reference(&nvc0->tfbbuf[i], NULL);

   nvc0->num_tfbbufs = num_buffers;

   nvc0->dirty |= NVC0_NEW_TFB_BUFFERS;
}

void
nvc0_init_state_functions(struct nvc0_context *nvc0)
{
   struct pipe_context *pipe = &nvc0->base.pipe;

   pipe->create_blend_state = nvc0_blend_state_create;
   pipe->bind_blend_state = nvc0_blend_state_bind;
   pipe->delete_blend_state = nvc0_blend_state_delete;

   pipe->create_rasterizer_state = nvc0_rasterizer_state_create;
   pipe->bind_rasterizer_state = nvc0_rasterizer_state_bind;
   pipe->delete_rasterizer_state = nvc0_rasterizer_state_delete;

   pipe->create_depth_stencil_alpha_state = nvc0_zsa_state_create;
   pipe->bind_depth_stencil_alpha_state = nvc0_zsa_state_bind;
   pipe->delete_depth_stencil_alpha_state = nvc0_zsa_state_delete;

   pipe->create_sampler_state = nv50_sampler_state_create;
   pipe->delete_sampler_state = nvc0_sampler_state_delete;
   pipe->bind_vertex_sampler_states   = nvc0_vp_sampler_states_bind;
   pipe->bind_fragment_sampler_states = nvc0_fp_sampler_states_bind;
   pipe->bind_geometry_sampler_states = nvc0_gp_sampler_states_bind;

   pipe->create_sampler_view = nvc0_create_sampler_view;
   pipe->sampler_view_destroy = nvc0_sampler_view_destroy;
   pipe->set_vertex_sampler_views   = nvc0_vp_set_sampler_views;
   pipe->set_fragment_sampler_views = nvc0_fp_set_sampler_views;
   pipe->set_geometry_sampler_views = nvc0_gp_set_sampler_views;

   pipe->create_vs_state = nvc0_vp_state_create;
   pipe->create_fs_state = nvc0_fp_state_create;
   pipe->create_gs_state = nvc0_gp_state_create;
   pipe->bind_vs_state = nvc0_vp_state_bind;
   pipe->bind_fs_state = nvc0_fp_state_bind;
   pipe->bind_gs_state = nvc0_gp_state_bind;
   pipe->delete_vs_state = nvc0_sp_state_delete;
   pipe->delete_fs_state = nvc0_sp_state_delete;
   pipe->delete_gs_state = nvc0_sp_state_delete;

   pipe->set_blend_color = nvc0_set_blend_color;
   pipe->set_stencil_ref = nvc0_set_stencil_ref;
   pipe->set_clip_state = nvc0_set_clip_state;
   pipe->set_sample_mask = nvc0_set_sample_mask;
   pipe->set_constant_buffer = nvc0_set_constant_buffer;
   pipe->set_framebuffer_state = nvc0_set_framebuffer_state;
   pipe->set_polygon_stipple = nvc0_set_polygon_stipple;
   pipe->set_scissor_state = nvc0_set_scissor_state;
   pipe->set_viewport_state = nvc0_set_viewport_state;

   pipe->create_vertex_elements_state = nvc0_vertex_state_create;
   pipe->delete_vertex_elements_state = nvc0_vertex_state_delete;
   pipe->bind_vertex_elements_state = nvc0_vertex_state_bind;

   pipe->set_vertex_buffers = nvc0_set_vertex_buffers;
   pipe->set_index_buffer = nvc0_set_index_buffer;

   pipe->create_stream_output_state = nvc0_tfb_state_create;
   pipe->delete_stream_output_state = nvc0_tfb_state_delete;
   pipe->bind_stream_output_state = nvc0_tfb_state_bind;
   pipe->set_stream_output_buffers = nvc0_set_transform_feedback_buffers;

   pipe->redefine_user_buffer = u_default_redefine_user_buffer;
}

