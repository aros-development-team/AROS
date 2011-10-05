#ifndef __NV50_CONTEXT_H__
#define __NV50_CONTEXT_H__

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_dynarray.h"

#include "draw/draw_vertex.h"

#include "nv50_debug.h"
#include "nv50_winsys.h"
#include "nv50_stateobj.h"
#include "nv50_screen.h"
#include "nv50_program.h"
#include "nv50_resource.h"

#include "nouveau/nouveau_context.h"
#include "nouveau/nv_object.xml.h"
#include "nouveau/nv_m2mf.xml.h"
#include "nv50_3ddefs.xml.h"
#include "nv50_3d.xml.h"
#include "nv50_2d.xml.h"

#define NV50_NEW_BLEND        (1 << 0)
#define NV50_NEW_RASTERIZER   (1 << 1)
#define NV50_NEW_ZSA          (1 << 2)
#define NV50_NEW_VERTPROG     (1 << 3)
#define NV50_NEW_GMTYPROG     (1 << 6)
#define NV50_NEW_FRAGPROG     (1 << 7)
#define NV50_NEW_BLEND_COLOUR (1 << 8)
#define NV50_NEW_STENCIL_REF  (1 << 9)
#define NV50_NEW_CLIP         (1 << 10)
#define NV50_NEW_SAMPLE_MASK  (1 << 11)
#define NV50_NEW_FRAMEBUFFER  (1 << 12)
#define NV50_NEW_STIPPLE      (1 << 13)
#define NV50_NEW_SCISSOR      (1 << 14)
#define NV50_NEW_VIEWPORT     (1 << 15)
#define NV50_NEW_ARRAYS       (1 << 16)
#define NV50_NEW_VERTEX       (1 << 17)
#define NV50_NEW_CONSTBUF     (1 << 18)
#define NV50_NEW_TEXTURES     (1 << 19)
#define NV50_NEW_SAMPLERS     (1 << 20)

#define NV50_BUFCTX_CONSTANT 0
#define NV50_BUFCTX_FRAME    1
#define NV50_BUFCTX_VERTEX   2
#define NV50_BUFCTX_TEXTURES 3
#define NV50_BUFCTX_COUNT    4

/* fixed constant buffer binding points - low indices for user's constbufs */
#define NV50_CB_PVP 124
#define NV50_CB_PGP 126
#define NV50_CB_PFP 125
#define NV50_CB_AUX 127

struct nv50_context {
   struct nouveau_context base;

   struct nv50_screen *screen;

   struct util_dynarray residents[NV50_BUFCTX_COUNT];
   unsigned residents_size;

   uint32_t dirty;

   struct {
      uint32_t instance_elts; /* bitmask of per-instance elements */
      uint32_t instance_base;
      uint32_t interpolant_ctrl;
      uint32_t semantic_color;
      uint32_t semantic_psize;
      int32_t index_bias;
      boolean prim_restart;
      boolean point_sprite;
      uint8_t num_vtxbufs;
      uint8_t num_vtxelts;
      uint8_t num_textures[3];
      uint8_t num_samplers[3];
      uint16_t scissor;
   } state;

   struct nv50_blend_stateobj *blend;
   struct nv50_rasterizer_stateobj *rast;
   struct nv50_zsa_stateobj *zsa;
   struct nv50_vertex_stateobj *vertex;

   struct nv50_program *vertprog;
   struct nv50_program *gmtyprog;
   struct nv50_program *fragprog;

   struct pipe_resource *constbuf[3][16];
   uint16_t constbuf_dirty[3];

   struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
   unsigned num_vtxbufs;
   struct pipe_index_buffer idxbuf;
   uint32_t vbo_fifo; /* bitmask of vertex elements to be pushed to FIFO */
   uint32_t vbo_user; /* bitmask of vertex buffers pointing to user memory */
   unsigned vbo_min_index; /* from pipe_draw_info, for vertex upload */
   unsigned vbo_max_index;

   struct pipe_sampler_view *textures[3][PIPE_MAX_SAMPLERS];
   unsigned num_textures[3];
   struct nv50_tsc_entry *samplers[3][PIPE_MAX_SAMPLERS];
   unsigned num_samplers[3];

   struct pipe_framebuffer_state framebuffer;
   struct pipe_blend_color blend_colour;
   struct pipe_stencil_ref stencil_ref;
   struct pipe_poly_stipple stipple;
   struct pipe_scissor_state scissor;
   struct pipe_viewport_state viewport;
   struct pipe_clip_state clip;

   unsigned sample_mask;

   boolean vbo_push_hint;

   struct draw_context *draw;
};

static INLINE struct nv50_context *
nv50_context(struct pipe_context *pipe)
{
   return (struct nv50_context *)pipe;
}

struct nv50_surface {
   struct pipe_surface base;
   uint32_t offset;
   uint32_t width;
   uint16_t height;
   uint16_t depth;
};

static INLINE struct nv50_surface *
nv50_surface(struct pipe_surface *ps)
{
   return (struct nv50_surface *)ps;
}

/* nv50_context.c */
struct pipe_context *nv50_create(struct pipe_screen *, void *);

void nv50_default_flush_notify(struct nouveau_channel *);

void nv50_bufctx_emit_relocs(struct nv50_context *);
void nv50_bufctx_add_resident(struct nv50_context *, int ctx,
                              struct nv04_resource *, uint32_t flags);
void nv50_bufctx_del_resident(struct nv50_context *, int ctx,
                              struct nv04_resource *);
static INLINE void
nv50_bufctx_reset(struct nv50_context *nv50, int ctx)
{
   nv50->residents_size -= nv50->residents[ctx].size;
   util_dynarray_resize(&nv50->residents[ctx], 0);
}

/* nv50_draw.c */
extern struct draw_stage *nv50_draw_render_stage(struct nv50_context *);

/* nv50_program.c */
boolean nv50_program_translate(struct nv50_program *);
void nv50_program_destroy(struct nv50_context *, struct nv50_program *);

/* nv50_query.c */
void nv50_init_query_functions(struct nv50_context *);

/* nv50_shader_state.c */
void nv50_vertprog_validate(struct nv50_context *);
void nv50_gmtyprog_validate(struct nv50_context *);
void nv50_fragprog_validate(struct nv50_context *);
void nv50_fp_linkage_validate(struct nv50_context *);
void nv50_gp_linkage_validate(struct nv50_context *);
void nv50_constbufs_validate(struct nv50_context *);
void nv50_validate_derived_rs(struct nv50_context *);

/* nv50_state.c */
extern void nv50_init_state_functions(struct nv50_context *);

/* nv50_state_validate.c */
extern boolean nv50_state_validate(struct nv50_context *);

/* nv50_surface.c */
extern void nv50_clear(struct pipe_context *, unsigned buffers,
                       const float *rgba, double depth, unsigned stencil);
extern void nv50_init_surface_functions(struct nv50_context *);

/* nv50_tex.c */
void nv50_validate_textures(struct nv50_context *);
void nv50_validate_samplers(struct nv50_context *);

struct pipe_sampler_view *
nv50_create_sampler_view(struct pipe_context *,
                         struct pipe_resource *,
                         const struct pipe_sampler_view *);

/* nv50_transfer.c */
void
nv50_sifc_linear_u8(struct nouveau_context *pipe,
                    struct nouveau_bo *dst, unsigned offset, unsigned domain,
                    unsigned size, void *data);
void
nv50_m2mf_copy_linear(struct nouveau_context *pipe,
                      struct nouveau_bo *dst, unsigned dstoff, unsigned dstdom,
                      struct nouveau_bo *src, unsigned srcoff, unsigned srcdom,
                      unsigned size);

/* nv50_vbo.c */
void nv50_draw_vbo(struct pipe_context *, const struct pipe_draw_info *);

void *
nv50_vertex_state_create(struct pipe_context *pipe,
                         unsigned num_elements,
                         const struct pipe_vertex_element *elements);
void
nv50_vertex_state_delete(struct pipe_context *pipe, void *hwcso);

void nv50_vertex_arrays_validate(struct nv50_context *nv50);

/* nv50_push.c */
void nv50_push_vbo(struct nv50_context *, const struct pipe_draw_info *);

#endif
