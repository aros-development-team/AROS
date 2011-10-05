#ifndef __NVC0_CONTEXT_H__
#define __NVC0_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_dynarray.h"

#include "draw/draw_vertex.h"

#include "nv50/nv50_debug.h"
#include "nvc0_winsys.h"
#include "nvc0_stateobj.h"
#include "nvc0_screen.h"
#include "nvc0_program.h"
#include "nvc0_resource.h"

#include "nouveau/nouveau_context.h"

#include "nvc0_3ddefs.xml.h"
#include "nvc0_3d.xml.h"
#include "nvc0_2d.xml.h"
#include "nvc0_m2mf.xml.h"

#define NVC0_NEW_BLEND        (1 << 0)
#define NVC0_NEW_RASTERIZER   (1 << 1)
#define NVC0_NEW_ZSA          (1 << 2)
#define NVC0_NEW_VERTPROG     (1 << 3)
#define NVC0_NEW_TCTLPROG     (1 << 4)
#define NVC0_NEW_TEVLPROG     (1 << 5)
#define NVC0_NEW_GMTYPROG     (1 << 6)
#define NVC0_NEW_FRAGPROG     (1 << 7)
#define NVC0_NEW_BLEND_COLOUR (1 << 8)
#define NVC0_NEW_STENCIL_REF  (1 << 9)
#define NVC0_NEW_CLIP         (1 << 10)
#define NVC0_NEW_SAMPLE_MASK  (1 << 11)
#define NVC0_NEW_FRAMEBUFFER  (1 << 12)
#define NVC0_NEW_STIPPLE      (1 << 13)
#define NVC0_NEW_SCISSOR      (1 << 14)
#define NVC0_NEW_VIEWPORT     (1 << 15)
#define NVC0_NEW_ARRAYS       (1 << 16)
#define NVC0_NEW_VERTEX       (1 << 17)
#define NVC0_NEW_CONSTBUF     (1 << 18)
#define NVC0_NEW_TEXTURES     (1 << 19)
#define NVC0_NEW_SAMPLERS     (1 << 20)
#define NVC0_NEW_TFB          (1 << 21)
#define NVC0_NEW_TFB_BUFFERS  (1 << 22)

#define NVC0_BUFCTX_CONSTANT 0
#define NVC0_BUFCTX_FRAME    1
#define NVC0_BUFCTX_VERTEX   2
#define NVC0_BUFCTX_TEXTURES 3
#define NVC0_BUFCTX_COUNT    4

struct nvc0_context {
   struct nouveau_context base;

   struct nvc0_screen *screen;

   struct util_dynarray residents[NVC0_BUFCTX_COUNT];
   unsigned residents_size;

   uint32_t dirty;

   struct {
      uint32_t instance_elts; /* bitmask of per-instance elements */
      uint32_t instance_base;
      int32_t index_bias;
      boolean prim_restart;
      boolean early_z;
      uint8_t num_vtxbufs;
      uint8_t num_vtxelts;
      uint8_t num_textures[5];
      uint8_t num_samplers[5];
      uint8_t tls_required; /* bitmask of shader types using l[] */
      uint16_t scissor;
      uint32_t uniform_buffer_bound[5];
   } state;

   struct nvc0_blend_stateobj *blend;
   struct nvc0_rasterizer_stateobj *rast;
   struct nvc0_zsa_stateobj *zsa;
   struct nvc0_vertex_stateobj *vertex;

   struct nvc0_program *vertprog;
   struct nvc0_program *tctlprog;
   struct nvc0_program *tevlprog;
   struct nvc0_program *gmtyprog;
   struct nvc0_program *fragprog;

   struct pipe_resource *constbuf[5][16];
   uint16_t constbuf_dirty[5];

   struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
   unsigned num_vtxbufs;
   struct pipe_index_buffer idxbuf;
   uint32_t vbo_fifo; /* bitmask of vertex elements to be pushed to FIFO */
   uint32_t vbo_user; /* bitmask of vertex buffers pointing to user memory */
   unsigned vbo_min_index; /* from pipe_draw_info, for vertex upload */
   unsigned vbo_max_index;

   struct pipe_sampler_view *textures[5][PIPE_MAX_SAMPLERS];
   unsigned num_textures[5];
   struct nv50_tsc_entry *samplers[5][PIPE_MAX_SAMPLERS];
   unsigned num_samplers[5];

   struct pipe_framebuffer_state framebuffer;
   struct pipe_blend_color blend_colour;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_poly_stipple stipple;
   struct pipe_scissor_state scissor;
   struct pipe_viewport_state viewport;
   struct pipe_clip_state clip;

   unsigned sample_mask;

   boolean vbo_push_hint;

   struct nvc0_transform_feedback_state *tfb;
   struct pipe_resource *tfbbuf[4];
   unsigned num_tfbbufs;
   unsigned tfb_offset[4];

   struct draw_context *draw;
};

#define NVC0_USING_EDGEFLAG(ctx) \
   ((ctx)->vertprog->vp.edgeflag < PIPE_MAX_ATTRIBS)

static INLINE struct nvc0_context *
nvc0_context(struct pipe_context *pipe)
{
   return (struct nvc0_context *)pipe;
}

struct nvc0_surface {
   struct pipe_surface base;
   uint32_t offset;
   uint32_t width;
   uint16_t height;
   uint16_t depth;
};

static INLINE struct nvc0_surface *
nvc0_surface(struct pipe_surface *ps)
{
   return (struct nvc0_surface *)ps;
}

/* nvc0_context.c */
struct pipe_context *nvc0_create(struct pipe_screen *, void *);

void nvc0_default_flush_notify(struct nouveau_channel *);

void nvc0_bufctx_emit_relocs(struct nvc0_context *);
void nvc0_bufctx_add_resident(struct nvc0_context *, int ctx,
                              struct nv04_resource *, uint32_t flags);
void nvc0_bufctx_del_resident(struct nvc0_context *, int ctx,
                              struct nv04_resource *);
static INLINE void
nvc0_bufctx_reset(struct nvc0_context *nvc0, int ctx)
{
   nvc0->residents_size -= nvc0->residents[ctx].size;
   util_dynarray_resize(&nvc0->residents[ctx], 0);
}

/* nvc0_draw.c */
extern struct draw_stage *nvc0_draw_render_stage(struct nvc0_context *);

/* nvc0_program.c */
boolean nvc0_program_translate(struct nvc0_program *);
void nvc0_program_destroy(struct nvc0_context *, struct nvc0_program *);

/* nvc0_query.c */
void nvc0_init_query_functions(struct nvc0_context *);

/* nvc0_shader_state.c */
void nvc0_vertprog_validate(struct nvc0_context *);
void nvc0_tctlprog_validate(struct nvc0_context *);
void nvc0_tevlprog_validate(struct nvc0_context *);
void nvc0_gmtyprog_validate(struct nvc0_context *);
void nvc0_fragprog_validate(struct nvc0_context *);

void nvc0_tfb_validate(struct nvc0_context *);

/* nvc0_state.c */
extern void nvc0_init_state_functions(struct nvc0_context *);

/* nvc0_state_validate.c */
extern boolean nvc0_state_validate(struct nvc0_context *);

/* nvc0_surface.c */
extern void nvc0_clear(struct pipe_context *, unsigned buffers,
                       const float *rgba, double depth, unsigned stencil);
extern void nvc0_init_surface_functions(struct nvc0_context *);

/* nvc0_tex.c */
void nvc0_validate_textures(struct nvc0_context *);
void nvc0_validate_samplers(struct nvc0_context *);

struct pipe_sampler_view *
nvc0_create_sampler_view(struct pipe_context *,
                         struct pipe_resource *,
                         const struct pipe_sampler_view *);

/* nvc0_transfer.c */
void
nvc0_m2mf_push_linear(struct nouveau_context *nv,
		      struct nouveau_bo *dst, unsigned offset, unsigned domain,
		      unsigned size, void *data);
void
nvc0_m2mf_copy_linear(struct nouveau_context *nv,
		      struct nouveau_bo *dst, unsigned dstoff, unsigned dstdom,
		      struct nouveau_bo *src, unsigned srcoff, unsigned srcdom,
		      unsigned size);

/* nvc0_vbo.c */
void nvc0_draw_vbo(struct pipe_context *, const struct pipe_draw_info *);

void *
nvc0_vertex_state_create(struct pipe_context *pipe,
                         unsigned num_elements,
                         const struct pipe_vertex_element *elements);
void
nvc0_vertex_state_delete(struct pipe_context *pipe, void *hwcso);

void nvc0_vertex_arrays_validate(struct nvc0_context *nvc0);

/* nvc0_push.c */
void nvc0_push_vbo(struct nvc0_context *, const struct pipe_draw_info *);
void nvc0_push_vbo2(struct nvc0_context *, const struct pipe_draw_info *);

#endif
