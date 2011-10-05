
#ifndef __NV50_STATEOBJ_H__
#define __NV50_STATEOBJ_H__

#include "pipe/p_state.h"

#define NV50_SCISSORS_CLIPPING

#define SB_BEGIN_3D(so, m, s)                                                  \
   (so)->state[(so)->size++] =                                                 \
      ((s) << 18) | (NV50_SUBCH_3D << 13) | NV50_3D_##m

#define SB_BEGIN_3D_(so, m, s)                                                 \
   (so)->state[(so)->size++] =                                                 \
      ((s) << 18) | (NV50_SUBCH_3D << 13) | m

#define SB_DATA(so, u) (so)->state[(so)->size++] = (u)

#include "nv50_stateobj_tex.h"

struct nv50_blend_stateobj {
   struct pipe_blend_state pipe;
   int size;
   uint32_t state[82]; // TODO: allocate less if !independent_blend_enable
};

struct nv50_rasterizer_stateobj {
   struct pipe_rasterizer_state pipe;
   int size;
   uint32_t state[42];
};

struct nv50_zsa_stateobj {
   struct pipe_depth_stencil_alpha_state pipe;
   int size;
   uint32_t state[29];
};

struct nv50_vertex_element {
   struct pipe_vertex_element pipe;
   uint32_t state;
};

struct nv50_vertex_stateobj {
   struct translate *translate;
   unsigned num_elements;
   uint32_t instance_elts;
   uint32_t instance_bufs;
   boolean need_conversion;
   unsigned vertex_size;
   unsigned packet_vertex_limit;
   struct nv50_vertex_element element[0];
};

#endif
