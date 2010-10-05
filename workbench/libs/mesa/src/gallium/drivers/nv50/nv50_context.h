#ifndef __NV50_CONTEXT_H__
#define __NV50_CONTEXT_H__

#include <stdio.h>
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_compiler.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_inlines.h"

#include "draw/draw_vertex.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"
#include "nouveau/nouveau_stateobj.h"
#include "nv50_reg.h"

#include "nv50_screen.h"
#include "nv50_program.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __FUNCTION__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

/* Constant buffer assignment */
#define NV50_CB_PMISC		0
#define NV50_CB_PVP		1
#define NV50_CB_PFP		2
#define NV50_CB_PGP		3
#define NV50_CB_AUX		4

#define NV50_NEW_BLEND		(1 << 0)
#define NV50_NEW_ZSA		(1 << 1)
#define NV50_NEW_BLEND_COLOUR	(1 << 2)
#define NV50_NEW_STIPPLE	(1 << 3)
#define NV50_NEW_SCISSOR	(1 << 4)
#define NV50_NEW_VIEWPORT	(1 << 5)
#define NV50_NEW_RASTERIZER	(1 << 6)
#define NV50_NEW_FRAMEBUFFER	(1 << 7)
#define NV50_NEW_VERTPROG	(1 << 8)
#define NV50_NEW_VERTPROG_CB	(1 << 9)
#define NV50_NEW_FRAGPROG	(1 << 10)
#define NV50_NEW_FRAGPROG_CB	(1 << 11)
#define NV50_NEW_GEOMPROG	(1 << 12)
#define NV50_NEW_GEOMPROG_CB	(1 << 13)
#define NV50_NEW_ARRAYS		(1 << 14)
#define NV50_NEW_SAMPLER	(1 << 15)
#define NV50_NEW_TEXTURE	(1 << 16)
#define NV50_NEW_STENCIL_REF	(1 << 17)
#define NV50_NEW_CLIP		(1 << 18)

struct nv50_blend_stateobj {
	struct pipe_blend_state pipe;
	struct nouveau_stateobj *so;
};

struct nv50_zsa_stateobj {
	struct pipe_depth_stencil_alpha_state pipe;
	struct nouveau_stateobj *so;
};

struct nv50_rasterizer_stateobj {
	struct pipe_rasterizer_state pipe;
	struct nouveau_stateobj *so;
};

struct nv50_sampler_stateobj {
	boolean normalized;
	unsigned tsc[8];
};

struct nv50_sampler_view {
	struct pipe_sampler_view pipe;
	uint32_t tic[8];
};

struct nv50_vtxelt_stateobj {
	struct pipe_vertex_element pipe[16];
	unsigned num_elements;
	uint32_t hw[16];
};

static INLINE struct nv50_sampler_view *
nv50_sampler_view(struct pipe_sampler_view *view)
{
	return (struct nv50_sampler_view *)view;
}

static INLINE unsigned
get_tile_height(uint32_t tile_mode)
{
        return 1 << ((tile_mode & 0xf) + 2);
}

static INLINE unsigned
get_tile_depth(uint32_t tile_mode)
{
        return 1 << (tile_mode >> 4);
}


struct nv50_surface {
	struct pipe_surface base;
};

static INLINE struct nv50_surface *
nv50_surface(struct pipe_surface *pt)
{
	return (struct nv50_surface *)pt;
}

struct nv50_state {
	struct nouveau_stateobj *hw[64];
	uint64_t hw_dirty;

	unsigned sampler_view_nr[3];
	struct nouveau_stateobj *vtxbuf;
	struct nouveau_stateobj *vtxattr;
	unsigned vtxelt_nr;
};

struct nv50_context {
	struct pipe_context pipe;

	struct nv50_screen *screen;

	struct draw_context *draw;

	struct nv50_state state;

	unsigned dirty;
	struct nv50_blend_stateobj *blend;
	struct nv50_zsa_stateobj *zsa;
	struct nv50_rasterizer_stateobj *rasterizer;
	struct pipe_blend_color blend_colour;
	struct pipe_stencil_ref stencil_ref;
	struct pipe_poly_stipple stipple;
	struct pipe_scissor_state scissor;
	struct pipe_viewport_state viewport;
	struct pipe_framebuffer_state framebuffer;
	struct pipe_clip_state clip;
	struct nv50_program *vertprog;
	struct nv50_program *fragprog;
	struct nv50_program *geomprog;
	struct pipe_resource *constbuf[PIPE_SHADER_TYPES];
	struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
	unsigned vtxbuf_nr;
	struct pipe_index_buffer idxbuf;
	struct nv50_vtxelt_stateobj *vtxelt;
	struct nv50_sampler_stateobj *sampler[3][PIPE_MAX_SAMPLERS];
	unsigned sampler_nr[3];
	struct pipe_sampler_view *sampler_views[3][PIPE_MAX_SAMPLERS];
	unsigned sampler_view_nr[3];

	unsigned vbo_fifo;
};

static INLINE struct nv50_context *
nv50_context(struct pipe_context *pipe)
{
	return (struct nv50_context *)pipe;
}

extern void nv50_init_surface_functions(struct nv50_context *nv50);
extern void nv50_init_state_functions(struct nv50_context *nv50);
extern void nv50_init_query_functions(struct nv50_context *nv50);
extern void nv50_init_transfer_functions(struct nv50_context *nv50);

extern void nv50_screen_init_miptree_functions(struct pipe_screen *pscreen);

extern int
nv50_surface_do_copy(struct nv50_screen *screen, struct pipe_surface *dst,
		     int dx, int dy, struct pipe_surface *src, int sx, int sy,
		     int w, int h);

/* nv50_draw.c */
extern struct draw_stage *nv50_draw_render_stage(struct nv50_context *nv50);

/* nv50_vbo.c */
extern void nv50_draw_vbo(struct pipe_context *pipe,
                          const struct pipe_draw_info *info);
extern void nv50_vtxelt_construct(struct nv50_vtxelt_stateobj *cso);
extern struct nouveau_stateobj *nv50_vbo_validate(struct nv50_context *nv50);

/* nv50_push.c */
extern void
nv50_push_elements_instanced(struct pipe_context *, struct pipe_resource *,
			     unsigned idxsize, int idxbias,
                             unsigned mode, unsigned start,
			     unsigned count, unsigned i_start,
			     unsigned i_count);

/* nv50_clear.c */
extern void nv50_clear(struct pipe_context *pipe, unsigned buffers,
		       const float *rgba, double depth, unsigned stencil);

/* nv50_program.c */
extern struct nouveau_stateobj *
nv50_vertprog_validate(struct nv50_context *nv50);
extern struct nouveau_stateobj *
nv50_fragprog_validate(struct nv50_context *nv50);
extern struct nouveau_stateobj *
nv50_geomprog_validate(struct nv50_context *nv50);
extern struct nouveau_stateobj *
nv50_fp_linkage_validate(struct nv50_context *nv50);
extern struct nouveau_stateobj *
nv50_gp_linkage_validate(struct nv50_context *nv50);
extern void nv50_program_destroy(struct nv50_context *nv50,
				 struct nv50_program *p);

/* nv50_state_validate.c */
extern boolean nv50_state_validate(struct nv50_context *nv50, unsigned dwords);

extern void nv50_so_init_sifc(struct nv50_context *nv50,
			      struct nouveau_stateobj *so,
			      struct nouveau_bo *bo, unsigned reloc,
			      unsigned offset, unsigned size);

/* nv50_tex.c */
extern boolean nv50_tex_construct(struct nv50_sampler_view *view);
extern void nv50_tex_relocs(struct nv50_context *);
extern struct nouveau_stateobj *nv50_tex_validate(struct nv50_context *);


/* nv50_context.c */
struct pipe_context *
nv50_create(struct pipe_screen *pscreen, void *priv);

static INLINE unsigned
nv50_prim(unsigned mode)
{
	switch (mode) {
	case PIPE_PRIM_POINTS: return NV50TCL_VERTEX_BEGIN_POINTS;
	case PIPE_PRIM_LINES: return NV50TCL_VERTEX_BEGIN_LINES;
	case PIPE_PRIM_LINE_LOOP: return NV50TCL_VERTEX_BEGIN_LINE_LOOP;
	case PIPE_PRIM_LINE_STRIP: return NV50TCL_VERTEX_BEGIN_LINE_STRIP;
	case PIPE_PRIM_TRIANGLES: return NV50TCL_VERTEX_BEGIN_TRIANGLES;
	case PIPE_PRIM_TRIANGLE_STRIP:
		return NV50TCL_VERTEX_BEGIN_TRIANGLE_STRIP;
	case PIPE_PRIM_TRIANGLE_FAN: return NV50TCL_VERTEX_BEGIN_TRIANGLE_FAN;
	case PIPE_PRIM_QUADS: return NV50TCL_VERTEX_BEGIN_QUADS;
	case PIPE_PRIM_QUAD_STRIP: return NV50TCL_VERTEX_BEGIN_QUAD_STRIP;
	case PIPE_PRIM_POLYGON: return NV50TCL_VERTEX_BEGIN_POLYGON;
	case PIPE_PRIM_LINES_ADJACENCY:
		return NV50TCL_VERTEX_BEGIN_LINES_ADJACENCY;
	case PIPE_PRIM_LINE_STRIP_ADJACENCY:
		return NV50TCL_VERTEX_BEGIN_LINE_STRIP_ADJACENCY;
	case PIPE_PRIM_TRIANGLES_ADJACENCY:
		return NV50TCL_VERTEX_BEGIN_TRIANGLES_ADJACENCY;
	case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
		return NV50TCL_VERTEX_BEGIN_TRIANGLE_STRIP_ADJACENCY;
	default:
		break;
	}

	NOUVEAU_ERR("invalid primitive type %d\n", mode);
	return NV50TCL_VERTEX_BEGIN_POINTS;
}

#endif
